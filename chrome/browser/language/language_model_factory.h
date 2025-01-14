// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H_
#define CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace language {
class LanguageModel;
}

// Manages the language model for each profile. The particular language model
// provided depends on feature flags.
class LanguageModelFactory : public BrowserContextKeyedServiceFactory {
 public:
  static LanguageModelFactory* GetInstance();
  static language::LanguageModel* GetForBrowserContext(
      content::BrowserContext* browser_context);

 private:
  friend struct base::DefaultSingletonTraits<LanguageModelFactory>;

  LanguageModelFactory();
  ~LanguageModelFactory() override;

  // BrowserContextKeyedServiceFactory overrides.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(LanguageModelFactory);
};

#endif  // CHROME_BROWSER_LANGUAGE_LANGUAGE_MODEL_FACTORY_H_
