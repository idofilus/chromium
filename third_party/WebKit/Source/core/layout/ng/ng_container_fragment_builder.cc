// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_container_fragment_builder.h"

#include "core/layout/ng/ng_exclusion_space.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "core/layout/ng/ng_unpositioned_float.h"
#include "core/style/ComputedStyle.h"

namespace blink {

NGContainerFragmentBuilder::NGContainerFragmentBuilder(
    scoped_refptr<const ComputedStyle> style,
    NGWritingMode writing_mode,
    TextDirection direction)
    : NGBaseFragmentBuilder(std::move(style), writing_mode, direction) {}

NGContainerFragmentBuilder::~NGContainerFragmentBuilder() {}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::SetInlineSize(
    LayoutUnit inline_size) {
  DCHECK_GE(inline_size, LayoutUnit());
  inline_size_ = inline_size;
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::SetBfcOffset(
    const NGBfcOffset& bfc_offset) {
  bfc_offset_ = bfc_offset;
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::SetEndMarginStrut(
    const NGMarginStrut& end_margin_strut) {
  end_margin_strut_ = end_margin_strut;
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::SetExclusionSpace(
    std::unique_ptr<const NGExclusionSpace> exclusion_space) {
  exclusion_space_ = std::move(exclusion_space);
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::SwapUnpositionedFloats(
    Vector<scoped_refptr<NGUnpositionedFloat>>* unpositioned_floats) {
  unpositioned_floats_.swap(*unpositioned_floats);
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::AddChild(
    scoped_refptr<NGLayoutResult> child,
    const NGLogicalOffset& child_offset) {
  // Collect the child's out of flow descendants.
  // child_offset is offset of inline_start/block_start vertex.
  // Candidates need offset of top/left vertex.
  const auto& ouf_of_flow_descendants = child->OutOfFlowPositionedDescendants();
  if (!ouf_of_flow_descendants.IsEmpty()) {
    NGLogicalOffset top_left_offset;
    NGPhysicalSize child_size = child->PhysicalFragment()->Size();
    switch (WritingMode()) {
      case kHorizontalTopBottom:
        top_left_offset =
            (IsRtl(Direction()))
                ? NGLogicalOffset{child_offset.inline_offset + child_size.width,
                                  child_offset.block_offset}
                : child_offset;
        break;
      case kVerticalRightLeft:
      case kSidewaysRightLeft:
        top_left_offset =
            (IsRtl(Direction()))
                ? NGLogicalOffset{child_offset.inline_offset +
                                      child_size.height,
                                  child_offset.block_offset + child_size.width}
                : NGLogicalOffset{child_offset.inline_offset,
                                  child_offset.block_offset + child_size.width};
        break;
      case kVerticalLeftRight:
      case kSidewaysLeftRight:
        top_left_offset = (IsRtl(Direction()))
                              ? NGLogicalOffset{child_offset.inline_offset +
                                                    child_size.height,
                                                child_offset.block_offset}
                              : child_offset;
        break;
    }
    for (const NGOutOfFlowPositionedDescendant& descendant :
         ouf_of_flow_descendants) {
      oof_positioned_candidates_.push_back(
          NGOutOfFlowPositionedCandidate{descendant, top_left_offset});
    }
  }

  return AddChild(child->PhysicalFragment(), child_offset);
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::AddChild(
    scoped_refptr<NGPhysicalFragment> child,
    const NGLogicalOffset& child_offset) {
  children_.push_back(std::move(child));
  offsets_.push_back(child_offset);
  return *this;
}

NGContainerFragmentBuilder&
NGContainerFragmentBuilder::AddOutOfFlowChildCandidate(
    NGBlockNode child,
    const NGLogicalOffset& child_offset) {
  DCHECK(child);
  oof_positioned_candidates_.push_back(NGOutOfFlowPositionedCandidate(
      NGOutOfFlowPositionedDescendant{
          child, NGStaticPosition::Create(WritingMode(), Direction(),
                                          NGPhysicalOffset())},
      child_offset));

  child.SaveStaticOffsetForLegacy(child_offset);
  return *this;
}

NGContainerFragmentBuilder&
NGContainerFragmentBuilder::AddInlineOutOfFlowChildCandidate(
    NGBlockNode child,
    const NGLogicalOffset& child_offset,
    TextDirection line_direction) {
  DCHECK(child);
  oof_positioned_candidates_.push_back(NGOutOfFlowPositionedCandidate(
      NGOutOfFlowPositionedDescendant{
          child, NGStaticPosition::Create(WritingMode(), line_direction,
                                          NGPhysicalOffset())},
      child_offset, line_direction));

  child.SaveStaticOffsetForLegacy(child_offset);
  return *this;
}

NGContainerFragmentBuilder& NGContainerFragmentBuilder::AddOutOfFlowDescendant(
    NGOutOfFlowPositionedDescendant descendant) {
  oof_positioned_descendants_.push_back(descendant);
  return *this;
}

void NGContainerFragmentBuilder::GetAndClearOutOfFlowDescendantCandidates(
    Vector<NGOutOfFlowPositionedDescendant>* descendant_candidates) {
  DCHECK(descendant_candidates->IsEmpty());

  descendant_candidates->ReserveCapacity(oof_positioned_candidates_.size());

  DCHECK_GE(inline_size_, LayoutUnit());
  DCHECK_GE(block_size_, LayoutUnit());
  NGPhysicalSize builder_physical_size{Size().ConvertToPhysical(WritingMode())};

  for (NGOutOfFlowPositionedCandidate& candidate : oof_positioned_candidates_) {
    TextDirection direction =
        candidate.is_line_relative ? candidate.line_direction : Direction();
    NGPhysicalOffset child_offset = candidate.child_offset.ConvertToPhysical(
        WritingMode(), direction, builder_physical_size, NGPhysicalSize());

    NGStaticPosition builder_relative_position;
    builder_relative_position.type = candidate.descendant.static_position.type;
    builder_relative_position.offset =
        child_offset + candidate.descendant.static_position.offset;

    descendant_candidates->push_back(NGOutOfFlowPositionedDescendant{
        candidate.descendant.node, builder_relative_position});
  }

  // Clear our current canidate list. This may get modified again if the
  // current fragment is a containing block, and AddChild is called with a
  // descendant from this list.
  //
  // The descendant may be a "position: absolute" which contains a "position:
  // fixed" for example. (This fragment isn't the containing block for the
  // fixed descendant).
  oof_positioned_candidates_.clear();
}

}  // namespace blink
