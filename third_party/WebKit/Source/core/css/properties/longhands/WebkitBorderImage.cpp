// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/longhands/WebkitBorderImage.h"

#include "core/css/properties/CSSPropertyBorderImageUtils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WebkitBorderImage::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSPropertyBorderImageUtils::ConsumeWebkitBorderImage(range, context);
}

}  // namespace CSSLonghand
}  // namespace blink
