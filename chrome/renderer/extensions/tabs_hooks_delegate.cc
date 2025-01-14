// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/tabs_hooks_delegate.h"

#include "base/strings/stringprintf.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/common/api/messaging/message.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/message_target.h"
#include "extensions/renderer/messaging_util.h"
#include "extensions/renderer/native_renderer_messaging_service.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/converter.h"

namespace extensions {

namespace {

using RequestResult = APIBindingHooks::RequestResult;

constexpr char kConnect[] = "tabs.connect";
constexpr char kSendMessage[] = "tabs.sendMessage";
constexpr char kSendRequest[] = "tabs.sendRequest";

}  // namespace

TabsHooksDelegate::TabsHooksDelegate(
    NativeRendererMessagingService* messaging_service)
    : messaging_service_(messaging_service) {}
TabsHooksDelegate::~TabsHooksDelegate() {}

RequestResult TabsHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  // TODO(devlin): This logic is the same in the RuntimeCustomHooksDelegate -
  // would it make sense to share it?
  using Handler = RequestResult (TabsHooksDelegate::*)(
      ScriptContext*, const std::vector<v8::Local<v8::Value>>&);
  static const struct {
    Handler handler;
    base::StringPiece method;
  } kHandlers[] = {
      {&TabsHooksDelegate::HandleSendMessage, kSendMessage},
      {&TabsHooksDelegate::HandleSendRequest, kSendRequest},
      {&TabsHooksDelegate::HandleConnect, kConnect},
  };

  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);

  Handler handler = nullptr;
  for (const auto& handler_entry : kHandlers) {
    if (handler_entry.method == method_name) {
      handler = handler_entry.handler;
      break;
    }
  }

  if (!handler)
    return RequestResult(RequestResult::NOT_HANDLED);

  std::string error;
  std::vector<v8::Local<v8::Value>> parsed_arguments;
  if (!signature->ParseArgumentsToV8(context, *arguments, refs,
                                     &parsed_arguments, &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  return (this->*handler)(script_context, parsed_arguments);
}

RequestResult TabsHooksDelegate::HandleSendRequest(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(3u, arguments.size());

  int tab_id = messaging_util::ExtractIntegerId(arguments[0]);
  v8::Local<v8::Value> v8_message = arguments[1];
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(script_context->v8_context(), v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = "Illegal argument to tabs.sendRequest for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[2]->IsNull())
    response_callback = arguments[2].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, MessageTarget::ForTab(tab_id, messaging_util::kNoFrameId),
      messaging_util::kSendRequestChannel, false, *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult TabsHooksDelegate::HandleSendMessage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(4u, arguments.size());

  int tab_id = messaging_util::ExtractIntegerId(arguments[0]);
  messaging_util::MessageOptions options;
  if (!arguments[2]->IsNull()) {
    std::string error;
    messaging_util::ParseOptionsResult parse_result =
        messaging_util::ParseMessageOptions(
            script_context->v8_context(), arguments[2].As<v8::Object>(),
            messaging_util::PARSE_FRAME_ID, &options, &error);
    switch (parse_result) {
      case messaging_util::TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case messaging_util::THROWN:
        return RequestResult(RequestResult::THROWN);
      case messaging_util::SUCCESS:
        break;
    }
  }

  v8::Local<v8::Value> v8_message = arguments[1];
  DCHECK(!v8_message.IsEmpty());
  std::unique_ptr<Message> message =
      messaging_util::MessageFromV8(script_context->v8_context(), v8_message);
  if (!message) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = "Illegal argument to tabs.sendMessage for 'message'.";
    return result;
  }

  v8::Local<v8::Function> response_callback;
  if (!arguments[3]->IsNull())
    response_callback = arguments[3].As<v8::Function>();

  messaging_service_->SendOneTimeMessage(
      script_context, MessageTarget::ForTab(tab_id, options.frame_id),
      messaging_util::kSendMessageChannel, false, *message, response_callback);

  return RequestResult(RequestResult::HANDLED);
}

RequestResult TabsHooksDelegate::HandleConnect(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& arguments) {
  DCHECK_EQ(2u, arguments.size());

  int tab_id = messaging_util::ExtractIntegerId(arguments[0]);

  messaging_util::MessageOptions options;
  if (!arguments[1]->IsNull()) {
    std::string error;
    messaging_util::ParseOptionsResult parse_result =
        messaging_util::ParseMessageOptions(
            script_context->v8_context(), arguments[1].As<v8::Object>(),
            messaging_util::PARSE_FRAME_ID | messaging_util::PARSE_CHANNEL_NAME,
            &options, &error);
    switch (parse_result) {
      case messaging_util::TYPE_ERROR: {
        RequestResult result(RequestResult::INVALID_INVOCATION);
        result.error = std::move(error);
        return result;
      }
      case messaging_util::THROWN:
        return RequestResult(RequestResult::THROWN);
      case messaging_util::SUCCESS:
        break;
    }
  }

  gin::Handle<GinPort> port = messaging_service_->Connect(
      script_context, MessageTarget::ForTab(tab_id, options.frame_id),
      options.channel_name, false);
  DCHECK(!port.IsEmpty());

  RequestResult result(RequestResult::HANDLED);
  result.return_value = port.ToV8();
  return result;
}

}  // namespace extensions
