// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGFragment_h
#define NGFragment_h

#include "core/CoreExport.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "core/layout/ng/ng_writing_mode.h"
#include "platform/LayoutUnit.h"

namespace blink {

struct NGBorderEdges;
struct NGLogicalSize;

class CORE_EXPORT NGFragment {
  STACK_ALLOCATED();

 public:
  NGFragment(NGWritingMode writing_mode,
             const NGPhysicalFragment& physical_fragment)
      : physical_fragment_(physical_fragment), writing_mode_(writing_mode) {}

  NGWritingMode WritingMode() const {
    return static_cast<NGWritingMode>(writing_mode_);
  }

  // Returns the border-box size.
  LayoutUnit InlineSize() const;
  LayoutUnit BlockSize() const;
  NGLogicalSize Size() const;

  NGBorderEdges BorderEdges() const;

  NGPhysicalFragment::NGFragmentType Type() const;

 protected:
  const NGPhysicalFragment& physical_fragment_;

  unsigned writing_mode_ : 3;
};

}  // namespace blink

#endif  // NGFragment_h
