// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/child_histogram_fetcher_impl.h"

#include <ctype.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_delta_serialization.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/single_thread_task_runner.h"
#include "content/child/child_process.h"
#include "content/common/child_process_messages.h"
#include "ipc/ipc_sender.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace content {

ChildHistogramFetcherFactoryImpl::ChildHistogramFetcherFactoryImpl() {}

ChildHistogramFetcherFactoryImpl::~ChildHistogramFetcherFactoryImpl() {}

void ChildHistogramFetcherFactoryImpl::Create(
    content::mojom::ChildHistogramFetcherFactoryRequest request) {
  mojo::MakeStrongBinding(std::make_unique<ChildHistogramFetcherFactoryImpl>(),
                          std::move(request));
}

void ChildHistogramFetcherFactoryImpl::CreateFetcher(
    mojo::ScopedSharedBufferHandle buffer_handle,
    content::mojom::ChildHistogramFetcherRequest request) {
  base::SharedMemoryHandle memory_handle;
  size_t memory_size;
  const MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(buffer_handle), &memory_handle, &memory_size, nullptr);

  if (result == MOJO_RESULT_OK) {
    // This message must be received only once. Multiple calls to create a
    // global allocator will cause a CHECK() failure.
    base::GlobalHistogramAllocator::CreateWithSharedMemoryHandle(memory_handle,
                                                                 memory_size);
  }

  base::PersistentHistogramAllocator* global_allocator =
      base::GlobalHistogramAllocator::Get();
  if (global_allocator)
    global_allocator->CreateTrackingHistograms(global_allocator->Name());

  content::mojom::ChildHistogramFetcherPtr child_histogram_interface;
  mojo::MakeStrongBinding(std::make_unique<ChildHistogramFetcherImpl>(),
                          std::move(request));
}

ChildHistogramFetcherImpl::ChildHistogramFetcherImpl() {}

ChildHistogramFetcherImpl::~ChildHistogramFetcherImpl() {}

// Extract snapshot data and then send it off to the Browser process.
// Send only a delta to what we have already sent.
void ChildHistogramFetcherImpl::GetChildNonPersistentHistogramData(
    HistogramDataCallback callback) {
  // If a persistent allocator is in use, it needs to occasionally update
  // some internal histograms. An upload is happening so this is a good time.
  base::PersistentHistogramAllocator* global_allocator =
      base::GlobalHistogramAllocator::Get();
  if (global_allocator)
    global_allocator->UpdateTrackingHistograms();

  if (!histogram_delta_serialization_) {
    histogram_delta_serialization_.reset(
        new base::HistogramDeltaSerialization("ChildProcess"));
  }

  std::vector<std::string> deltas;
  // "false" to PerpareAndSerializeDeltas() indicates to *not* include
  // histograms held in persistent storage on the assumption that they will be
  // visible to the recipient through other means.
  histogram_delta_serialization_->PrepareAndSerializeDeltas(&deltas, false);

  std::move(callback).Run(deltas);

#ifndef NDEBUG
  static int count = 0;
  count++;
  LOCAL_HISTOGRAM_COUNTS("Histogram.ChildProcessHistogramSentCount", count);
#endif
}

}  // namespace content
