/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/dom/DocumentParser.h"

#include <memory>
#include "core/dom/Document.h"
#include "core/dom/DocumentParserClient.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "platform/wtf/Assertions.h"

namespace blink {

DocumentParser::DocumentParser(Document* document)
    : state_(kParsingState),
      document_was_loaded_as_part_of_navigation_(false),
      document_(document) {
  DCHECK(document);
}

DocumentParser::~DocumentParser() {}

void DocumentParser::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
  visitor->Trace(clients_);
}

void DocumentParser::SetDecoder(std::unique_ptr<TextResourceDecoder>) {
  NOTREACHED();
}

TextResourceDecoder* DocumentParser::Decoder() {
  return nullptr;
}

void DocumentParser::PrepareToStopParsing() {
  DCHECK_EQ(state_, kParsingState);
  state_ = kStoppingState;
}

void DocumentParser::StopParsing() {
  state_ = kStoppedState;

  // Clients may be removed while in the loop. Make a snapshot for iteration.
  HeapVector<Member<DocumentParserClient>> clients_snapshot;
  CopyToVector(clients_, clients_snapshot);

  for (DocumentParserClient* client : clients_snapshot) {
    if (!clients_.Contains(client))
      continue;

    client->NotifyParserStopped();
  }
}

void DocumentParser::Detach() {
  state_ = kDetachedState;
  document_ = nullptr;
}

void DocumentParser::SuspendScheduledTasks() {}

void DocumentParser::ResumeScheduledTasks() {}

void DocumentParser::AddClient(DocumentParserClient* client) {
  clients_.insert(client);
}

void DocumentParser::RemoveClient(DocumentParserClient* client) {
  clients_.erase(client);
}

}  // namespace blink
