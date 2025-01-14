// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/ruleset_manager.h"

#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/declarative_net_request/ruleset_matcher.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/api/declarative_net_request/utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = url_pattern_index::flat;

// Maps content::ResourceType to flat_rule::ElementType.
flat_rule::ElementType GetElementType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_LAST_TYPE:
    case content::RESOURCE_TYPE_PREFETCH:
    case content::RESOURCE_TYPE_SUB_RESOURCE:
    // TODO(crbug.com/696822): Add support for main frame and csp report to
    // url_pattern_index. These are supported by the Web Request API.
    case content::RESOURCE_TYPE_MAIN_FRAME:
    case content::RESOURCE_TYPE_CSP_REPORT:
      return flat_rule::ElementType_OTHER;
    case content::RESOURCE_TYPE_SCRIPT:
    case content::RESOURCE_TYPE_WORKER:
    case content::RESOURCE_TYPE_SHARED_WORKER:
    case content::RESOURCE_TYPE_SERVICE_WORKER:
      return flat_rule::ElementType_SCRIPT;
    case content::RESOURCE_TYPE_IMAGE:
    case content::RESOURCE_TYPE_FAVICON:
      return flat_rule::ElementType_IMAGE;
    case content::RESOURCE_TYPE_STYLESHEET:
      return flat_rule::ElementType_STYLESHEET;
    case content::RESOURCE_TYPE_OBJECT:
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
      return flat_rule::ElementType_OBJECT;
    case content::RESOURCE_TYPE_XHR:
      return flat_rule::ElementType_XMLHTTPREQUEST;
    case content::RESOURCE_TYPE_SUB_FRAME:
      return flat_rule::ElementType_SUBDOCUMENT;
    case content::RESOURCE_TYPE_PING:
      return flat_rule::ElementType_PING;
    case content::RESOURCE_TYPE_MEDIA:
      return flat_rule::ElementType_MEDIA;
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      return flat_rule::ElementType_FONT;
  }
  NOTREACHED();
  return flat_rule::ElementType_OTHER;
}

// Returns the flat_rule::ElementType for the given |request|.
flat_rule::ElementType GetElementType(const net::URLRequest& request) {
  if (request.url().SchemeIsWSOrWSS())
    return flat_rule::ElementType_WEBSOCKET;

  const auto* info = content::ResourceRequestInfo::ForRequest(&request);
  return info ? GetElementType(info->GetResourceType())
              : flat_rule::ElementType_OTHER;
}

// Returns whether the request to |url| is third party to its |document_origin|.
// TODO(crbug.com/696822): Look into caching this.
bool IsThirdPartyRequest(const GURL& url, const url::Origin& document_origin) {
  if (document_origin.unique())
    return true;

  return !net::registry_controlled_domains::SameDomainOrHost(
      url, document_origin,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

void ClearRendererCacheOnUI() {
  web_cache::WebCacheManager::GetInstance()->ClearCacheOnNavigation();
}

// Helper to clear each renderer's in-memory cache the next time it navigates.
void ClearRendererCacheOnNavigation() {
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    ClearRendererCacheOnUI();
  } else {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::BindOnce(&ClearRendererCacheOnUI));
  }
}

}  // namespace

RulesetManager::RulesetManager(const InfoMap* info_map) : info_map_(info_map) {
  DCHECK(info_map_);

  // RulesetManager can be created on any sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

RulesetManager::~RulesetManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void RulesetManager::AddRuleset(
    const ExtensionId& extension_id,
    std::unique_ptr<RulesetMatcher> ruleset_matcher) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  DCHECK(!base::ContainsKey(rules_map_, extension_id))
      << "AddRuleset called twice in succession for " << extension_id;
  rules_map_[extension_id] = std::move(ruleset_matcher);

  // Clear the renderers' cache so that they take the new rules into account.
  ClearRendererCacheOnNavigation();
}

void RulesetManager::RemoveRuleset(const ExtensionId& extension_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsAPIAvailable());

  DCHECK(base::ContainsKey(rules_map_, extension_id))
      << "RemoveRuleset called without a corresponding AddRuleset for "
      << extension_id;
  rules_map_.erase(extension_id);

  // Clear the renderers' cache so that they take the removed rules into
  // account.
  ClearRendererCacheOnNavigation();
}

bool RulesetManager::ShouldBlockRequest(const net::URLRequest& request,
                                        bool is_incognito_context) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Return early if DNR is not enabled.
  if (!IsAPIAvailable())
    return false;

  if (test_observer_)
    test_observer_->OnShouldBlockRequest(request, is_incognito_context);

  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldBlockRequestTime.AllExtensions");

  const GURL& url = request.url();
  const url::Origin first_party_origin =
      request.initiator().value_or(url::Origin());
  const flat_rule::ElementType element_type = GetElementType(request);
  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);

  for (const auto& pair : rules_map_) {
    const bool evaluate_ruleset =
        !is_incognito_context || info_map_->IsIncognitoEnabled(pair.first);

    // TODO(crbug.com/777714): Check host permissions etc.
    if (evaluate_ruleset &&
        pair.second->ShouldBlockRequest(url, first_party_origin, element_type,
                                        is_third_party)) {
      return true;
    }
  }
  return false;
}

bool RulesetManager::ShouldRedirectRequest(const net::URLRequest& request,
                                           bool is_incognito_context,
                                           GURL* redirect_url) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(redirect_url);

  // Return early if DNR is not enabled.
  if (!IsAPIAvailable())
    return false;

  // Redirecting WebSocket handshake request is prohibited.
  const flat_rule::ElementType element_type = GetElementType(request);
  if (element_type == flat_rule::ElementType_WEBSOCKET)
    return false;

  SCOPED_UMA_HISTOGRAM_TIMER(
      "Extensions.DeclarativeNetRequest.ShouldRedirectRequestTime."
      "AllExtensions");

  const GURL& url = request.url();
  const url::Origin first_party_origin =
      request.initiator().value_or(url::Origin());

  const bool is_third_party = IsThirdPartyRequest(url, first_party_origin);

  bool redirect = false;
  base::Time winning_extension_install_time = base::Time::Min();

  // TODO(crbug.com/696822): Change |rules_map_| to be a list, sorted by
  // decreasing installation time. This will help prevent redundant evaluation
  // of rulesets.
  for (const auto& pair : rules_map_) {
    base::Time extension_install_time = info_map_->GetInstallTime(pair.first);

    // Give priority to more recently installed extension.
    const bool evaluate_ruleset =
        extension_install_time > winning_extension_install_time &&
        (!is_incognito_context || info_map_->IsIncognitoEnabled(pair.first));

    // TODO(crbug.com/777714): Check host permissions etc.
    if (evaluate_ruleset && pair.second->ShouldRedirectRequest(
                                url, first_party_origin, element_type,
                                is_third_party, redirect_url)) {
      winning_extension_install_time = extension_install_time;
      redirect = true;
    }
  }

  return redirect;
}

void RulesetManager::SetObserverForTest(TestObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  test_observer_ = observer;
}

}  // namespace declarative_net_request
}  // namespace extensions
