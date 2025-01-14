// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/web_task_runner_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "platform/scheduler/base/task_queue.h"
#include "platform/scheduler/base/time_domain.h"
#include "public/platform/WebTraceLocation.h"

namespace blink {
namespace scheduler {

scoped_refptr<WebTaskRunnerImpl> WebTaskRunnerImpl::Create(
    scoped_refptr<TaskQueue> task_queue,
    base::Optional<TaskType> task_type) {
  return base::AdoptRef(
      new WebTaskRunnerImpl(std::move(task_queue), task_type));
}

bool WebTaskRunnerImpl::RunsTasksInCurrentSequence() {
  return task_queue_->RunsTasksInCurrentSequence();
}

double WebTaskRunnerImpl::MonotonicallyIncreasingVirtualTimeSeconds() const {
  return Now().ToInternalValue() /
         static_cast<double>(base::Time::kMicrosecondsPerSecond);
}

WebTaskRunnerImpl::WebTaskRunnerImpl(scoped_refptr<TaskQueue> task_queue,
                                     base::Optional<TaskType> task_type)
    : task_queue_(std::move(task_queue)), task_type_(task_type) {}

WebTaskRunnerImpl::~WebTaskRunnerImpl() {}

base::TimeTicks WebTaskRunnerImpl::Now() const {
  TimeDomain* time_domain = task_queue_->GetTimeDomain();
  // It's possible task_queue_ has been Unregistered which can lead to a null
  // TimeDomain.  If that happens just return the current real time.
  if (!time_domain)
    return base::TimeTicks::Now();
  return time_domain->Now();
}

scoped_refptr<base::SingleThreadTaskRunner>
WebTaskRunnerImpl::ToSingleThreadTaskRunner() {
  return task_queue_.get();
}

bool WebTaskRunnerImpl::PostDelayedTask(const base::Location& location,
                                        base::OnceClosure task,
                                        base::TimeDelta delay) {
  return task_queue_->PostTaskWithMetadata(TaskQueue::PostedTask(
      std::move(task), location, delay, base::Nestable::kNestable, task_type_));
}

}  // namespace scheduler
}  // namespace blink
