/*
 * Copyright (C) 2022 Igalia S.L. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CSSBorderImageWidthValue.h"

#include "Rect.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

CSSBorderImageWidthValue::CSSBorderImageWidthValue(Ref<CSSValue>&& widths)
    : CSSValue(BorderImageWidthClass)
    , m_widths(WTFMove(widths))
{
}

Ref<CSSBorderImageWidthValue> CSSBorderImageWidthValue::create(Ref<CSSValue> widths)
{
    return adoptRef(*new CSSBorderImageWidthValue(WTFMove(widths)));
}

String CSSBorderImageWidthValue::customCSSText() const
{
    // border-image-width can't set m_overridesBorderWidths to true by itself, so serialize as empty string.
    // It can only be true via the -webkit-border-image shorthand, whose serialization will unwrap widths() if needed.
    return emptyString();
}

bool CSSBorderImageWidthValue::equals(const CSSBorderImageWidthValue& other) const
{
    return compareCSSValue(m_widths, other.m_widths);
}

bool CSSBorderImageWidthValue::overridesBorderWidths(CSSValue& widths)
{
    if (auto* primitiveValue = dynamicDowncast<CSSPrimitiveValue>(widths))
        return primitiveValue->isLength();
    for (auto& width : downcast<CSSValueList>(widths)) {
        if (!downcast<CSSPrimitiveValue>(width.get()).isLength())
            return false;
    }
    return true;
}

} // namespace WebCore
