// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/properties/shorthands/GridTemplate.h"

#include "core/css/parser/CSSParserContext.h"
#include "core/css/parser/CSSPropertyParserHelpers.h"
#include "core/css/properties/CSSPropertyGridUtils.h"
#include "core/layout/LayoutObject.h"

namespace blink {
namespace CSSShorthand {

bool GridTemplate::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* template_rows = nullptr;
  CSSValue* template_columns = nullptr;
  CSSValue* template_areas = nullptr;
  if (!CSSPropertyGridUtils::ConsumeGridTemplateShorthand(
          important, range, context, template_rows, template_columns,
          template_areas))
    return false;

  DCHECK(template_rows);
  DCHECK(template_columns);
  DCHECK(template_areas);

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyGridTemplateRows, CSSPropertyGridTemplate, *template_rows,
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyGridTemplateColumns, CSSPropertyGridTemplate,
      *template_columns, important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyGridTemplateAreas, CSSPropertyGridTemplate, *template_areas,
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);

  return true;
}

bool GridTemplate::IsLayoutDependent(const ComputedStyle* style,
                                     LayoutObject* layout_object) const {
  return layout_object && layout_object->IsLayoutGrid();
}

}  // namespace CSSShorthand
}  // namespace blink
