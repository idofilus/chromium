// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_REGISTRY_INTERFACE_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_REGISTRY_INTERFACE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/watcher.h"

namespace chromeos {
namespace file_system_provider {

// Remembers and restores file systems in a persistent storage.
class RegistryInterface {
 public:
  struct RestoredFileSystem;

  // List of file systems together with their watchers to be remounted.
  typedef std::vector<RestoredFileSystem> RestoredFileSystems;

  // Information about a file system to be restored.
  struct RestoredFileSystem {
    RestoredFileSystem();
    RestoredFileSystem(const RestoredFileSystem& other);
    ~RestoredFileSystem();

    std::string provider_id;
    MountOptions options;
    Watchers watchers;
  };

  virtual ~RegistryInterface();

  // Remembers the file system in preferences, in order to remount after a
  // reboot.
  virtual void RememberFileSystem(
      const ProvidedFileSystemInfo& file_system_info,
      const Watchers& watchers) = 0;

  // Removes the file system from preferences, so it is not remounmted anymore
  // after a reboot.
  virtual void ForgetFileSystem(const std::string& provider_id,
                                const std::string& file_system_id) = 0;

  // Restores from preferences file systems mounted previously by the
  // |provider_id| file system provider. The returned list should be used to
  // remount them.
  virtual std::unique_ptr<RestoredFileSystems> RestoreFileSystems(
      const std::string& provider_id) = 0;

  // Updates a tag for the specified watcher.
  virtual void UpdateWatcherTag(const ProvidedFileSystemInfo& file_system_info,
                                const Watcher& watcher) = 0;
};

}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_REGISTRY_INTERFACE_H_
