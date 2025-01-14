// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/renderer_main_platform_delegate.h"

#include <errno.h>
#include <sys/stat.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/sandbox_init.h"
#include "services/service_manager/sandbox/sandbox.h"

namespace content {

RendererMainPlatformDelegate::RendererMainPlatformDelegate(
    const MainFunctionParams& parameters) {}

RendererMainPlatformDelegate::~RendererMainPlatformDelegate() {
}

void RendererMainPlatformDelegate::PlatformInitialize() {
}

void RendererMainPlatformDelegate::PlatformUninitialize() {
}

bool RendererMainPlatformDelegate::EnableSandbox() {
  // The setuid sandbox is started in the zygote process: zygote_main_linux.cc
  // https://chromium.googlesource.com/chromium/src/+/master/docs/linux_suid_sandbox.md
  //
  // Anything else is started in InitializeSandbox().
  service_manager::SandboxLinux::Options options;
  options.has_wasm_trap_handler =
      base::FeatureList::IsEnabled(features::kWebAssemblyTrapHandler);
  service_manager::Sandbox::Initialize(
      service_manager::SandboxTypeFromCommandLine(
          *base::CommandLine::ForCurrentProcess()),
      service_manager::SandboxLinux::PreSandboxHook(), options);

  // about:sandbox uses a value returned from SandboxLinux::GetStatus() before
  // any renderer has been started.
  // Here, we test that the status of SeccompBpf in the renderer is consistent
  // with what SandboxLinux::GetStatus() said we would do.
  auto* linux_sandbox = service_manager::SandboxLinux::GetInstance();
  if (linux_sandbox->GetStatus() & service_manager::Sandbox::kSeccompBPF) {
    CHECK(linux_sandbox->seccomp_bpf_started());
  }

  // Under the setuid sandbox, we should not be able to open any file via the
  // filesystem.
  if (linux_sandbox->GetStatus() & service_manager::Sandbox::kSUID) {
    CHECK(!base::PathExists(base::FilePath("/proc/cpuinfo")));
  }

#if defined(__x86_64__)
  // Limit this test to architectures where seccomp BPF is active in renderers.
  if (linux_sandbox->seccomp_bpf_started()) {
    errno = 0;
    // This should normally return EBADF since the first argument is bogus,
    // but we know that under the seccomp-bpf sandbox, this should return EPERM.
    CHECK_EQ(fchmod(-1, 07777), -1);
    CHECK_EQ(errno, EPERM);
  }
#endif  // __x86_64__

  return true;
}

}  // namespace content
