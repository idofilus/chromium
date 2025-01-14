// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/callback_function.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off

#ifndef V8StringSequenceCallbackFunctionLongSequenceArg_h
#define V8StringSequenceCallbackFunctionLongSequenceArg_h

#include "bindings/core/v8/NativeValueTraits.h"
#include "core/CoreExport.h"
#include "platform/bindings/CallbackFunctionBase.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class CORE_EXPORT V8StringSequenceCallbackFunctionLongSequenceArg final : public CallbackFunctionBase {
 public:
  static V8StringSequenceCallbackFunctionLongSequenceArg* Create(ScriptState*, v8::Local<v8::Value> callback);

  ~V8StringSequenceCallbackFunctionLongSequenceArg() = default;

  bool call(ScriptWrappable* scriptWrappable, const Vector<int32_t>& arg, Vector<String>& returnValue);

 private:
  V8StringSequenceCallbackFunctionLongSequenceArg(ScriptState*, v8::Local<v8::Function>);
};

}  // namespace blink

#endif  // V8StringSequenceCallbackFunctionLongSequenceArg_h
