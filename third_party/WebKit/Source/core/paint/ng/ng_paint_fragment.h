// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ng_paint_fragment_h
#define ng_paint_fragment_h

#include "core/layout/ng/ng_physical_fragment.h"
#include "core/loader/resource/ImageResourceObserver.h"
#include "platform/graphics/paint/DisplayItemClient.h"
#include "platform/scroll/ScrollTypes.h"

namespace blink {

// The NGPaintFragment contains a NGPhysicalFragment and geometry in the paint
// coordinate system.
//
// NGPhysicalFragment is limited to its parent coordinate system for caching and
// sharing subtree. This class makes it possible to compute visual rects in the
// parent transform node.
//
// NGPaintFragment is an ImageResourceObserver, which means that it gets
// notified when associated images are changed.
// This is used for 2 main use cases:
// - reply to 'background-image' as we need to invalidate the background in this
//   case.
//   (See https://drafts.csswg.org/css-backgrounds-3/#the-background-image)
// - image (<img>, svg <image>) or video (<video>) elements that are
//   placeholders for displaying them.
class NGPaintFragment : public DisplayItemClient, public ImageResourceObserver {
 public:
  explicit NGPaintFragment(scoped_refptr<const NGPhysicalFragment>,
                           bool stop_at_block_layout_root = false);

  const NGPhysicalFragment& PhysicalFragment() const {
    return *physical_fragment_;
  }

  const Vector<std::unique_ptr<NGPaintFragment>>& Children() const {
    return children_;
  }

  // Update VisualRect() for this object and all its descendants from
  // LayoutObject. Corresponding LayoutObject::VisualRect() must be computed and
  // set beforehand.
  void UpdateVisualRectFromLayoutObject();

  // Collects rectangles that the outline of this object would be drawing along
  // the outside of, even if the object isn't styled with a outline for now. The
  // rects also cover continuations.
  void AddOutlineRects(Vector<LayoutRect>*, const LayoutPoint& offset) const;

  // TODO(layout-dev): Implement when we have oveflow support.
  // TODO(eae): Switch to using NG geometry types.
  bool HasOverflowClip() const { return false; }
  bool ShouldClipOverflow() const { return false; }
  bool HasSelfPaintingLayer() const { return false; }
  LayoutRect VisualRect() const override { return visual_rect_; }
  LayoutRect VisualOverflowRect() const {
    return {LayoutPoint(), VisualRect().Size()};
  }
  LayoutRect OverflowClipRect(const LayoutPoint& location,
                              OverlayScrollbarClipBehavior) const {
    return {location, VisualRect().Size()};
  }

  // DisplayItemClient methods.
  String DebugName() const override { return "NGPaintFragment"; }

  // Commonly used functions for NGPhysicalFragment.
  Node* GetNode() const { return PhysicalFragment().GetNode(); }
  LayoutObject* GetLayoutObject() const {
    return PhysicalFragment().GetLayoutObject();
  }
  const ComputedStyle& Style() const { return PhysicalFragment().Style(); }
  NGPhysicalOffset Offset() const { return PhysicalFragment().Offset(); }
  NGPhysicalSize Size() const { return PhysicalFragment().Size(); }

 private:
  void SetVisualRect(const LayoutRect& rect) { visual_rect_ = rect; }

  void PopulateDescendants(bool stop_at_block_layout_root);

  // A context object used for UpdateVisualObject() and PopulateDescendants().
  struct UpdateContext {
    const LayoutObject* parent_box;
    const NGPhysicalOffset offset_to_parent_box;
  };
  void UpdateVisualRectFromLayoutObject(const UpdateContext&);

  scoped_refptr<const NGPhysicalFragment> physical_fragment_;
  LayoutRect visual_rect_;

  Vector<std::unique_ptr<NGPaintFragment>> children_;
};

}  // namespace blink

#endif  // ng_paint_fragment_h
