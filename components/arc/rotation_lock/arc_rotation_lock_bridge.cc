// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/rotation_lock/arc_rotation_lock_bridge.h"

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/memory/singleton.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace arc {

namespace {

// Singleton factory for ArcRotationLockBridge.
class ArcRotationLockBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcRotationLockBridge,
          ArcRotationLockBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcRotationLockBridgeFactory";

  static ArcRotationLockBridgeFactory* GetInstance() {
    return base::Singleton<ArcRotationLockBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcRotationLockBridgeFactory>;
  ArcRotationLockBridgeFactory() = default;
  ~ArcRotationLockBridgeFactory() override = default;
};

}  // namespace

// static
ArcRotationLockBridge* ArcRotationLockBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcRotationLockBridgeFactory::GetForBrowserContext(context);
}

ArcRotationLockBridge::ArcRotationLockBridge(content::BrowserContext* context,
                                             ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service) {
  arc_bridge_service_->rotation_lock()->AddObserver(this);
  // TODO(mash): Support this functionality without ash::Shell access in Chrome.
  if (ash::Shell::HasInstance()) {
    ash::Shell::Get()->screen_orientation_controller()->AddObserver(this);
    ash::Shell::Get()->tablet_mode_controller()->AddObserver(this);
  }
}

ArcRotationLockBridge::~ArcRotationLockBridge() {
  arc_bridge_service_->rotation_lock()->RemoveObserver(this);
  // TODO(mus): mus needs proper shutdown process.
  // TODO(mash): Support this functionality without ash::Shell access in Chrome.
  if (ash::Shell::HasInstance()) {
    ash::Shell::Get()->screen_orientation_controller()->RemoveObserver(this);
    ash::Shell::Get()->tablet_mode_controller()->RemoveObserver(this);
  }
}

void ArcRotationLockBridge::OnInstanceReady() {
  SendRotationLockState();
}

void ArcRotationLockBridge::OnUserRotationLockChanged() {
  SendRotationLockState();
}

void ArcRotationLockBridge::OnTabletModeStarted() {
  SendRotationLockState();
}

void ArcRotationLockBridge::OnTabletModeEnded() {
  SendRotationLockState();
}

void ArcRotationLockBridge::SendRotationLockState() {
  // TODO(mash): Support this functionality without ash::Shell access in Chrome.
  if (!ash::Shell::HasInstance())
    return;

  mojom::RotationLockInstance* rotation_lock_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->rotation_lock(),
                                  OnRotationLockStateChanged);
  if (!rotation_lock_instance)
    return;

  display::Display current_display;
  if (display::Display::HasInternalDisplay()) {
    bool found = display::Screen::GetScreen()->GetDisplayWithDisplayId(
        display::Display::InternalDisplayId(), &current_display);
    DCHECK(found);
  }

  bool in_tablet_mode = ash::Shell::Get()
                            ->tablet_mode_controller()
                            ->IsTabletModeWindowManagerEnabled();
  bool accelerometer_active = in_tablet_mode
                                  ? !ash::Shell::Get()
                                         ->screen_orientation_controller()
                                         ->rotation_locked()
                                  : false;

  rotation_lock_instance->OnRotationLockStateChanged(
      accelerometer_active,
      static_cast<arc::mojom::Rotation>(current_display.rotation()));
}

}  // namespace arc
