// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/tether_component.h"

namespace chromeos {

namespace tether {

TetherComponent::TetherComponent() {}

TetherComponent::~TetherComponent() {}

void TetherComponent::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void TetherComponent::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void TetherComponent::TransitionToStatus(Status new_status) {
  if (status_ == new_status)
    return;

  status_ = new_status;

  if (status_ != Status::SHUT_DOWN)
    return;

  for (auto& observer : observer_list_)
    observer.OnShutdownComplete();
}

}  // namespace tether

}  // namespace chromeos
