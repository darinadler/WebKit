/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004-2022 Apple Inc. All rights reserved.
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

#include "CSSProperty.h"
#include <memory>
#include <wtf/TypeCasts.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSStyleDeclaration;
class CachedResource;
class Color;
class ImmutableStyleProperties;
class MutableStyleProperties;
class PropertySetCSSStyleDeclaration;
class StyledElement;
class StylePropertyShorthand;

struct CSSParserContext;

enum CSSValueID : uint16_t;
enum CSSParserMode : uint8_t;

class StyleProperties : public RefCounted<StyleProperties> {
public:
    void operator delete(StyleProperties*, std::destroying_delete_t);

    class PropertyReference {
    public:
        PropertyReference(const StylePropertyMetadata& metadata, const CSSValue* value)
            : m_metadata(metadata)
            , m_value(value)
        { }

        CSSPropertyID id() const { return m_metadata.propertyID(); }
        CSSPropertyID shorthandID() const { return m_metadata.shorthandID(); }

        bool isImportant() const { return m_metadata.important; }
        bool isInherited() const { return m_metadata.inherited; }

        String name() const;
        String cssText() const;

        const CSSValue* value() const { return m_value; }

        bool deprecatedIsImplicit() const { return m_metadata.implicit; }
        bool deprecatedWasSetFromShorthand() const { return m_metadata.wasSetFromShorthand; }

        CSSValue* deprecatedValue() { return const_cast<CSSValue*>(m_value); }
        CSSProperty deprecatedCSSProperty() const { return CSSProperty(id(), const_cast<PropertyReference*>(this)->deprecatedValue(), m_metadata.important, m_metadata.wasSetFromShorthand, m_metadata.indexInShorthandsVector, m_metadata.implicit); }

    private:
        const StylePropertyMetadata& m_metadata;
        const CSSValue* m_value;
    };

    template<typename T> struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = PropertyReference;
        using difference_type = ptrdiff_t;
        using pointer = PropertyReference;
        using reference = PropertyReference;

        Iterator(const T& properties)
            : properties { properties }
        {
        }

        PropertyReference operator*() const { return properties.propertyAt(index); }
        Iterator& operator++() { ++index; return *this; }
        bool operator==(std::nullptr_t) const { return index >= properties.propertyCount(); }
        bool operator!=(std::nullptr_t) const { return index < properties.propertyCount(); }

    private:
        const T& properties;
        unsigned index { 0 };
    };

    unsigned propertyCount() const;
    bool isEmpty() const { return !propertyCount(); }
    PropertyReference propertyAt(unsigned) const;

    Iterator<StyleProperties> begin() const { return { *this }; }
    static constexpr std::nullptr_t end() { return nullptr; }
    unsigned size() const { return propertyCount(); }

    bool hasProperty(CSSPropertyID) const;
    WEBCORE_EXPORT RefPtr<CSSValue> propertyValue(CSSPropertyID) const;
    RefPtr<CSSValue> customPropertyValue(StringView customPropertyName) const;

    // FIXME: Why call it "string" when it's a single property, but "text" when it's all properties?
    // FIXME: Maybe use the word "serialize" in these function names.
    WEBCORE_EXPORT String propertyAsString(CSSPropertyID) const;
    String customPropertyAsString(StringView customPropertyName) const;

    WEBCORE_EXPORT std::optional<Color> propertyAsColor(CSSPropertyID) const;
    WEBCORE_EXPORT std::optional<CSSValueID> propertyAsValueID(CSSPropertyID) const;

    bool propertyIsImportant(CSSPropertyID) const;
    bool customPropertyIsImportant(StringView customPropertyName) const;

    String deprecatedPropertyShorthandName(CSSPropertyID) const;
    bool deprecatedIsPropertyImplicit(CSSPropertyID) const;

    Ref<MutableStyleProperties> copyBlockProperties() const;

    CSSParserMode cssParserMode() const { return static_cast<CSSParserMode>(m_cssParserMode); }

    WEBCORE_EXPORT Ref<MutableStyleProperties> mutableCopy() const;
    Ref<ImmutableStyleProperties> immutableCopyIfNeeded() const;

    Ref<MutableStyleProperties> copyPropertiesInSet(Span<const CSSPropertyID>) const;

    // FIXME: Why call it "string" when it's a single property, but "text" when it's all properties?
    // FIXME: Maybe use the word "serialize" in these function names.
    String asText() const;
    AtomString asTextAtom() const;

    bool hasCSSOMWrapper() const;
    bool isMutable() const { return m_isMutable; }

    bool traverseSubresources(const Function<bool(const CachedResource&)>& handler) const;

    static unsigned averageSizeInBytes();

    bool propertyMatches(CSSPropertyID, const CSSValue&) const;
    bool propertyMatches(CSSPropertyID, CSSValueID) const;

#ifndef NDEBUG
    void showStyle();
#endif

    int findPropertyIndex(CSSPropertyID) const;
    int findCustomPropertyIndex(StringView propertyName) const;

protected:
    enum Mutability : bool { Immutable, Mutable };
    StyleProperties(CSSParserMode mode, Mutability mutability)
        : m_cssParserMode(mode)
        , m_isMutable(mutability != Immutable)
    { }

    StyleProperties(CSSParserMode mode, unsigned immutableArraySize)
        : m_cssParserMode(mode)
        , m_type(ImmutablePropertiesType)
        , m_arraySize(immutableArraySize)
    { }

    unsigned immutablePropertyCount() const { return m_immutablePropertyCount; }

private:
    StringBuilder asTextInternal() const;

    unsigned m_cssParserMode : 3 { 0 };
    unsigned m_isMutable : 1 { false };
    unsigned m_immutablePropertyCount : 28 { 0 };
};

DECLARE_ALLOCATOR_WITH_HEAP_IDENTIFIER(ImmutableStyleProperties);
class ImmutableStyleProperties final : public StyleProperties {
    WTF_MAKE_FAST_ALLOCATED_WITH_HEAP_IDENTIFIER(ImmutableStyleProperties);
public:
    WEBCORE_EXPORT ~ImmutableStyleProperties();
    static Ref<ImmutableStyleProperties> create(Span<const CSSProperty>, CSSParserMode);

    unsigned propertyCount() const { return immutablePropertyCount(); }
    bool isEmpty() const { return !propertyCount(); }
    PropertyReference propertyAt(unsigned index) const;

    Iterator<ImmutableStyleProperties> begin() const { return { *this }; }
    static constexpr std::nullptr_t end() { return nullptr; }
    unsigned size() const { return propertyCount(); }

    int findPropertyIndex(CSSPropertyID) const;
    int findCustomPropertyIndex(StringView propertyName) const;
    
    void* m_storage;

private:
    PackedPtr<const CSSValue>* valueArray() const;
    const StylePropertyMetadata* metadataArray() const;
    ImmutableStyleProperties(Span<const CSSProperty>, CSSParserMode);
};

DECLARE_ALLOCATOR_WITH_HEAP_IDENTIFIER(MutableStyleProperties);
class MutableStyleProperties final : public StyleProperties {
    WTF_MAKE_FAST_ALLOCATED_WITH_HEAP_IDENTIFIER(MutableStyleProperties);
public:
    WEBCORE_EXPORT static Ref<MutableStyleProperties> create();
    WEBCORE_EXPORT static Ref<MutableStyleProperties> create(CSSParserMode);
    using PropertyVector = Vector<CSSProperty, 4>;
    static Ref<MutableStyleProperties> create(PropertyVector&&);

    WEBCORE_EXPORT ~MutableStyleProperties();

    unsigned propertyCount() const { return m_propertyVector.size(); }
    bool isEmpty() const { return !propertyCount(); }
    PropertyReference propertyAt(unsigned index) const;

    Iterator<MutableStyleProperties> begin() const { return { *this }; }
    static constexpr std::nullptr_t end() { return nullptr; }
    unsigned size() const { return propertyCount(); }

    PropertySetCSSStyleDeclaration* cssStyleDeclaration() { return m_cssomWrapper.get(); }

    bool addParsedProperties(Span<const CSSProperty>);
    bool addParsedProperty(const CSSProperty&);

    // These expand shorthand properties into multiple properties.
    bool setProperty(CSSPropertyID, const String& value, bool important, CSSParserContext, bool* didFailParsing = nullptr);
    bool setProperty(CSSPropertyID, const String& value, bool important = false, bool* didFailParsing = nullptr);
    void setProperty(CSSPropertyID, RefPtr<CSSValue>&&, bool important = false);

    // These do not. FIXME: This is too messy, we can do better.
    bool setProperty(CSSPropertyID, CSSValueID identifier, bool important = false);
    bool setProperty(const CSSProperty&, CSSProperty* slot = nullptr);

    bool removeProperty(CSSPropertyID, String* returnText = nullptr);
    void removeBlockProperties();
    bool removePropertiesInSet(Span<const CSSPropertyID>);

    void mergeAndOverrideOnConflict(const StyleProperties&);

    void clear();
    bool parseDeclaration(const String& styleDeclaration, CSSParserContext);

    WEBCORE_EXPORT CSSStyleDeclaration& ensureCSSStyleDeclaration();
    CSSStyleDeclaration& ensureInlineCSSStyleDeclaration(StyledElement& parentElement);

    int findPropertyIndex(CSSPropertyID) const;
    int findCustomPropertyIndex(StringView propertyName) const;
    
    // Methods for querying and altering CSS custom properties.
    bool setCustomProperty(const String& propertyName, const String& value, bool important, CSSParserContext);
    bool removeCustomProperty(const String& propertyName, String* returnText = nullptr);

private:
    explicit MutableStyleProperties(CSSParserMode);
    explicit MutableStyleProperties(const StyleProperties&);
    explicit MutableStyleProperties(PropertyVector&&);

    bool removeShorthandProperty(CSSPropertyID, String* returnText = nullptr);
    bool removePropertyAtIndex(int index, String* returnText);
    CSSProperty* findCSSPropertyWithID(CSSPropertyID);
    CSSProperty* findCustomCSSPropertyWithName(const String&);
    bool canUpdateInPlace(const CSSProperty&, CSSProperty* toReplace) const;

    friend class StyleProperties;

    PropertyVector m_propertyVector;
    std::unique_ptr<PropertySetCSSStyleDeclaration> m_cssomWrapper;
};

inline PackedPtr<const CSSValue>* ImmutableStyleProperties::valueArray() const
{
    return bitwise_cast<PackedPtr<const CSSValue>*>(metadataArray() + propertyCount());
}

inline const StylePropertyMetadata* ImmutableStyleProperties::metadataArray() const
{
    return reinterpret_cast<const StylePropertyMetadata*>(&m_storage);
}

inline ImmutableStyleProperties::PropertyReference ImmutableStyleProperties::propertyAt(unsigned index) const
{
    return PropertyReference(metadataArray()[index], valueArray()[index].get());
}

inline MutableStyleProperties::PropertyReference MutableStyleProperties::propertyAt(unsigned index) const
{
    auto& property = m_propertyVector[index];
    return PropertyReference(property.metadata(), property.value());
}

inline StyleProperties::PropertyReference StyleProperties::propertyAt(unsigned index) const
{
    if (is<MutableStyleProperties>(*this))
        return downcast<MutableStyleProperties>(*this).propertyAt(index);
    return downcast<ImmutableStyleProperties>(*this).propertyAt(index);
}

inline unsigned StyleProperties::propertyCount() const
{
    if (is<MutableStyleProperties>(*this))
        return downcast<MutableStyleProperties>(*this).propertyCount();
    return downcast<ImmutableStyleProperties>(*this).propertyCount();
}

inline void StyleProperties::operator delete(StyleProperties* value, std::destroying_delete_t)
{
    if (auto* mutableValue = dynamicDowncast<MutableStyleProperties>(*value)) {
        std::destroy_at(mutableValue);
        MutableStyleProperties::freeAfterDestruction(value);
    } else {
        std::destroy_at(&downcast<ImmutableStyleProperties>(*value));
        ImmutableStyleProperties::freeAfterDestruction(value);
    }
}

inline int StyleProperties::findPropertyIndex(CSSPropertyID propertyID) const
{
    if (is<MutableStyleProperties>(*this))
        return downcast<MutableStyleProperties>(*this).findPropertyIndex(propertyID);
    return downcast<ImmutableStyleProperties>(*this).findPropertyIndex(propertyID);
}

inline int StyleProperties::findCustomPropertyIndex(StringView propertyName) const
{
    if (is<MutableStyleProperties>(*this))
        return downcast<MutableStyleProperties>(*this).findCustomPropertyIndex(propertyName);
    return downcast<ImmutableStyleProperties>(*this).findCustomPropertyIndex(propertyName);
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::MutableStyleProperties)
    static bool isType(const WebCore::StyleProperties& set) { return set.isMutable(); }
SPECIALIZE_TYPE_TRAITS_END()

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ImmutableStyleProperties)
    static bool isType(const WebCore::StyleProperties& set) { return !set.isMutable(); }
SPECIALIZE_TYPE_TRAITS_END()
