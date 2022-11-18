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

#pragma once

#include "CSSValue.h"
#include <wtf/Ref.h>

namespace WebCore {

// FIXME: This is only used for legacy border image width that sometimes overrides border widths.
class CSSBorderImageWidthValue final : public CSSValue {
public:
    static Ref<CSSBorderImageWidthValue> create(Ref<CSSValue> widths);

    String customCSSText() const;
    bool equals(const CSSBorderImageWidthValue&) const;

    const Ref<CSSValue>& widths() const { return m_widths; }
    bool overridesBorderWidths() const { return overridesBorderWidths(m_widths); }

    static bool overridesBorderWidths(CSSValue&);

private:
    CSSBorderImageWidthValue(Ref<CSSValue>&& widths);

    Ref<CSSValue> m_widths;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(CSSBorderImageWidthValue, isBorderImageWidthValue())
