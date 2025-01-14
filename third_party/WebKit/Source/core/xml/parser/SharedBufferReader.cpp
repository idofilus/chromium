/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/xml/parser/SharedBufferReader.h"

#include "platform/SharedBuffer.h"
#include "platform/wtf/RefPtr.h"

#include <algorithm>
#include <cstring>

namespace blink {

SharedBufferReader::SharedBufferReader(scoped_refptr<const SharedBuffer> buffer)
    : buffer_(std::move(buffer)), current_offset_(0) {}

SharedBufferReader::~SharedBufferReader() {}

int SharedBufferReader::ReadData(char* output_buffer, int asked_to_read) {
  if (!buffer_ || current_offset_ > buffer_->size())
    return 0;

  size_t bytes_copied = 0;
  size_t bytes_left = buffer_->size() - current_offset_;
  size_t len_to_copy = std::min(SafeCast<size_t>(asked_to_read), bytes_left);

  while (bytes_copied < len_to_copy) {
    const char* data;
    size_t segment_size = buffer_->GetSomeData(data, current_offset_);
    if (!segment_size)
      break;

    segment_size = std::min(segment_size, len_to_copy - bytes_copied);
    memcpy(output_buffer + bytes_copied, data, segment_size);
    bytes_copied += segment_size;
    current_offset_ += segment_size;
  }

  return SafeCast<int>(bytes_copied);
}

}  // namespace blink
