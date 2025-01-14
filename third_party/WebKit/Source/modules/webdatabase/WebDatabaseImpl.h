// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_MODULES_WEBDATABASE_WEBDATABASEIMPL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_MODULES_WEBDATABASE_WEBDATABASEIMPL_H_

#include <stdint.h>

#include "third_party/WebKit/public/platform/modules/webdatabase/web_database.mojom-blink.h"

namespace blink {

// Receives database messages from the browser process and processes them on the
// IO thread.
class WebDatabaseImpl : public mojom::blink::WebDatabase {
 public:
  WebDatabaseImpl();
  ~WebDatabaseImpl() override;

  static void Create(mojom::blink::WebDatabaseRequest);

 private:
  // blink::mojom::blink::Database
  void UpdateSize(const scoped_refptr<SecurityOrigin>&,
                  const String& name,
                  int64_t size) override;
  void CloseImmediately(const scoped_refptr<SecurityOrigin>&,
                        const String& name) override;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_MODULES_WEBDATABASE_WEBDATABASEIMPL_H_
