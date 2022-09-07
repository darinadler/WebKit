/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2016-2022 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CSSValueKeywords.h"
#include "Color.h"

namespace WebCore {

class StyleKeywordColorResolver;

enum class StyleColorOptions : uint8_t {
    ForVisitedLink = 1 << 0,
    UseSystemAppearance = 1 << 1,
    UseDarkAppearance = 1 << 2,
    UseElevatedUserInterfaceLevel = 1 << 3
};

class StyleColor {
public:
    // https://drafts.csswg.org/css-color-4/#typedef-color
    enum class CSSColorType : uint8_t {
        Absolute = 1 << 0,
        Current = 1 << 1,
        System = 1 << 2,
    };
    static bool isColorKeyword(CSSValueID, OptionSet<CSSColorType> = { CSSColorType::Absolute, CSSColorType::Current, CSSColorType::System });
    static bool isAbsoluteColorKeyword(CSSValueID);
    WEBCORE_EXPORT static bool isSystemColorKeyword(CSSValueID);

    static Color colorFromKeyword(CSSValueID, OptionSet<StyleColorOptions>);

    StyleColor();
    StyleColor(const Color&);
    StyleColor(SRGBA<uint8_t>);
    StyleColor(CSSValueID);
    StyleColor(Function<Color(const Color&)>&&, StyleColor&&);
    StyleColor(Function<Color(const Color&, const Color&)>&&, StyleColor&&, StyleColor&&);
    StyleColor(const StyleColor&);
    StyleColor(StyleColor&&);
    StyleColor& operator=(const StyleColor&);
    StyleColor& operator=(StyleColor&&);
    ~StyleColor();

    bool isCurrentColor() const { return m_color.m_colorAndFlags == (encode(Type::Keyword) | encode(CSSValueCurrentcolor)); }

    bool isAlreadyResolved() const { return type() == Type::Normal; }
    const Color& alreadyResolvedColor() const;

    Color resolvedColor(StyleKeywordColorResolver&) const;

private:
    class OutOfLineStyleColor;
    enum class Type : uint8_t { Normal, Keyword, OutOfLine };

    friend void swap(StyleColor&, StyleColor&);
    friend bool operator==(const StyleColor&, const StyleColor&);
    friend bool operator==(const OutOfLineStyleColor&, const OutOfLineStyleColor&);
    friend void add(Hasher&, const StyleColor&);
    friend WTF::TextStream& operator<<(WTF::TextStream&, const StyleColor&);

    Type type() const { return decodeType(m_color.m_colorAndFlags); }
    CSSValueID keyword() const;

    bool isOutOfLineStyleColor() const { return type() == Type::OutOfLine; }
    OutOfLineStyleColor& outOfLineStyleColor() const;
    void setOutOfLineStyleColor(const StyleColor&);
    void destroyOutOfLineStyleColor();

    static constexpr auto styleColorTypeShift = Color::flagsShift + 6;

    static constexpr uint64_t encode(Type type) { return static_cast<uint64_t>(type) << styleColorTypeShift; }
    static constexpr Type decodeType(uint64_t colorAndFlags) { return static_cast<Type>((colorAndFlags >> styleColorTypeShift) & 0x3); }

    static constexpr uint64_t encode(CSSValueID keyword) { return keyword; }
    static constexpr CSSValueID decodeKeyword(uint64_t colorAndFlags) { return static_cast<CSSValueID>(colorAndFlags & 0xFFFF); }

    Color m_color;
};

bool operator==(const StyleColor&, const StyleColor&);
bool operator!=(const StyleColor&, const StyleColor&);

inline StyleColor::StyleColor()
{
    m_color.m_colorAndFlags = encode(Type::Keyword) | encode(CSSValueCurrentcolor);
}

inline StyleColor::StyleColor(const Color& color)
    : m_color { color }
{
}

inline StyleColor::StyleColor(SRGBA<uint8_t> color)
    : m_color { color }
{
}

inline StyleColor::StyleColor(const StyleColor& color)
{
    if (color.isOutOfLineStyleColor())
        setOutOfLineStyleColor(color);
    else
        m_color = color.m_color;
}

inline StyleColor::StyleColor(StyleColor&& color)
{
    if (color.isOutOfLineStyleColor())
        m_color.m_colorAndFlags = std::exchange(color.m_color.m_colorAndFlags, Color::invalidColorAndFlags);
    else
        m_color = WTFMove(color.m_color);
}

inline StyleColor& StyleColor::operator=(const StyleColor& color)
{
    StyleColor copy { color };
    swap(copy, *this);
    return *this;
}

inline StyleColor& StyleColor::operator=(StyleColor&& color)
{
    StyleColor copy { WTFMove(color) };
    swap(copy, *this);
    return *this;
}

inline StyleColor::~StyleColor()
{
    if (isOutOfLineStyleColor())
        destroyOutOfLineStyleColor();
}

inline void swap(StyleColor& a, StyleColor& b)
{
    swap(a.m_color, b.m_color);
}

inline auto StyleColor::outOfLineStyleColor() const -> OutOfLineStyleColor&
{
    ASSERT(isOutOfLineStyleColor());
    return *static_cast<OutOfLineStyleColor*>(Color::decodedOutOfLinePointer(m_color.m_colorAndFlags));
}

inline CSSValueID StyleColor::keyword() const
{
    ASSERT(type() == Type::Keyword);
    return decodeKeyword(m_color.m_colorAndFlags);
}

inline const Color& StyleColor::alreadyResolvedColor() const
{
    ASSERT(isAlreadyResolved());
    return m_color;
}

inline bool operator==(const StyleColor& a, const StyleColor& b)
{
    auto type = a.type();
    if (type != b.type())
        return false;

    switch (type) {
    case StyleColor::Type::Normal:
        return a.alreadyResolvedColor() == b.alreadyResolvedColor();
    case StyleColor::Type::Keyword:
        return a.keyword() == b.keyword();
    case StyleColor::Type::OutOfLine:
        return a.outOfLineStyleColor() == b.outOfLineStyleColor();
    }
}

inline bool operator!=(const StyleColor& a, const StyleColor& b)
{
    return !(a == b);
}

} // namespace WebCore
