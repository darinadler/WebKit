/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006 Apple Inc.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#include "CSSValue.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSValueList;

enum class TextDirection : bool;
enum class WritingMode : uint8_t;

enum CSSPropertyID : uint16_t;

bool isInitialValueForLonghand(CSSPropertyID, const CSSValue&);

struct StylePropertyMetadata {
    StylePropertyMetadata(CSSPropertyID propertyID, bool wasSetFromShorthand, int indexInShorthandsVector, bool important, bool implicit, bool inherited)
        : propertyIDInteger(propertyID)
        , wasSetFromShorthand(wasSetFromShorthand)
        , indexInShorthandsVector(indexInShorthandsVector)
        , important(important)
        , implicit(implicit)
        , inherited(inherited)
    {
        ASSERT(propertyID != CSSPropertyInvalid);
        ASSERT_WITH_MESSAGE(propertyID < firstShorthandProperty, "unexpected property: %d", propertyID);
    }

    CSSPropertyID propertyID() const { return static_cast<CSSPropertyID>(propertyIDInteger); }
    CSSPropertyID shorthandID() const;

    bool operator==(const StylePropertyMetadata& other) const
    {
        return propertyIDInteger == other.propertyIDInteger
            && wasSetFromShorthand == other.wasSetFromShorthand
            && indexInShorthandsVector == other.indexInShorthandsVector
            && important == other.important
            && implicit == other.implicit
            && inherited == other.inherited;
    }

    uint16_t propertyIDInteger : 10;
    uint16_t wasSetFromShorthand : 1;
    uint16_t indexInShorthandsVector : 2; // If this property was set as part of an ambiguous shorthand, gives the index in the shorthands vector.
    uint16_t important : 1;
    uint16_t implicit : 1; // Whether or not the property was set implicitly as the result of a shorthand.
    uint16_t inherited : 1;
};

class CSSProperty {
public:
    CSSProperty(CSSPropertyID propertyID, RefPtr<CSSValue>&& value, bool important = false, bool isSetFromShorthand = false, int indexInShorthandsVector = 0, bool implicit = false)
        : m_metadata(propertyID, isSetFromShorthand, indexInShorthandsVector, important, implicit, isInheritedProperty(propertyID))
        , m_value(WTFMove(value))
    {
    }

    CSSPropertyID id() const { return m_metadata.propertyID(); }
    bool wasSetFromShorthand() const { return m_metadata.wasSetFromShorthand; };
    CSSPropertyID shorthandID() const { return m_metadata.shorthandID(); };
    bool isImportant() const { return m_metadata.important; }

    CSSValue* value() const { return m_value.get(); }

    void wrapValueInCommaSeparatedList();

    static CSSPropertyID resolveDirectionAwareProperty(CSSPropertyID, TextDirection, WritingMode);
    static CSSPropertyID unresolvePhysicalProperty(CSSPropertyID, TextDirection, WritingMode);
    static bool isInheritedProperty(CSSPropertyID);
    static Vector<String> aliasesForProperty(CSSPropertyID);
    static bool isDirectionAwareProperty(CSSPropertyID);
    static bool isInLogicalPropertyGroup(CSSPropertyID);
    static bool areInSameLogicalPropertyGroupWithDifferentMappingLogic(CSSPropertyID, CSSPropertyID);
    static bool isDescriptorOnly(CSSPropertyID);
    static bool isColorProperty(CSSPropertyID);
    static UChar listValuedPropertySeparator(CSSPropertyID);
    static bool isListValuedProperty(CSSPropertyID propertyID) { return !!listValuedPropertySeparator(propertyID); }
    static Ref<CSSValueList> createListForProperty(CSSPropertyID);
    static bool allowsNumberOrIntegerInput(CSSPropertyID);

    const StylePropertyMetadata& metadata() const { return m_metadata; }

    bool operator==(const CSSProperty& other) const
    {
        if (!(m_metadata == other.m_metadata))
            return false;

        if (!m_value && !other.m_value)
            return true;

        if (!m_value || !other.m_value)
            return false;
        
        return m_value->equals(*other.m_value);
    }

private:
    StylePropertyMetadata m_metadata;
    RefPtr<CSSValue> m_value;
};

} // namespace WebCore

namespace WTF {

template<> struct VectorTraits<WebCore::CSSProperty> : VectorTraitsBase<false, WebCore::CSSProperty> {
    static const bool canInitializeWithMemset = true;
    static const bool canMoveWithMemcpy = true;
};

}
