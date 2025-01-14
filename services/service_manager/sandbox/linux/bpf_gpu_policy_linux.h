// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_GPU_POLICY_LINUX_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_GPU_POLICY_LINUX_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "sandbox/linux/syscall_broker/broker_process.h"
#include "services/service_manager/sandbox/export.h"
#include "services/service_manager/sandbox/linux/bpf_base_policy_linux.h"

namespace service_manager {

class SERVICE_MANAGER_SANDBOX_EXPORT GpuProcessPolicy : public BPFBasePolicy {
 public:
  GpuProcessPolicy();
  ~GpuProcessPolicy() override;

  sandbox::bpf_dsl::ResultExpr EvaluateSyscall(
      int system_call_number) const override;

  std::unique_ptr<BPFBasePolicy> GetBrokerSandboxPolicy() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuProcessPolicy);
};

class SERVICE_MANAGER_SANDBOX_EXPORT GpuBrokerProcessPolicy
    : public GpuProcessPolicy {
 public:
  GpuBrokerProcessPolicy();
  ~GpuBrokerProcessPolicy() override;

  sandbox::bpf_dsl::ResultExpr EvaluateSyscall(
      int system_call_number) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuBrokerProcessPolicy);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_LINUX_BPF_GPU_POLICY_LINUX_H_
