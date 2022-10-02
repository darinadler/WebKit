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
#include "CSSSerializer.h"

namespace WebCore {

class RectBase {
public:
    CSSPrimitiveValue* top() const { return m_top.get(); }
    CSSPrimitiveValue* right() const { return m_right.get(); }
    CSSPrimitiveValue* bottom() const { return m_bottom.get(); }
    CSSPrimitiveValue* left() const { return m_left.get(); }

    void setTop(RefPtr<CSSPrimitiveValue>&& top) { m_top = WTFMove(top); }
    void setRight(RefPtr<CSSPrimitiveValue>&& right) { m_right = WTFMove(right); }
    void setBottom(RefPtr<CSSPrimitiveValue>&& bottom) { m_bottom = WTFMove(bottom); }
    void setLeft(RefPtr<CSSPrimitiveValue>&& left) { m_left = WTFMove(left); }

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
    RefPtr<CSSPrimitiveValue> m_top;
    RefPtr<CSSPrimitiveValue> m_right;
    RefPtr<CSSPrimitiveValue> m_bottom;
    RefPtr<CSSPrimitiveValue> m_left;
};

class Rect final : public RectBase, public RefCounted<Rect> {
public:
    static Ref<Rect> create() { return adoptRef(*new Rect); }

    void serialize(CSSSerializer& serializer) const
    {
        serializer.builder().append("rect("_s);
        top()->serialize(serializer);
        serializer.builder().append(", "_s);
        right()->serialize(serializer);
        serializer.builder().append(", "_s);
        bottom()->serialize(serializer);
        serializer.builder().append(", "_s);
        left()->serialize(serializer);
        serializer.builder().append(')');
    }

private:
    Rect() = default;
};

class Quad final : public RectBase, public RefCounted<Quad> {
public:
    static Ref<Quad> create() { return adoptRef(*new Quad); }

    void serialize(CSSSerializer& serializer) const
    {
        auto topStart = serializer.builder().length();
        top()->serialize(serializer);
        serializer.builder().append(' ');
        auto rightStart = serializer.builder().length();
        right()->serialize(serializer);
        serializer.builder().append(' ');
        auto bottomStart = serializer.builder().length();
        bottom()->serialize(serializer);
        serializer.builder().append(' ');
        auto leftStart = serializer.builder().length();
        left()->serialize(serializer);

        StringView fullString = serializer.builder();
        auto top = fullString.substring(topStart, rightStart - topStart - 1);
        auto right = fullString.substring(rightStart, bottomStart - rightStart - 1);
        auto bottom = fullString.substring(bottomStart, leftStart - bottomStart - 1);
        auto left = fullString.substring(leftStart);

        if (right == top && bottom == top && left == top)
            serializer.builder().shrink(rightStart - 1);
        else if (bottom == top && right == left)
            serializer.builder().shrink(bottomStart - 1);
        else if (right == left)
            serializer.builder().shrink(leftStart - 1);
    }

private:
    Quad() = default;
};

} // namespace WebCore
