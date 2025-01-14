// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_frontend_host_impl.h"

#include <stddef.h>
#include <string>

#include "content/browser/bad_message.h"
#include "content/browser/devtools/grit/devtools_resources_map.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/associated_interface_provider.h"
#include "content/public/common/content_client.h"

namespace content {

namespace {
const char kCompatibilityScript[] = "devtools_compatibility.js";
const char kCompatibilityScriptSourceURL[] =
    "\n//# "
    "sourceURL=chrome-devtools://devtools/bundled/devtools_compatibility.js";
}

// static
DevToolsFrontendHost* DevToolsFrontendHost::Create(
    RenderFrameHost* frame_host,
    const HandleMessageCallback& handle_message_callback) {
  DCHECK(!frame_host->GetParent());
  return new DevToolsFrontendHostImpl(frame_host, handle_message_callback);
}

// static
void DevToolsFrontendHost::SetupExtensionsAPI(
    RenderFrameHost* frame_host,
    const std::string& extension_api) {
  DCHECK(frame_host->GetParent());
  mojom::DevToolsFrontendAssociatedPtr frontend;
  frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&frontend);
  frontend->SetupDevToolsExtensionAPI(extension_api);
}

// static
base::StringPiece DevToolsFrontendHost::GetFrontendResource(
    const std::string& path) {
  for (size_t i = 0; i < kDevtoolsResourcesSize; ++i) {
    if (path == kDevtoolsResources[i].name) {
      return GetContentClient()->GetDataResource(
          kDevtoolsResources[i].value, ui::SCALE_FACTOR_NONE);
    }
  }
  return std::string();
}

DevToolsFrontendHostImpl::DevToolsFrontendHostImpl(
    RenderFrameHost* frame_host,
    const HandleMessageCallback& handle_message_callback)
    : web_contents_(WebContents::FromRenderFrameHost(frame_host)),
      handle_message_callback_(handle_message_callback),
      binding_(this) {
  mojom::DevToolsFrontendAssociatedPtr frontend;
  frame_host->GetRemoteAssociatedInterfaces()->GetInterface(&frontend);
  std::string api_script =
      content::DevToolsFrontendHost::GetFrontendResource(kCompatibilityScript)
          .as_string() +
      kCompatibilityScriptSourceURL;
  mojom::DevToolsFrontendHostAssociatedPtrInfo host;
  binding_.Bind(mojo::MakeRequest(&host));
  frontend->SetupDevToolsFrontend(api_script, std::move(host));
}

DevToolsFrontendHostImpl::~DevToolsFrontendHostImpl() {
}

void DevToolsFrontendHostImpl::BadMessageRecieved() {
  bad_message::ReceivedBadMessage(web_contents_->GetMainFrame()->GetProcess(),
                                  bad_message::DFH_BAD_EMBEDDER_MESSAGE);
}

void DevToolsFrontendHostImpl::DispatchEmbedderMessage(
    const std::string& message) {
  handle_message_callback_.Run(message);
}

}  // namespace content
