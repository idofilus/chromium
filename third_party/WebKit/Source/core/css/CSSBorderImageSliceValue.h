/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSBorderImageSliceValue_h
#define CSSBorderImageSliceValue_h

#include "core/css/CSSQuadValue.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class CSSBorderImageSliceValue : public CSSValue {
 public:
  static CSSBorderImageSliceValue* Create(CSSQuadValue* slices, bool fill) {
    return new CSSBorderImageSliceValue(slices, fill);
  }

  String CustomCSSText() const;

  // TODO(sashab): Change this to a quad of CSSPrimitiveValues, or add separate
  // methods for topSlice(), leftSlice(), etc.
  const CSSQuadValue& Slices() const { return *slices_; }
  bool Fill() const { return fill_; }

  bool Equals(const CSSBorderImageSliceValue&) const;

  void TraceAfterDispatch(blink::Visitor*);

 private:
  CSSBorderImageSliceValue(CSSQuadValue* slices, bool fill);

  // These four values are used to make "cuts" in the border image. They can be
  // numbers or percentages.
  Member<CSSQuadValue> slices_;
  bool fill_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSBorderImageSliceValue,
                            IsBorderImageSliceValue());

}  // namespace blink

#endif  // CSSBorderImageSliceValue_h
