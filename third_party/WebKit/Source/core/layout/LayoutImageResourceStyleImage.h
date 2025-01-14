/*
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 1999 Antti Koivisto <koivisto@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <kde@carewolf.com>
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc.
 *               All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef LayoutImageResourceStyleImage_h
#define LayoutImageResourceStyleImage_h

#include "core/layout/LayoutImageResource.h"
#include "core/style/StyleImage.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

class LayoutObject;

class LayoutImageResourceStyleImage final : public LayoutImageResource {
 public:
  ~LayoutImageResourceStyleImage() override;

  static LayoutImageResource* Create(StyleImage* style_image) {
    return new LayoutImageResourceStyleImage(style_image);
  }
  void Initialize(LayoutObject*) override;
  void Shutdown() override;

  bool HasImage() const override { return true; }
  scoped_refptr<Image> GetImage(const IntSize&) const override;
  bool ErrorOccurred() const override { return style_image_->ErrorOccurred(); }

  bool ImageHasRelativeSize() const override {
    return style_image_->ImageHasRelativeSize();
  }
  LayoutSize ImageSize(float multiplier) const override;
  WrappedImagePtr ImagePtr() const override { return style_image_->Data(); }

  void Trace(blink::Visitor*) override;

 private:
  explicit LayoutImageResourceStyleImage(StyleImage*);
  Member<StyleImage> style_image_;
};

}  // namespace blink

#endif  // LayoutImageStyleImage_h
