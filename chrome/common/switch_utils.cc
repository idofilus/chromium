// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/switch_utils.h"

#include <stddef.h>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"

#if defined(OS_WIN)
#include "base/strings/string_util.h"
#endif  // defined(OS_WIN)

namespace switches {

namespace {

// Switches enumerated here will be removed when a background instance of
// Chrome restarts itself. If your key is designed to only be used once,
// or if it does not make sense when restarting a background instance to
// pick up an automatic update, be sure to add it to this list.
constexpr const char* kSwitchesToRemoveOnAutorestart[] = {
    switches::kApp,
    switches::kAppId,
    switches::kForceFirstRun,
    switches::kMakeDefaultBrowser,
    switches::kNoStartupWindow,
    switches::kRestoreLastSession,
    switches::kShowAppList,
    switches::kWinJumplistAction};

}  // namespace

void RemoveSwitchesForAutostart(base::CommandLine::SwitchMap* switch_list) {
  for (const char* switch_to_remove : kSwitchesToRemoveOnAutorestart)
    switch_list->erase(switch_to_remove);

#if defined(OS_WIN)
  // The relaunched browser process shouldn't reuse the /prefetch:# switch of
  // the current process because the process type can change (e.g. a process
  // initially launched in background can be relaunched in foreground).
  static const char kPrefetchSwitchPrefix[] = "prefetch:";
  auto it = switch_list->lower_bound(kPrefetchSwitchPrefix);
  if (it != switch_list->end() &&
      base::StartsWith(it->first, kPrefetchSwitchPrefix,
                       base::CompareCase::SENSITIVE)) {
    switch_list->erase(it);
  }
#endif  // defined(OS_WIN)
}

}  // namespace switches
