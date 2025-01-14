/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
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
 */

#ifndef CSSValueList_h
#define CSSValueList_h

#include "core/CoreExport.h"
#include "core/css/CSSValue.h"
#include "platform/wtf/Vector.h"

namespace blink {

class CORE_EXPORT CSSValueList : public CSSValue {
  WTF_MAKE_NONCOPYABLE(CSSValueList);

 public:
  using iterator = HeapVector<Member<const CSSValue>, 4>::iterator;
  using const_iterator = HeapVector<Member<const CSSValue>, 4>::const_iterator;

  static CSSValueList* CreateCommaSeparated() {
    return new CSSValueList(kCommaSeparator);
  }
  static CSSValueList* CreateSpaceSeparated() {
    return new CSSValueList(kSpaceSeparator);
  }
  static CSSValueList* CreateSlashSeparated() {
    return new CSSValueList(kSlashSeparator);
  }

  iterator begin() { return values_.begin(); }
  iterator end() { return values_.end(); }
  const_iterator begin() const { return values_.begin(); }
  const_iterator end() const { return values_.end(); }

  size_t length() const { return values_.size(); }
  const CSSValue& Item(size_t index) const { return *values_[index]; }

  void Append(const CSSValue& value) { values_.push_back(value); }
  bool RemoveAll(const CSSValue&);
  bool HasValue(const CSSValue&) const;
  CSSValueList* Copy() const;

  String CustomCSSText() const;
  bool Equals(const CSSValueList&) const;

  bool HasFailedOrCanceledSubresources() const;

  bool MayContainUrl() const;
  void ReResolveUrl(const Document&) const;

  void TraceAfterDispatch(blink::Visitor*);

 protected:
  CSSValueList(ClassType, ValueListSeparator);

 private:
  explicit CSSValueList(ValueListSeparator);

  HeapVector<Member<const CSSValue>, 4> values_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSValueList, IsValueList());

}  // namespace blink

#endif  // CSSValueList_h
