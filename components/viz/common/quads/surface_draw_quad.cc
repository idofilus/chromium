// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/surface_draw_quad.h"

#include "base/logging.h"
#include "base/optional.h"
#include "base/trace_event/trace_event_argument.h"
#include "base/values.h"

namespace viz {

SurfaceDrawQuad::SurfaceDrawQuad() = default;

SurfaceDrawQuad::SurfaceDrawQuad(const SurfaceDrawQuad& other) = default;

SurfaceDrawQuad::~SurfaceDrawQuad() = default;

SurfaceDrawQuad& SurfaceDrawQuad::operator=(const SurfaceDrawQuad& other) =
    default;

void SurfaceDrawQuad::SetNew(
    const SharedQuadState* shared_quad_state,
    const gfx::Rect& rect,
    const gfx::Rect& visible_rect,
    const SurfaceId& primary_surface_id,
    const base::Optional<SurfaceId>& fallback_surface_id,
    SkColor default_background_color) {
  bool needs_blending = true;
  DrawQuad::SetAll(shared_quad_state, DrawQuad::SURFACE_CONTENT, rect,
                   visible_rect, needs_blending);
  this->primary_surface_id = primary_surface_id;
  this->fallback_surface_id = fallback_surface_id;
  this->default_background_color = default_background_color;
}

void SurfaceDrawQuad::SetAll(
    const SharedQuadState* shared_quad_state,
    const gfx::Rect& rect,
    const gfx::Rect& visible_rect,
    bool needs_blending,
    const SurfaceId& primary_surface_id,
    const base::Optional<SurfaceId>& fallback_surface_id,
    SkColor default_background_color) {
  DrawQuad::SetAll(shared_quad_state, DrawQuad::SURFACE_CONTENT, rect,
                   visible_rect, needs_blending);
  this->primary_surface_id = primary_surface_id;
  this->fallback_surface_id = fallback_surface_id;
  this->default_background_color = default_background_color;
}

const SurfaceDrawQuad* SurfaceDrawQuad::MaterialCast(const DrawQuad* quad) {
  DCHECK_EQ(quad->material, DrawQuad::SURFACE_CONTENT);
  return static_cast<const SurfaceDrawQuad*>(quad);
}

void SurfaceDrawQuad::ExtendValue(base::trace_event::TracedValue* value) const {
  value->SetString("primary_surface_id", primary_surface_id.ToString());
  if (fallback_surface_id.has_value())
    value->SetString("fallback_surface_id", fallback_surface_id->ToString());
}

}  // namespace viz
