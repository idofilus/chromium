// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_LAYER_DELEGATE_H_
#define UI_COMPOSITOR_LAYER_DELEGATE_H_

#include "ui/compositor/compositor_export.h"
#include "ui/compositor/property_change_reason.h"

namespace gfx {
class Rect;
}

namespace ui {
class PaintContext;

// A delegate interface implemented by an object that renders to a Layer.
class COMPOSITOR_EXPORT LayerDelegate {
 public:
  // Paint content for the layer to the specified context.
  virtual void OnPaintLayer(const PaintContext& context) = 0;

  // A notification that this layer has had a delegated frame swap and
  // will be repainted.
  virtual void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) = 0;

  // Called when the layer's device scale factor has changed.
  virtual void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                          float new_device_scale_factor) = 0;

  // Invoked when the bounds are set. |reason| indicates whether the bounds were
  // set directly or by an animation. This will be called at every step of a
  // bounds animation.
  virtual void OnLayerBoundsChanged(const gfx::Rect& old_bounds,
                                    PropertyChangeReason reason);

  // Invoked when the transform or opacity is set. |reason| indicates whether
  // the property was set directly or by an animation. This will be called
  // before the first frame of an animation is rendered and when the animation
  // ends, but not necessarily at every frame of the animation.
  virtual void OnLayerTransformed(PropertyChangeReason reason);
  virtual void OnLayerOpacityChanged(PropertyChangeReason reason);

 protected:
  virtual ~LayerDelegate() {}
};

}  // namespace ui

#endif  // UI_COMPOSITOR_LAYER_DELEGATE_H_
