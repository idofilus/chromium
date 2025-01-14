// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_CHROMEOS_TEST_TOUCH_DEVICE_MANAGER_TEST_API_H_
#define UI_DISPLAY_MANAGER_CHROMEOS_TEST_TOUCH_DEVICE_MANAGER_TEST_API_H_

#include "base/macros.h"
#include "ui/display/manager/chromeos/touch_device_manager.h"
#include "ui/display/manager/display_manager_export.h"

namespace ui {
struct TouchscreenDevice;
}  // namespace ui

namespace display {
class ManagedDisplayInfo;

namespace test {

class DISPLAY_MANAGER_EXPORT TouchDeviceManagerTestApi {
 public:
  explicit TouchDeviceManagerTestApi(TouchDeviceManager* touch_device_manager);
  ~TouchDeviceManagerTestApi();

  // Associate the given display |display_info| with the touch device |device|.
  void Associate(ManagedDisplayInfo* display_info,
                 const ui::TouchscreenDevice& device);

  // Returns the count of touch devices currently asosicated with the display
  // |info|.
  std::size_t GetTouchDeviceCount(const ManagedDisplayInfo& info) const;

  // Returns true if the display |info| and touch device |device| are currenlty
  // associated.
  bool AreAssociated(const ManagedDisplayInfo& info,
                     const ui::TouchscreenDevice& device) const;

  // Resets the touch device manager by clearing all records of historical
  // touch association and calibration data.
  void ResetTouchDeviceManager();

 private:
  // Not owned
  TouchDeviceManager* touch_device_manager_;

  DISALLOW_COPY_AND_ASSIGN(TouchDeviceManagerTestApi);
};

}  // namespace test
}  // namespace display
#endif  // UI_DISPLAY_MANAGER_CHROMEOS_TEST_TOUCH_DEVICE_MANAGER_TEST_API_H_
