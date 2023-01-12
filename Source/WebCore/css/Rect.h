/*
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#pragma once

#include "CSSPrimitiveValue.h"
#include <wtf/RefPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

class RectBase {
public:
    const CSSPrimitiveValue* top() const { return m_top.get(); }
    const CSSPrimitiveValue* right() const { return m_right.get(); }
    const CSSPrimitiveValue* bottom() const { return m_bottom.get(); }
    const CSSPrimitiveValue* left() const { return m_left.get(); }

    void setTop(RefPtr<const CSSPrimitiveValue>&& top) { m_top = WTFMove(top); }
    void setRight(RefPtr<const CSSPrimitiveValue>&& right) { m_right = WTFMove(right); }
    void setBottom(RefPtr<const CSSPrimitiveValue>&& bottom) { m_bottom = WTFMove(bottom); }
    void setLeft(RefPtr<const CSSPrimitiveValue>&& left) { m_left = WTFMove(left); }

    bool equals(const RectBase& other) const
    {
        return compareCSSValuePtr(m_top, other.m_top)
            && compareCSSValuePtr(m_right, other.m_right)
            && compareCSSValuePtr(m_left, other.m_left)
            && compareCSSValuePtr(m_bottom, other.m_bottom);
    }

protected:
    RectBase() = default;
    ~RectBase() = default;

private:
    RefPtr<const CSSPrimitiveValue> m_top;
    RefPtr<const CSSPrimitiveValue> m_right;
    RefPtr<const CSSPrimitiveValue> m_bottom;
    RefPtr<const CSSPrimitiveValue> m_left;
};

class Rect final : public RectBase, public RefCounted<Rect> {
public:
    static Ref<Rect> create() { return adoptRef(*new Rect); }

    String cssText() const
    {
        return generateCSSString(top()->cssText(), right()->cssText(), bottom()->cssText(), left()->cssText());
    }

private:
    Rect() = default;
    static String generateCSSString(const String& top, const String& right, const String& bottom, const String& left)
    {
        return "rect(" + top + ", " + right + ", " + bottom + ", " + left + ')';
    }
};

class Quad final : public RectBase, public RefCounted<Quad> {
public:
    static Ref<Quad> create() { return adoptRef(*new Quad); }

    String cssText() const
    {
        return generateCSSString(top()->cssText(), right()->cssText(), bottom()->cssText(), left()->cssText());
    }

    static String generateCSSString(String&& top, const String& right, const String& bottom, const String& left)
    {
        if (left != right)
            return makeString(top, ' ', right, ' ', bottom, ' ', left);
        if (bottom != top)
            return makeString(top, ' ', right, ' ', bottom);
        if (right != top)
            return makeString(top, ' ', right);
        return WTFMove(top);
    }

private:
    Quad() = default;
};

} // namespace WebCore
