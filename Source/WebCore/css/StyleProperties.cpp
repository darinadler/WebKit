/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004-2022 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "StyleProperties.h"

#include "CSSBorderImageWidthValue.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSCustomPropertyValue.h"
#include "CSSGridLineNamesValue.h"
#include "CSSGridTemplateAreasValue.h"
#include "CSSOffsetRotateValue.h"
#include "CSSParser.h"
#include "CSSParserIdioms.h"
#include "CSSPendingSubstitutionValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyParser.h"
#include "CSSTokenizer.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "Color.h"
#include "Document.h"
#include "FontSelectionValueInlines.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "Rect.h"
#include "StylePropertyShorthand.h"
#include "StylePropertyShorthandFunctions.h"
#include "StyleSheetContents.h"
#include <bitset>
#include <wtf/text/StringBuilder.h>

#ifndef NDEBUG
#include <stdio.h>
#include <wtf/text/CString.h>
#endif

namespace WebCore {

DEFINE_ALLOCATOR_WITH_HEAP_IDENTIFIER(ImmutableStyleProperties);
DEFINE_ALLOCATOR_WITH_HEAP_IDENTIFIER(MutableStyleProperties);

constexpr unsigned maxShorthandLength = 9; // FIXME: Generate this from CSSProperties.json.
constexpr unsigned maxShorthandsForLonghand = 4; // FIXME: Generate this from CSSProperties.json and use it for StylePropertyShorthandVector too.

static constexpr size_t sizeForImmutableStylePropertiesWithPropertyCount(unsigned count)
{
    return sizeof(ImmutableStyleProperties) - sizeof(void*) + sizeof(StylePropertyMetadata) * count + sizeof(PackedPtr<const CSSValue>) * count;
}

// See the function above if you need to update this.
struct SameSizeAsStyleProperties : public RefCounted<SameSizeAsStyleProperties> {
    unsigned bitfield;
};
static_assert(sizeof(StyleProperties) == sizeof(SameSizeAsStyleProperties), "style property set should stay small");

static bool isValueIDIncludingList(const CSSValue& value, CSSValueID id)
{
    if (is<CSSValueList>(value)) {
        auto& valueList = downcast<CSSValueList>(value);
        if (valueList.size() != 1)
            return false;
        auto* item = valueList.item(0);
        return item && isValueID(*item, id);
    }
    return isValueID(value, id);
}

static bool isValueIDIncludingList(const RefPtr<CSSValue>& value, CSSValueID id)
{
    return value && isValueIDIncludingList(*value, id);
}

Ref<ImmutableStyleProperties> ImmutableStyleProperties::create(Span<const CSSProperty> properties, CSSParserMode mode)
{
    void* slot = ImmutableStylePropertiesMalloc::malloc(sizeForImmutableStylePropertiesWithPropertyCount(properties.size()));
    return adoptRef(*new (NotNull, slot) ImmutableStyleProperties(properties, mode));
}

Ref<ImmutableStyleProperties> StyleProperties::immutableCopyIfNeeded() const
{
    if (is<ImmutableStyleProperties>(*this))
        return downcast<ImmutableStyleProperties>(const_cast<StyleProperties&>(*this));
    return ImmutableStyleProperties::create(downcast<MutableStyleProperties>(*this).m_propertyVector.span(), cssParserMode());
}

MutableStyleProperties::MutableStyleProperties(CSSParserMode cssParserMode)
    : StyleProperties(cssParserMode, Mutable)
{
}

MutableStyleProperties::MutableStyleProperties(PropertyVector&& properties)
    : StyleProperties(HTMLStandardMode, Mutable)
    , m_propertyVector(WTFMove(properties))
{
}

MutableStyleProperties::~MutableStyleProperties() = default;

ImmutableStyleProperties::ImmutableStyleProperties(Span<const CSSProperty> properties, CSSParserMode mode)
    : StyleProperties(mode, properties.size())
{
    auto* metadataArray = const_cast<StylePropertyMetadata*>(this->metadataArray());
    auto* valueArray = bitwise_cast<PackedPtr<const CSSValue>*>(this->valueArray());
    for (unsigned i = 0; i < properties.size(); ++i) {
        metadataArray[i] = properties[i].metadata();
        auto* value = properties[i].value();
        valueArray[i] = value;
        value->ref();
    }
}

ImmutableStyleProperties::~ImmutableStyleProperties()
{
    auto* valueArray = bitwise_cast<PackedPtr<const CSSValue>*>(this->valueArray());
    for (unsigned i = 0; i < propertyCount(); ++i)
        valueArray[i]->deref();
}

MutableStyleProperties::MutableStyleProperties(const StyleProperties& other)
    : StyleProperties(other.cssParserMode(), Mutable)
{
    if (is<MutableStyleProperties>(other))
        m_propertyVector = downcast<MutableStyleProperties>(other).m_propertyVector;
    else
        m_propertyVector = map(downcast<ImmutableStyleProperties>(other), [] (auto property) { return property.deprecatedCSSProperty(); });
}

static String textExpandingInitialValuePlaceholder(CSSPropertyID property, const CSSValue& value)
{
    if (value.isInitialValuePlaceholder())
        return initialValueTextForLonghand(property);

    // Special case for values that may have come from background and mask shorthands.
    // Those are comma-separated lists that may contain initial value placeholders.
    // Other lists won't have any initial value placeholders; it's harmless to use this code path for them.
    if (value.isBaseValueList()) {
        auto& list = downcast<CSSValueList>(value);
        if (list.separator() == CSSValueList::CommaSeparator) {
            StringBuilder result;
            auto separator = ""_s;
            for (auto& individualValue : downcast<CSSValueList>(value))
                result.append(std::exchange(separator, ", "_s), textExpandingInitialValuePlaceholder(property, individualValue));
            return result.toString();
        }
    }

    return value.cssText();
}

static CSSValueID valueIDExpandingInitialValuePlaceholder(CSSPropertyID property, const CSSValue& value)
{
    return value.isInitialValuePlaceholder() ? initialValueIDForLonghand(property) : valueID(value);
}

static std::optional<CSSValueID> valueIDExpandingInitialValuePlaceholder(CSSPropertyID property, const CSSValue* value)
{
    if (!value)
        return std::nullopt;
    return valueIDExpandingInitialValuePlaceholder(property, *value);
}

String StyleProperties::commonShorthandChecks(const StylePropertyShorthand& shorthand) const
{
    std::optional<CSSValueID> specialKeyword;
    bool allSpecialKeywords = true;
    std::optional<bool> importance;
    std::optional<const CSSPendingSubstitutionValue*> firstValueFromShorthand;
    for (auto longhand : shorthand) {
        int propertyIndex = findPropertyIndex(longhand);
        if (propertyIndex == -1)
            return emptyString();
        auto property = propertyAt(propertyIndex);

        // Don't serialize if longhands have different importance.
        bool isImportant = property.isImportant();
        if (importance.value_or(isImportant) != isImportant)
            return emptyString();
        importance = isImportant;

        auto value = property.value();

        // Don't serialize if longhands have different CSS-wide keywords.
        if (!value->isCSSWideKeyword()) {
            if (specialKeyword)
                return emptyString();
            allSpecialKeywords = false;
        } else {
            if (!allSpecialKeywords)
                return emptyString();
            auto keyword = valueID(value);
            if (specialKeyword.value_or(keyword) != keyword)
                return emptyString();
            specialKeyword = keyword;
            continue;
        }

        // Don't serialize if any longhand was set to a variable.
        if (is<CSSVariableReferenceValue>(value))
            return emptyString();

        // Don't serialize if any longhand was set by a different shorthand.
        auto* valueFromShorthand = dynamicDowncast<CSSPendingSubstitutionValue>(value);
        if (valueFromShorthand && valueFromShorthand->shorthandPropertyId() != shorthand.id())
            return emptyString();

        // Don't serialize if longhands are inconsistent about whether they were set by the shorthand.
        if (!firstValueFromShorthand)
            firstValueFromShorthand = valueFromShorthand;
        else {
            bool wasSetByShorthand = valueFromShorthand;
            bool firstWasSetByShorthand = *firstValueFromShorthand;
            if (firstWasSetByShorthand != wasSetByShorthand)
                return emptyString();
        }
    }
    if (specialKeyword)
        return nameString(*specialKeyword);
    if (*firstValueFromShorthand)
        return (*firstValueFromShorthand)->shorthandValue().cssText();
    return String();
}

String StyleProperties::propertyAsString(CSSPropertyID propertyID) const
{
    if (auto value = propertyValue(propertyID)) {
        switch (propertyID) {
        case CSSPropertyFillOpacity:
        case CSSPropertyFloodOpacity:
        case CSSPropertyOpacity:
        case CSSPropertyStopOpacity:
        case CSSPropertyStrokeOpacity:
            // Opacity percentage values serialize as a fraction in the range 0-1, not "%".
            if (is<CSSPrimitiveValue>(*value) && downcast<CSSPrimitiveValue>(*value).isPercentage())
                return makeString(downcast<CSSPrimitiveValue>(*value).doubleValue() / 100);
            FALLTHROUGH;
        default:
            return textExpandingInitialValuePlaceholder(propertyID, *value);
        }
    }

    auto shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length())
        return String();

    if (auto result = commonShorthandChecks(shorthand); !result.isNull())
        return result.isEmpty() ? nullString() : result;

    switch (propertyID) {
    case CSSPropertyAll:
    case CSSPropertyMarker:
        return getCommonValue(shorthand);
    case CSSPropertyAnimation:
    case CSSPropertyBackground:
    case CSSPropertyBackgroundPosition:
    case CSSPropertyMask:
    case CSSPropertyMaskPosition:
    case CSSPropertyTransition:
    case CSSPropertyWebkitMask:
    case CSSPropertyWebkitMaskPosition:
        return getLayeredShorthandValue(shorthand);
    case CSSPropertyBorderBlockEnd:
    case CSSPropertyBorderBlockStart:
    case CSSPropertyBorderBottom:
    case CSSPropertyBorderInlineEnd:
    case CSSPropertyBorderInlineStart:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderTop:
    case CSSPropertyColumnRule:
    case CSSPropertyColumns:
    case CSSPropertyFlex:
    case CSSPropertyFlexFlow:
    case CSSPropertyListStyle:
    case CSSPropertyOutline:
    case CSSPropertyPerspectiveOrigin:
    case CSSPropertyTextEmphasis:
    case CSSPropertyTransformOrigin:
    case CSSPropertyWebkitTextDecoration:
    case CSSPropertyWebkitTextStroke:
        return getShorthandValue(shorthand);
    case CSSPropertyBorderBlockColor:
    case CSSPropertyBorderBlockStyle:
    case CSSPropertyBorderBlockWidth:
    case CSSPropertyBorderInlineColor:
    case CSSPropertyBorderInlineStyle:
    case CSSPropertyBorderInlineWidth:
    case CSSPropertyBorderSpacing:
    case CSSPropertyContainIntrinsicSize:
    case CSSPropertyGap:
    case CSSPropertyInsetBlock:
    case CSSPropertyInsetInline:
    case CSSPropertyMarginBlock:
    case CSSPropertyMarginInline:
    case CSSPropertyOverflow:
    case CSSPropertyOverscrollBehavior:
    case CSSPropertyPaddingBlock:
    case CSSPropertyPaddingInline:
    case CSSPropertyScrollMarginBlock:
    case CSSPropertyScrollMarginInline:
    case CSSPropertyScrollPaddingBlock:
    case CSSPropertyScrollPaddingInline:
        return get2Values(shorthand);
    case CSSPropertyBorderColor:
    case CSSPropertyBorderStyle:
    case CSSPropertyBorderWidth:
    case CSSPropertyInset:
    case CSSPropertyMargin:
    case CSSPropertyPadding:
    case CSSPropertyScrollMargin:
    case CSSPropertyScrollPadding:
        return get4Values(shorthand);
    case CSSPropertyBorder:
        if (!hasAllInitialValues(borderImageShorthand()))
            return String();
        return borderPropertyValue(borderWidthShorthand(), borderStyleShorthand(), borderColorShorthand());
    case CSSPropertyBorderBlock:
        return borderPropertyValue(borderBlockWidthShorthand(), borderBlockStyleShorthand(), borderBlockColorShorthand());
    case CSSPropertyBorderInline:
        return borderPropertyValue(borderInlineWidthShorthand(), borderInlineStyleShorthand(), borderInlineColorShorthand());
    case CSSPropertyBorderImage:
    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitMaskBoxImage:
        return borderImagePropertyValue(shorthand);
    case CSSPropertyBorderRadius:
    case CSSPropertyWebkitBorderRadius:
        return borderRadiusShorthandValue(shorthand);
    case CSSPropertyOffset:
        return offsetValue();
    case CSSPropertyContainer:
        if (propertyAsValueID(CSSPropertyContainerType) == CSSValueNormal)
            return propertyAsString(CSSPropertyContainerName);
        return getShorthandValue(shorthand, " / ");
    case CSSPropertyGridArea:
        return getGridAreaShorthandValue();
    case CSSPropertyGridTemplate:
        return getGridTemplateValue();
    case CSSPropertyGrid:
        return getGridValue();
    case CSSPropertyGridColumn:
    case CSSPropertyGridRow:
        return getGridRowColumnShorthandValue(shorthand);
    case CSSPropertyPageBreakInside:
    case CSSPropertyWebkitColumnBreakInside:
        return breakInsideShorthandValue(shorthand);
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
        return pageBreakValue(shorthand);
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
        return webkitColumnBreakValue(shorthand);
    case CSSPropertyPlaceContent:
    case CSSPropertyPlaceItems:
    case CSSPropertyPlaceSelf:
        return getAlignmentShorthandValue(shorthand);
    case CSSPropertyFont:
        return fontValue(shorthand);
    case CSSPropertyFontVariant:
        return fontVariantValue();
    case CSSPropertyFontSynthesis:
        return fontSynthesisValue();
    case CSSPropertyTextDecorationSkip:
    case CSSPropertyTextDecoration:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyWebkitPerspective:
    case CSSPropertyWebkitTextOrientation:
        ASSERT(shorthand.length() == 1);
        return propertyAsString(shorthand.properties()[0]);
    default:
        ASSERT_NOT_REACHED();
        return String();
    }
}

std::optional<Color> StyleProperties::propertyAsColor(CSSPropertyID property) const
{
    auto colorValue = propertyValue(property);
    if (!is<CSSPrimitiveValue>(colorValue))
        return std::nullopt;

    auto& primitiveColor = downcast<CSSPrimitiveValue>(*colorValue);
    return primitiveColor.isRGBColor() ? primitiveColor.color() : CSSParser::parseColorWithoutContext(textExpandingInitialValuePlaceholder(property, *colorValue));
}

std::optional<CSSValueID> StyleProperties::propertyAsValueID(CSSPropertyID property) const
{
    return valueIDExpandingInitialValuePlaceholder(property, propertyValue(property).get());
}

String StyleProperties::customPropertyAsString(StringView propertyName) const
{
    if (auto value = customPropertyValue(propertyName))
        return value->cssText();
    return String();
}

static std::optional<CSSValueID> fontStretchKeyword(double value)
{
    // If the numeric value does not fit in the fixed point FontSelectionValue, don't convert it to a keyword even if it rounds to a keyword value.
    float valueAsFloat = value;
    FontSelectionValue valueAsFontSelectionValue { valueAsFloat };
    float valueAsFloatAfterRoundTrip = valueAsFontSelectionValue;
    if (value != valueAsFloatAfterRoundTrip)
        return std::nullopt;
    return fontStretchKeyword(valueAsFontSelectionValue);
}

String StyleProperties::fontValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.id() == CSSPropertyFont);

    // If all properties are set to the same system font shorthand, serialize as that.
    // If some but not all properties are, the font shorthand can't represent that, serialize as empty string.
    std::optional<CSSValueID> specialKeyword;
    bool allSpecialKeywords = true;
    for (auto property : shorthand) {
        // Can't call propertyAsValueID here because we need to bypass the isSystemFontShorthand check in propertyValue.
        int index = findPropertyIndex(property);
        auto keyword = index == -1 ? CSSValueInvalid : valueID(propertyAt(index).value());
        if (!CSSPropertyParserHelpers::isSystemFontShorthand(keyword))
            allSpecialKeywords = false;
        else {
            if (specialKeyword.value_or(keyword) != keyword)
                return emptyString();
            specialKeyword = keyword;
        }
    }
    if (specialKeyword)
        return allSpecialKeywords ? nameString(*specialKeyword) : emptyString();

    // If a subproperty is not set to the initial value, the font shorthand can't represent that, serialize as empty string.
    for (auto [property, initialValue] : fontShorthandSubpropertiesResetToInitialValues) {
        auto keyword = propertyAsValueID(property);
        if (keyword && *keyword != initialValue)
            return emptyString();
    }

    // These properties are the mandatory ones. If either is missing, serialize as empty string.
    auto size = propertyValue(CSSPropertyFontSize);
    auto family = propertyValue(CSSPropertyFontFamily);
    if (!size || !family)
        return emptyString();

    // Only two values of variant-caps can be serialized in the font shorthand, if the value is anything else serialize as empty string.
    auto variantCaps = propertyAsValueID(CSSPropertyFontVariantCaps);
    if (variantCaps && *variantCaps != CSSValueNormal && *variantCaps != CSSValueSmallCaps)
        return emptyString();

    // Font stretch values can only be serialized in the font shorthand as keywords, since percentages are also valid font sizes.
    // If a font stretch percentage can be expressed as a keyword, then do that.
    std::optional<CSSValueID> stretchKeyword;
    if (auto stretchBase = propertyValue(CSSPropertyFontStretch)) {
        auto stretch = downcast<CSSPrimitiveValue>(stretchBase.get());
        if (!stretch->isPercentage())
            stretchKeyword = stretch->valueID();
        else {
            stretchKeyword = fontStretchKeyword(stretch->doubleValue());
            if (!stretchKeyword)
                return emptyString();
        }
    }

    StringBuilder result;
    auto appendOptionalValue = [this, &result](CSSPropertyID property) {
        int foundPropertyIndex = findPropertyIndex(property);
        if (foundPropertyIndex == -1)
            return; // All longhands must have at least implicit values if "font" is specified.

        // Omit default normal values.
        if (propertyAsValueID(property) == CSSValueNormal)
            return;

        auto prefix = property == CSSPropertyLineHeight ? " / " : " ";
        result.append(result.isEmpty() ? "" : prefix, textExpandingInitialValuePlaceholder(property, *propertyAt(foundPropertyIndex).value()));
    };

    appendOptionalValue(CSSPropertyFontStyle);
    appendOptionalValue(CSSPropertyFontVariantCaps);
    appendOptionalValue(CSSPropertyFontWeight);
    if (stretchKeyword && *stretchKeyword != CSSValueNormal)
        result.append(result.isEmpty() ? "" : " ", nameString(*stretchKeyword));
    result.append(result.isEmpty() ? "" : " ", size->cssText());
    appendOptionalValue(CSSPropertyLineHeight);
    result.append(result.isEmpty() ? "" : " ", family->cssText());
    return result.toString();
}

String StyleProperties::offsetValue() const
{
    ASSERT(offsetShorthand().length() == 5);
    StringBuilder result;

    auto offsetPosition = propertyAt(findPropertyIndex(CSSPropertyOffsetPosition));
    if (!offsetPosition.deprecatedIsImplicit())
        result.append(offsetPosition.value()->cssText());

    auto offsetPath = propertyAt(findPropertyIndex(CSSPropertyOffsetPath));
    if (!offsetPath.deprecatedIsImplicit())
        result.append(result.isEmpty() ? "" : " ", offsetPath.value()->cssText());

    // At this point, result is not empty because either offset-position or offset-path
    // must be present.

    auto offsetDistance = propertyAt(findPropertyIndex(CSSPropertyOffsetDistance));
    if (!offsetDistance.deprecatedIsImplicit()) {
        auto offsetDistanceValue = offsetDistance.value();
        // Only include offset-distance if the distance is non-zero.
        // isZero() returns std::nullopt if offsetDistanceValue is a calculated value, in which case
        // we use value_or() to override to false.
        if (!downcast<CSSPrimitiveValue>(*offsetDistanceValue).isZero().value_or(false))
            result.append(' ', offsetDistanceValue->cssText());
    }

    auto offsetRotate = propertyAt(findPropertyIndex(CSSPropertyOffsetRotate));
    if (!offsetRotate.deprecatedIsImplicit()) {
        auto offsetRotateValue = offsetRotate.value();
        if (!downcast<CSSOffsetRotateValue>(*offsetRotateValue).isInitialValue())
            result.append(' ', offsetRotateValue->cssText());
    }

    auto offsetAnchor = propertyAt(findPropertyIndex(CSSPropertyOffsetAnchor));
    if (!offsetAnchor.deprecatedIsImplicit()) {
        auto offsetAnchorValue = offsetAnchor.value();
        if (!isValueID(offsetAnchorValue, CSSValueAuto))
            result.append(" / ", offsetAnchorValue->cssText());
    }

    return result.toString();
}

String StyleProperties::fontVariantValue() const
{
    StringBuilder result;
    bool isLigaturesNone = propertyAsValueID(CSSPropertyFontVariantLigatures) == CSSValueNone;
    for (auto property : fontVariantShorthand()) {
        auto value = propertyValue(property);

        // System keywords are not representable by font-variant and are the only reason propertyValue would return nullptr.
        if (!value)
            return emptyString();

        if (value->isInitialValuePlaceholder() || isValueID(*value, CSSValueNormal))
            continue;

        // font-variant cannot represent font-variant-ligatures: none along with other non-normal longhands.
        if (isLigaturesNone && property != CSSPropertyFontVariantLigatures)
            return emptyString();

        result.append(result.isEmpty() ? "" : " ", value->cssText());
    }
    return result.isEmpty() ? nameString(CSSValueNormal) : result.toString();
}

String StyleProperties::fontSynthesisValue() const
{
    // font-synthesis: none | [ weight || style || small-caps ]
    ASSERT(fontSynthesisShorthand().length() == 3);

    bool haveWeight = propertyAsValueID(CSSPropertyFontSynthesisWeight) == CSSValueAuto;
    bool haveStyle = propertyAsValueID(CSSPropertyFontSynthesisStyle) == CSSValueAuto;
    bool haveSmallCaps = propertyAsValueID(CSSPropertyFontSynthesisSmallCaps) == CSSValueAuto;

    if (!haveWeight && !haveStyle && !haveSmallCaps)
        return nameString(CSSValueNone);

    auto weight = haveWeight ? "weight" : "";
    auto styleSeparator = haveWeight && haveStyle ? " " : "";
    auto style = haveStyle ? "style" : "";
    auto smallCapsSeparator = (haveWeight || haveStyle) && haveSmallCaps ? " " : "";
    auto smallCaps = haveSmallCaps ? "small-caps" : "";
    return makeString(weight, styleSeparator, style, smallCapsSeparator, smallCaps);
}

String StyleProperties::get2Values(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 2);
    auto first = propertyAsString(shorthand.properties()[0]);
    auto second = propertyAsString(shorthand.properties()[1]);
    if (first != second)
        return makeString(first, ' ', second);
    return first;
}

String StyleProperties::get4Values(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 4);
    auto top = propertyAsString(shorthand.properties()[0]);
    auto right = propertyAsString(shorthand.properties()[1]);
    auto bottom = propertyAsString(shorthand.properties()[2]);
    auto left = propertyAsString(shorthand.properties()[3]);
    return Quad::generateCSSString(WTFMove(top), right, bottom, left);
}

class LayerValues {
public:
    explicit LayerValues(const StylePropertyShorthand& shorthand)
        : m_shorthand(shorthand)
    {
        ASSERT(m_shorthand.length() <= maxShorthandLength);
    }

    void set(unsigned index, const CSSValue* value, bool skipSerializing = false)
    {
        ASSERT(index < m_shorthand.length());
        m_skipSerializing[index] = skipSerializing
            || !value || isInitialValueForLonghand(m_shorthand.properties()[index], *value);
        m_values[index] = value;
    }

    bool& skip(unsigned index)
    {
        ASSERT(index < m_shorthand.length());
        return m_skipSerializing[index];
    }

    std::optional<CSSValueID> valueID(unsigned index) const
    {
        ASSERT(index < m_shorthand.length());
        return valueIDExpandingInitialValuePlaceholder(m_shorthand.properties()[index], m_values[index].get());
    }

    CSSValueID valueIDIncludingCustomIdent(unsigned index) const
    {
        auto* value = dynamicDowncast<CSSPrimitiveValue>(m_values[index].get());
        if (value && value->isCustomIdent())
            return cssValueKeywordID(value->stringValue());
        return valueID(index).value_or(CSSValueInvalid);
    }

    bool equalValueIDs(unsigned indexA, unsigned indexB) const
    {
        auto valueA = valueID(indexA);
        auto valueB = valueID(indexB);
        return valueA && valueB && *valueA == *valueB;
    }

    bool isValueID(unsigned index) const
    {
        auto result = valueID(index);
        return result && *result != CSSValueInvalid;
    }

    bool isPair(unsigned index) const
    {
        // This returns false for implicit initial values that are pairs, which is OK for now.
        ASSERT(index < m_shorthand.length());
        auto value = dynamicDowncast<CSSPrimitiveValue>(m_values[index].get());
        return value && value->isPair();
    }

    void serialize(StringBuilder& builder) const
    {
        // If all are skipped, then serialize the first.
        auto begin = std::begin(m_skipSerializing);
        auto end = begin + m_shorthand.length();
        bool allSkipped = std::find(begin, end, false) == end;

        auto separator = builder.isEmpty() ? ""_s : ", "_s;
        for (unsigned j = 0; j < m_shorthand.length(); j++) {
            if (allSkipped ? j : m_skipSerializing[j])
                continue;
            auto longhand = m_shorthand.properties()[j];
            if (longhand == CSSPropertyBackgroundSize || longhand == CSSPropertyMaskSize)
                separator = " / "_s;
            if (auto& value = m_values[j])
                builder.append(separator, textExpandingInitialValuePlaceholder(longhand, *value));
            else
                builder.append(separator, initialValueTextForLonghand(longhand));
            separator = " "_s;
        }
    }

private:
    const StylePropertyShorthand& m_shorthand;
    bool m_skipSerializing[maxShorthandLength] { };
    RefPtr<const CSSValue> m_values[maxShorthandLength];
};

String StyleProperties::getLayeredShorthandValue(const StylePropertyShorthand& shorthand) const
{
    unsigned length = shorthand.length();
    ASSERT(length <= maxShorthandLength);

    Vector<Ref<const CSSValue>, maxShorthandLength> values;
    size_t numLayers = 1;
    for (auto longhand : shorthand) {
        auto value = propertyValue(longhand);
        if (value->isBaseValueList())
            numLayers = std::max(downcast<CSSValueList>(*value).length(), numLayers);
        values.uncheckedAppend(value.releaseNonNull());
    }

    StringBuilder result;
    for (size_t i = 0; i < numLayers; i++) {
        LayerValues layerValues { shorthand };

        for (unsigned j = 0; j < length; j++) {
            auto& value = values[j].get();
            if (value.isBaseValueList())
                layerValues.set(j, downcast<CSSValueList>(value).item(i));
            else {
                // Color is only in the last layer. Other singletons are only in the first.
                auto singletonLayer = shorthand.properties()[j] == CSSPropertyBackgroundColor ? numLayers - 1 : 0;
                layerValues.set(j, &value, i != singletonLayer);
            }
        }

        for (unsigned j = 0; j < length; j++) {
            auto longhand = shorthand.properties()[j];

            // A single box value sets both background-origin and background-clip.
            // A single geometry-box value sets both mask-origin and mask-clip.
            // A single geometry-box value sets both mask-origin and -webkit-mask-clip.
            if (longhand == CSSPropertyBackgroundClip || longhand == CSSPropertyMaskClip || longhand == CSSPropertyWebkitMaskClip) {
                // The previous property is origin.
                ASSERT(j >= 1);
                ASSERT(shorthand.properties()[j - 1] == CSSPropertyBackgroundOrigin
                    || shorthand.properties()[j - 1] == CSSPropertyMaskOrigin);
                if (layerValues.equalValueIDs(j - 1, j)) {
                    // If the two are the same, one value sets both.
                    if (!layerValues.skip(j - 1) && !layerValues.skip(j))
                        layerValues.skip(j) = true;
                } else if (!layerValues.skip(j - 1) || !layerValues.skip(j)) {
                    // If the two are different, both need to be serialized, except in the special case of no-clip.
                    if (layerValues.valueID(j) != CSSValueNoClip) {
                        layerValues.skip(j - 1) = false;
                        layerValues.skip(j) = false;
                    }
                }
            }

            if (layerValues.skip(j))
                continue;

            // The syntax for background-size means that if it is present, background-position must be too.
            // The syntax for mask-size means that if it is present, mask-position must be too.
            if (longhand == CSSPropertyBackgroundSize || longhand == CSSPropertyMaskSize) {
                // The previous properties are X and Y.
                ASSERT(j >= 2);
                ASSERT(shorthand.properties()[j - 2] == CSSPropertyBackgroundPositionX
                    || shorthand.properties()[j - 2] == CSSPropertyWebkitMaskPositionX);
                ASSERT(shorthand.properties()[j - 1] == CSSPropertyBackgroundPositionY
                    || shorthand.properties()[j - 1] == CSSPropertyWebkitMaskPositionY);
                layerValues.skip(j - 2) = false;
                layerValues.skip(j - 1) = false;
            }

            // A single background-position value (identifier or numeric) sets the other value to center.
            // A single mask-position value (identifier or numeric) sets the other value to center.
            // Order matters when one is numeric, but not when both are identifiers.
            if (longhand == CSSPropertyBackgroundPositionY || longhand == CSSPropertyWebkitMaskPositionY) {
                // The previous property is X.
                ASSERT(j >= 1);
                ASSERT(shorthand.properties()[j - 1] == CSSPropertyBackgroundPositionX
                    || shorthand.properties()[j - 1] == CSSPropertyWebkitMaskPositionX);
                if (layerValues.valueID(j - 1) == CSSValueCenter && layerValues.isValueID(j))
                    layerValues.skip(j - 1) = true;
                else if (layerValues.valueID(j) == CSSValueCenter && !layerValues.isPair(j - 1)) {
                    layerValues.skip(j - 1) = false;
                    layerValues.skip(j) = true;
                }
            }

            // The first value in each animation shorthand that can be parsed as a time is assigned to
            // animation-duration, and the second is assigned to animation-delay, so we must serialize
            // both if we are serializing animation-delay.
            if (longhand == CSSPropertyAnimationDelay) {
                // The first property is animation-duration.
                constexpr auto animationDurationIndex = 0;
                ASSERT(shorthand.properties()[0] == CSSPropertyAnimationDuration);
                layerValues.skip(animationDurationIndex) = false;
            }
        }

        // In the animation shorthand, if the value of animation-name could be parsed as one of
        // the other longhands that longhand must be serialized to avoid ambiguity.
        if (shorthand.id() == CSSPropertyAnimation) {
            constexpr auto animationTimingFunctionIndex = 1;
            constexpr auto animationIterationCountIndex = 3;
            constexpr auto animationDirectionIndex = 4;
            constexpr auto animationFillModeIndex = 5;
            constexpr auto animationPlayStateIndex = 6;
            constexpr auto animationNameIndex = 7;
            ASSERT(shorthand.properties()[animationTimingFunctionIndex] == CSSPropertyAnimationTimingFunction);
            ASSERT(shorthand.properties()[animationIterationCountIndex] == CSSPropertyAnimationIterationCount);
            ASSERT(shorthand.properties()[animationDirectionIndex] == CSSPropertyAnimationDirection);
            ASSERT(shorthand.properties()[animationFillModeIndex] == CSSPropertyAnimationFillMode);
            ASSERT(shorthand.properties()[animationPlayStateIndex] == CSSPropertyAnimationPlayState);
            ASSERT(shorthand.properties()[animationNameIndex] == CSSPropertyAnimationName);
            if (!layerValues.skip(animationNameIndex)) {
                switch (layerValues.valueIDIncludingCustomIdent(animationNameIndex)) {
                case CSSValueAlternate:
                case CSSValueAlternateReverse:
                case CSSValueNormal:
                case CSSValueReverse:
                    layerValues.skip(animationDirectionIndex) = false;
                    break;
                case CSSValueBackwards:
                case CSSValueBoth:
                case CSSValueForwards:
                    layerValues.skip(animationFillModeIndex) = false;
                    break;
                case CSSValueEase:
                case CSSValueEaseIn:
                case CSSValueEaseInOut:
                case CSSValueEaseOut:
                case CSSValueLinear:
                case CSSValueStepEnd:
                case CSSValueStepStart:
                    layerValues.skip(animationTimingFunctionIndex) = false;
                    break;
                case CSSValueInfinite:
                    layerValues.skip(animationIterationCountIndex) = false;
                    break;
                case CSSValuePaused:
                case CSSValueRunning:
                    layerValues.skip(animationPlayStateIndex) = false;
                    break;
                default:
                    break;
                }
            }
        }

        layerValues.serialize(result);
    }

    return result.isEmpty() ? String() : result.toString();
}

String StyleProperties::getGridTemplateValue() const
{
    ASSERT(gridTemplateShorthand().length() == 3);

    auto rows = propertyValue(CSSPropertyGridTemplateRows);
    auto columns = propertyValue(CSSPropertyGridTemplateColumns);
    auto areas = propertyAt(findPropertyIndex(CSSPropertyGridTemplateAreas));
    if (!is<CSSGridTemplateAreasValue>(areas.value())) {
        auto rowsText = rows->cssText();
        auto columnsText = columns->cssText();
        if (rowsText == columnsText && (columns->isCSSWideKeyword() || isValueID(columns, CSSValueNone)))
            return rowsText;
        return makeString(rowsText, " / ", columnsText);
    }

    // We only want to try serializing the interleaved areas/templates
    // format if it was set from this shorthand, since that automatically
    // excludes values that can't be represented in this format (subgrid,
    // and the repeat() function).
    if (!areas.deprecatedWasSetFromShorthand())
        return String();

    StringBuilder result;

    ASSERT(is<CSSValueList>(rows));
    ASSERT(is<CSSGridTemplateAreasValue>(areas.value()));
    auto& areasValue = downcast<CSSGridTemplateAreasValue>(*areas.value());
    bool first = true;
    unsigned row = 0;
    for (auto& currentValue : downcast<CSSValueList>(*rows)) {
        if (!first)
            result.append(' ');
        first = false;

        if (is<CSSGridLineNamesValue>(currentValue))
            result.append(currentValue->cssText());
        else {
            result.append('"', areasValue.stringForRow(row), '"');
            if (!isValueID(currentValue, CSSValueAuto))
                result.append(' ', currentValue->cssText());
            row++;
        }
    }
    if (!isValueID(columns, CSSValueNone))
        result.append(" / ", columns->cssText());

    return result.toString();
}

static bool gridAutoFlowContains(const RefPtr<CSSValue>& autoFlow, CSSValueID id)
{
    if (is<CSSValueList>(autoFlow)) {
        for (auto& currentValue : downcast<CSSValueList>(*autoFlow)) {
            if (isValueID(currentValue, id))
                return true;
        }
        return false;
    }
    return isValueID(autoFlow, id);
}

String StyleProperties::getGridValue() const
{
    ASSERT(gridShorthand().length() == 6);

    auto autoColumns = propertyValue(CSSPropertyGridAutoColumns);
    auto autoRows = propertyValue(CSSPropertyGridAutoRows);
    auto autoFlow = propertyValue(CSSPropertyGridAutoFlow);

    if (isValueIDIncludingList(autoColumns, CSSValueAuto) && isValueIDIncludingList(autoRows, CSSValueAuto) && isValueIDIncludingList(autoFlow, CSSValueRow))
        return getGridTemplateValue();

    if (propertyAsValueID(CSSPropertyGridTemplateAreas) != CSSValueNone)
        return String();

    auto rows = propertyValue(CSSPropertyGridTemplateRows);
    auto columns = propertyValue(CSSPropertyGridTemplateColumns);

    auto dense = gridAutoFlowContains(autoFlow, CSSValueDense) ? " dense" : "";

    if (gridAutoFlowContains(autoFlow, CSSValueColumn)) {
        if (!isValueIDIncludingList(autoRows, CSSValueAuto) || !isValueIDIncludingList(columns, CSSValueNone))
            return String();

        auto autoColumnsSeparator = "";
        String autoColumnsString;
        if (!isValueIDIncludingList(autoColumns, CSSValueAuto)) {
            autoColumnsSeparator = " ";
            autoColumnsString = autoColumns->cssText();
        }
        return makeString(rows->cssText(), " / auto-flow", dense, autoColumnsSeparator, autoColumnsString);
    }

    if (!gridAutoFlowContains(autoFlow, CSSValueRow) && !gridAutoFlowContains(autoFlow, CSSValueDense))
        return String();

    if (!isValueIDIncludingList(autoColumns, CSSValueAuto) || !isValueIDIncludingList(rows, CSSValueNone))
        return String();

    auto autoRowsSeparator = "";
    String autoRowsString;
    if (!isValueIDIncludingList(autoRows, CSSValueAuto)) {
        autoRowsSeparator = " ";
        autoRowsString = autoRows->cssText();
    }
    return makeString("auto-flow", dense, autoRowsSeparator, autoRowsString, " / ", columns->cssText());
}

static bool isCustomIdentValue(const CSSValue& value)
{
    return is<CSSPrimitiveValue>(value) && downcast<CSSPrimitiveValue>(value).isCustomIdent();
}

static bool canOmitTrailingGridAreaValue(const CSSValue& value, const CSSValue& trailing)
{
    if (isCustomIdentValue(value))
        return isCustomIdentValue(trailing) && value.cssText() == trailing.cssText();
    return isValueID(trailing, CSSValueAuto);
}

String StyleProperties::getGridRowColumnShorthandValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 2);
    auto start = propertyValue(shorthand.properties()[0]);
    auto end = propertyValue(shorthand.properties()[1]);
    if (canOmitTrailingGridAreaValue(*start, *end))
        return start->cssText();
    return makeString(start->cssText(), " / ", end->cssText());
}

String StyleProperties::getGridAreaShorthandValue() const
{
    ASSERT(gridAreaShorthand().length() == 4);
    RefPtr<CSSValue> values[4];
    values[0] = propertyValue(CSSPropertyGridRowStart);
    values[1] = propertyValue(CSSPropertyGridColumnStart);
    values[2] = propertyValue(CSSPropertyGridRowEnd);
    values[3] = propertyValue(CSSPropertyGridColumnEnd);

    unsigned trailingValues = 3;
    if (canOmitTrailingGridAreaValue(*values[1], *values[3])) {
        trailingValues--;
        if (canOmitTrailingGridAreaValue(*values[0], *values[2])) {
            trailingValues--;
            if (canOmitTrailingGridAreaValue(*values[0], *values[1]))
                trailingValues--;
        }
    }

    StringBuilder result;
    result.append(values[0]->cssText());
    for (unsigned i = 1; i <= trailingValues; ++i)
        result.append(" / ", values[i]->cssText());
    return result.toString();
}

String StyleProperties::getShorthandValue(const StylePropertyShorthand& shorthand, const char* separator) const
{
    StringBuilder result;
    for (auto longhand : shorthand) {
        if (deprecatedIsPropertyImplicit(longhand))
            continue;
        result.append(result.isEmpty() ? "" : separator, propertyAsString(longhand));
    }
    return result.toString();
}

// Returns a non-null value if all properties have the same value.
String StyleProperties::getCommonValue(const StylePropertyShorthand& shorthand) const
{
    String result;
    for (auto longhand : shorthand) {
        String text = propertyAsString(longhand);
        if (result.isNull())
            result = text;
        else if (result != text)
            return String();
    }
    return result;
}

String StyleProperties::getAlignmentShorthandValue(const StylePropertyShorthand& shorthand) const
{
    auto commonValue = getCommonValue(shorthand);
    return commonValue.isNull() ? getShorthandValue(shorthand) : commonValue;
}

String StyleProperties::borderImagePropertyValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 5);
    StringBuilder result;
    bool omittedSlice = false;
    bool omittedWidth = false;
    String commonWideValueText;
    auto separator = "";
    for (auto longhand : shorthand) {
        auto value = propertyValue(longhand);

        // FIXME: We should omit values based on them being equal to the initial value, not based on the implicit flag.
        if (deprecatedIsPropertyImplicit(longhand)) {
            if (longhand == CSSPropertyBorderImageSlice || longhand == CSSPropertyWebkitMaskBoxImageSlice)
                omittedSlice = true;
            else if (longhand == CSSPropertyBorderImageWidth || longhand == CSSPropertyWebkitMaskBoxImageWidth)
                omittedWidth = true;
            continue;
        }
        if (omittedSlice && (longhand == CSSPropertyBorderImageWidth || longhand == CSSPropertyBorderImageOutset || longhand == CSSPropertyWebkitMaskBoxImageWidth || longhand == CSSPropertyWebkitMaskBoxImageOutset))
            return String();

        String valueText;

        // -webkit-border-image has a legacy behavior that makes fixed border slices also set the border widths.
        if (is<CSSBorderImageWidthValue>(value.get())) {
            auto* borderImageWidth = downcast<CSSBorderImageWidthValue>(value.get());
            Quad& widths = borderImageWidth->widths();
            bool overridesBorderWidths = shorthand.id() == CSSPropertyWebkitBorderImage && (widths.top()->isLength() || widths.right()->isLength() || widths.bottom()->isLength() || widths.left()->isLength());
            if (overridesBorderWidths != borderImageWidth->m_overridesBorderWidths)
                return String();
            valueText = widths.cssText();
        } else
            valueText = value->cssText();

        // Append separator and text.
        if (longhand == CSSPropertyBorderImageWidth || longhand == CSSPropertyWebkitMaskBoxImageWidth)
            separator = " / ";
        else if (longhand == CSSPropertyBorderImageOutset || longhand == CSSPropertyWebkitMaskBoxImageOutset)
            separator = omittedWidth ? " / / " : " / ";
        result.append(separator, valueText);
        separator = " ";
    }
    if (result.isEmpty())
        return nameString(CSSValueNone);
    return result.toString();
}

String StyleProperties::borderRadiusShorthandValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 4);
    RefPtr<CSSPrimitiveValue> horizontalRadii[4];
    RefPtr<CSSPrimitiveValue> verticalRadii[4];
    for (unsigned i = 0; i < 4; ++i) {
        auto pair = downcast<CSSPrimitiveValue>(*propertyValue(shorthand.properties()[i])).pairValue();
        horizontalRadii[i] = pair->first();
        verticalRadii[i] = pair->second();
    }

    bool serializeBoth = false;
    for (unsigned i = 0; i < 4; ++i) {
        if (!horizontalRadii[i]->equals(*verticalRadii[i])) {
            serializeBoth = true;
            break;
        }
    }

    StringBuilder result;
    auto serializeRadii = [&](const auto (&r)[4]) {
        if (!r[3]->equals(*r[1]))
            result.append(r[0]->cssText(), ' ', r[1]->cssText(), ' ', r[2]->cssText(), ' ', r[3]->cssText());
        else if (!r[2]->equals(*r[0]) || (shorthand.id() == CSSPropertyWebkitBorderRadius && !serializeBoth && !r[1]->equals(*r[0])))
            result.append(r[0]->cssText(), ' ', r[1]->cssText(), ' ', r[2]->cssText());
        else if (!r[1]->equals(*r[0]))
            result.append(r[0]->cssText(), ' ', r[1]->cssText());
        else
            result.append(r[0]->cssText());
    };
    serializeRadii(horizontalRadii);
    if (serializeBoth) {
        result.append(" / ");
        serializeRadii(verticalRadii);
    }
    return result.toString();
}

String StyleProperties::borderPropertyValue(const StylePropertyShorthand& width, const StylePropertyShorthand& style, const StylePropertyShorthand& color) const
{
    auto widthText = getCommonValue(width);
    if (widthText.isNull())
        return String();
    auto styleText = getCommonValue(style);
    if (styleText.isNull())
        return String();
    auto colorText = getCommonValue(color);
    if (colorText.isNull())
        return String();

    if (styleText == "none"_s)
        styleText = String();
    if (colorText == "currentcolor"_s)
        colorText = String();
    if (widthText == "medium"_s && (!styleText.isNull() || !colorText.isNull()))
        widthText = String();

    bool needFirstSpace = !widthText.isNull() && (!styleText.isNull() || !colorText.isNull());
    bool needSecondSpace = !styleText.isNull() && !colorText.isNull();
    return makeString(widthText, needFirstSpace ? " "_s : ""_s, styleText, needSecondSpace ? " "_s : ""_s, colorText);
}

String StyleProperties::breakInsideShorthandValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 1);
    auto longhand = shorthand.properties()[0];
    ASSERT(longhand == CSSPropertyBreakInside);
    auto keyword = propertyAsValueID(longhand).value_or(CSSValueInvalid);
    switch (keyword) {
    case CSSValueAuto:
    case CSSValueAvoid:
        return nameString(keyword);
    default:
        return String();
    }
}

String StyleProperties::pageBreakValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 1);
    auto longhand = shorthand.properties()[0];
    ASSERT(longhand == CSSPropertyBreakAfter || longhand == CSSPropertyBreakBefore);
    auto keyword = propertyAsValueID(longhand).value_or(CSSValueInvalid);
    switch (keyword) {
    case CSSValuePage:
        return nameString(CSSValueAlways);
    case CSSValueAuto:
    case CSSValueAvoid:
    case CSSValueLeft:
    case CSSValueRight:
        return nameString(keyword);
    default:
        return String();
    }
}

String StyleProperties::webkitColumnBreakValue(const StylePropertyShorthand& shorthand) const
{
    ASSERT(shorthand.length() == 1);
    auto longhand = shorthand.properties()[0];
    ASSERT(longhand == CSSPropertyBreakAfter || longhand == CSSPropertyBreakBefore);
    switch (propertyAsValueID(longhand).value_or(CSSValueInvalid)) {
    case CSSValueColumn:
        return nameString(CSSValueAlways);
    case CSSValueAvoidColumn:
        return nameString(CSSValueAvoid);
    case CSSValueAuto:
        return nameString(CSSValueAuto);
    default:
        return String();
    }
}

bool StyleProperties::hasAllInitialValues(const StylePropertyShorthand& shorthand) const
{
    for (auto longhand : shorthand) {
        auto value = propertyValue(longhand);
        if (!isInitialValueForLonghand(longhand, *value))
            return false;
    }
    return true;
}

RefPtr<CSSValue> StyleProperties::propertyValue(CSSPropertyID propertyID) const
{
    int foundPropertyIndex = findPropertyIndex(propertyID);
    if (foundPropertyIndex == -1)
        return nullptr;
    auto property = propertyAt(foundPropertyIndex);
    auto value = property.deprecatedValue();
    // System fonts are represented as CSSPrimitiveValue for various font subproperties, but these must serialize as the empty string.
    // It might be better to implement this as a special CSSValue type instead of turning them into null here.
    if (property.shorthandID() == CSSPropertyFont && CSSPropertyParserHelpers::isSystemFontShorthand(valueID(value)))
        return nullptr;
    return value;
}

bool StyleProperties::hasProperty(CSSPropertyID propertyID) const
{
    return findPropertyIndex(propertyID) != -1;
}

RefPtr<CSSValue> StyleProperties::customPropertyValue(StringView propertyName) const
{
    int foundPropertyIndex = findCustomPropertyIndex(propertyName);
    if (foundPropertyIndex == -1)
        return nullptr;
    return propertyAt(foundPropertyIndex).deprecatedValue();
}

bool MutableStyleProperties::removeShorthandProperty(CSSPropertyID propertyID, String* returnText)
{
    auto shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length())
        return false;
    if (returnText)
        *returnText = propertyAsString(propertyID);
    return removePropertiesInSet(shorthand.properties());
}

bool MutableStyleProperties::removePropertyAtIndex(int index, String* returnText)
{
    if (index == -1) {
        if (returnText)
            *returnText = emptyString();
        return false;
    }

    if (returnText) {
        auto property = propertyAt(index);
        *returnText = textExpandingInitialValuePlaceholder(property.id(), *property.value());
    }

    // A more efficient removal strategy would involve marking entries as empty
    // and sweeping them when the vector grows too big.
    m_propertyVector.remove(index);
    return true;
}

bool MutableStyleProperties::removeProperty(CSSPropertyID propertyID, String* returnText)
{
    return removeShorthandProperty(propertyID, returnText)
        || removePropertyAtIndex(findPropertyIndex(propertyID), returnText);
}

bool MutableStyleProperties::removeCustomProperty(const String& propertyName, String* returnText)
{
    return removePropertyAtIndex(findCustomPropertyIndex(propertyName), returnText);
}

bool StyleProperties::propertyIsImportant(CSSPropertyID propertyID) const
{
    int foundPropertyIndex = findPropertyIndex(propertyID);
    if (foundPropertyIndex != -1)
        return propertyAt(foundPropertyIndex).isImportant();

    auto shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length())
        return false;

    for (auto longhand : shorthand) {
        if (!propertyIsImportant(longhand))
            return false;
    }
    return true;
}

bool StyleProperties::customPropertyIsImportant(StringView propertyName) const
{
    int foundPropertyIndex = findCustomPropertyIndex(propertyName);
    return foundPropertyIndex != -1 && propertyAt(foundPropertyIndex).isImportant();
}

String StyleProperties::deprecatedPropertyShorthandName(CSSPropertyID propertyID) const
{
    int foundPropertyIndex = findPropertyIndex(propertyID);
    if (foundPropertyIndex == -1)
        return String();
    return nameString(propertyAt(foundPropertyIndex).shorthandID());
}

bool StyleProperties::deprecatedIsPropertyImplicit(CSSPropertyID propertyID) const
{
    int foundPropertyIndex = findPropertyIndex(propertyID);
    return foundPropertyIndex != -1 && propertyAt(foundPropertyIndex).deprecatedIsImplicit();
}

bool MutableStyleProperties::setProperty(CSSPropertyID propertyID, const String& value, bool important, CSSParserContext parserContext, bool* didFailParsing)
{
    if (!isExposed(propertyID, &parserContext.propertySettings) && !isInternal(propertyID)) {
        // Allow internal properties as we use them to handle certain DOM-exposed values
        // (e.g. -webkit-font-size-delta from execCommand('FontSizeDelta')).
        ASSERT_NOT_REACHED();
        return false;
    }

    // Setting the value to an empty string just removes the property in both IE and Gecko.
    // Setting it to null seems to produce less consistent results, but we treat it just the same.
    if (value.isEmpty())
        return removeProperty(propertyID);

    parserContext.mode = cssParserMode();

    // When replacing an existing property value, this moves the property to the end of the list.
    // Firefox preserves the position, and MSIE moves the property to the beginning.
    auto parseResult = CSSParser::parseValue(*this, propertyID, value, important, parserContext);
    if (didFailParsing)
        *didFailParsing = parseResult == CSSParser::ParseResult::Error;
    return parseResult == CSSParser::ParseResult::Changed;
}

bool MutableStyleProperties::setProperty(CSSPropertyID propertyID, const String& value, bool important, bool* didFailParsing)
{
    CSSParserContext parserContext(cssParserMode());
    return setProperty(propertyID, value, important, parserContext, didFailParsing);
}

bool MutableStyleProperties::setCustomProperty(const String& propertyName, const String& value, bool important, CSSParserContext parserContext)
{
    // Setting the value to an empty string just removes the property in both IE and Gecko.
    // Setting it to null seems to produce less consistent results, but we treat it just the same.
    if (value.isEmpty())
        return removeCustomProperty(propertyName);

    parserContext.mode = cssParserMode();

    auto propertyNameAtom = AtomString { propertyName };

    // When replacing an existing property value, this moves the property to the end of the list.
    // Firefox preserves the position, and MSIE moves the property to the beginning.
    return CSSParser::parseCustomPropertyValue(*this, propertyNameAtom, value, important, parserContext) == CSSParser::ParseResult::Changed;
}

void MutableStyleProperties::setProperty(CSSPropertyID propertyID, RefPtr<CSSValue>&& value, bool important)
{
    auto shorthand = shorthandForProperty(propertyID);
    if (!shorthand.length()) {
        setProperty(CSSProperty(propertyID, WTFMove(value), important));
        return;
    }

    removePropertiesInSet(shorthand.properties());

    for (auto longhand : shorthand)
        m_propertyVector.append(CSSProperty(longhand, value.copyRef(), important));
}

bool MutableStyleProperties::canUpdateInPlace(const CSSProperty& property, CSSProperty* toReplace) const
{
    // If the property is in a logical property group, we can't just update the value in-place,
    // because afterwards there might be another property of the same group but different mapping logic.
    // In that case the latter might override the former, so setProperty would have no effect.
    CSSPropertyID id = property.id();
    if (CSSProperty::isInLogicalPropertyGroup(id)) {
        ASSERT(toReplace >= m_propertyVector.begin());
        ASSERT(toReplace < m_propertyVector.end());
        for (CSSProperty* it = toReplace + 1; it != m_propertyVector.end(); ++it) {
            if (CSSProperty::areInSameLogicalPropertyGroupWithDifferentMappingLogic(id, it->id()))
                return false;
        }
    }
    return true;
}

bool MutableStyleProperties::setProperty(const CSSProperty& property, CSSProperty* slot)
{
    if (!removeShorthandProperty(property.id())) {
        CSSProperty* toReplace = slot;
        if (!slot) {
            if (property.id() == CSSPropertyCustom) {
                if (property.value())
                    toReplace = findCustomCSSPropertyWithName(downcast<CSSCustomPropertyValue>(*property.value()).name());
            } else
                toReplace = findCSSPropertyWithID(property.id());
        }
        
        if (toReplace) {
            if (canUpdateInPlace(property, toReplace)) {
                if (*toReplace == property)
                    return false;

                *toReplace = property;
                return true;
            }
            m_propertyVector.remove(toReplace - m_propertyVector.begin());
            toReplace = nullptr;
        }
    }

    m_propertyVector.append(property);
    return true;
}

bool MutableStyleProperties::setProperty(CSSPropertyID propertyID, CSSValueID identifier, bool important)
{
    return setProperty(CSSProperty(propertyID, CSSValuePool::createIdentifierValue(identifier), important));
}

bool MutableStyleProperties::parseDeclaration(const String& styleDeclaration, CSSParserContext context)
{
    auto oldProperties = WTFMove(m_propertyVector);
    m_propertyVector.clear();

    context.mode = cssParserMode();

    CSSParser parser(context);
    parser.parseDeclaration(*this, styleDeclaration);

    // We could do better. Just changing property order does not require style invalidation.
    return oldProperties != m_propertyVector;
}

bool MutableStyleProperties::addParsedProperties(Span<const CSSProperty> properties)
{
    bool anyChanged = false;
    m_propertyVector.reserveCapacity(m_propertyVector.size() + properties.size());
    for (auto& property : properties) {
        if (addParsedProperty(property))
            anyChanged = true;
    }
    return anyChanged;
}

bool MutableStyleProperties::addParsedProperty(const CSSProperty& property)
{
    if (property.id() == CSSPropertyCustom) {
        if ((property.value() && !customPropertyIsImportant(downcast<CSSCustomPropertyValue>(*property.value()).name())) || property.isImportant())
            return setProperty(property);
        return false;
    }
    return setProperty(property);
}

String StyleProperties::asText() const
{
    return asTextInternal().toString();
}

AtomString StyleProperties::asTextAtom() const
{
    return asTextInternal().toAtomString();
}

static constexpr bool canUseShorthandForLonghand(CSSPropertyID shorthandID, CSSPropertyID longhandID)
{
    ASSERT(isShorthandCSSProperty(shorthandID));
    ASSERT(isLonghand(longhandID));
    switch (shorthandID) {
    // We are not yet using the CSSPropertyFont shorthand here because our editing code is currently incompatible.
    case CSSPropertyFont:
        return false;

    // Avoid legacy shorthands according to https://www.w3.org/TR/css-cascade-5/#legacy-shorthand
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyWebkitBorderRadius:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitMaskPosition:
    case CSSPropertyWebkitPerspective:
    case CSSPropertyWebkitTextDecoration:
    case CSSPropertyWebkitTextOrientation:
        return false;

    // No other browser currently supports text-decoration-skip, so it's currently more web
    // compatible to avoid collapsing text-decoration-skip-ink, its only longhand.
    case CSSPropertyTextDecorationSkip:
        return false;

    // FIXME: -webkit-mask is a legacy shorthand but it's used to serialize -webkit-mask-clip,
    // which should be a legacy shorthand of mask-clip, but it's implemented as a longhand.
    case CSSPropertyWebkitMask:
        return longhandID == CSSPropertyWebkitMaskClip;

    // FIXME: more mask nonsense.
    case CSSPropertyMask:
        return longhandID != CSSPropertyMaskComposite && longhandID != CSSPropertyMaskMode && longhandID != CSSPropertyMaskSize;

    // FIXME: If font-variant-ligatures is none, this depends on the value of the longhand.
    case CSSPropertyFontVariant:
    // FIXME: These shorthands are avoided for unknown legacy reasons, probably shouldn't be avoided.
    case CSSPropertyBorderBlockEnd:
    case CSSPropertyBorderBlockStart:
    case CSSPropertyBorderBottom:
    case CSSPropertyBorderInlineEnd:
    case CSSPropertyBorderInlineStart:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderTop:
    case CSSPropertyColumnRule:
    case CSSPropertyColumns:
    case CSSPropertyContainer:
    case CSSPropertyFontSynthesis:
    case CSSPropertyGap:
    case CSSPropertyGridArea:
    case CSSPropertyGridColumn:
    case CSSPropertyGridRow:
    case CSSPropertyMaskPosition:
    case CSSPropertyOffset:
    case CSSPropertyPlaceContent:
    case CSSPropertyPlaceItems:
    case CSSPropertyPlaceSelf:
    case CSSPropertyTextEmphasis:
    case CSSPropertyWebkitTextStroke:
        return false;

    default:
        return true;
    }
}

StringBuilder StyleProperties::asTextInternal() const
{
    StringBuilder result;

    constexpr unsigned shorthandPropertyCount = lastShorthandProperty - firstShorthandProperty + 1;
    std::bitset<shorthandPropertyCount> shorthandPropertyUsed;
    std::bitset<shorthandPropertyCount> shorthandPropertyAppeared;

    auto separator = "";
    for (auto property : *this) {
        auto propertyID = property.id();
        ASSERT(isLonghand(propertyID) || propertyID == CSSPropertyCustom);
        Vector<CSSPropertyID, maxShorthandsForLonghand> shorthands;

        if (is<CSSPendingSubstitutionValue>(property.value())) {
            auto& substitutionValue = downcast<CSSPendingSubstitutionValue>(*property.value());
            shorthands.append(substitutionValue.shorthandPropertyId());
        } else {
            for (auto& shorthand : matchingShorthandsForLonghand(propertyID)) {
                if (canUseShorthandForLonghand(shorthand.id(), propertyID))
                    shorthands.append(shorthand.id());
            }
        }

        String value;
        bool alreadyUsedShorthand = false;
        for (auto& shorthandPropertyID : shorthands) {
            ASSERT(isShorthandCSSProperty(shorthandPropertyID));
            unsigned shorthandPropertyIndex = shorthandPropertyID - firstShorthandProperty;

            ASSERT(shorthandPropertyIndex < shorthandPropertyUsed.size());
            if (shorthandPropertyUsed[shorthandPropertyIndex]) {
                alreadyUsedShorthand = true;
                break;
            }
            if (shorthandPropertyAppeared[shorthandPropertyIndex])
                continue;
            shorthandPropertyAppeared.set(shorthandPropertyIndex);

            value = propertyAsString(shorthandPropertyID);
            if (!value.isNull()) {
                propertyID = shorthandPropertyID;
                shorthandPropertyUsed.set(shorthandPropertyIndex);
                break;
            }
        }
        if (alreadyUsedShorthand)
            continue;

        String name;
        if (propertyID == CSSPropertyCustom)
            name = downcast<CSSCustomPropertyValue>(*property.value()).name();
        else
            name = nameString(propertyID);

        if (value.isNull())
            value = textExpandingInitialValuePlaceholder(propertyID, *property.value());

        result.append(separator, name, ": ", value, property.isImportant() ? " !important" : "", ';');
        separator = " ";
    }

    return result;
}

bool StyleProperties::hasCSSOMWrapper() const
{
    return is<MutableStyleProperties>(*this) && downcast<MutableStyleProperties>(*this).m_cssomWrapper;
}

void MutableStyleProperties::mergeAndOverrideOnConflict(const StyleProperties& other)
{
    for (auto property : other)
        addParsedProperty(property.deprecatedCSSProperty());
}

bool StyleProperties::traverseSubresources(const Function<bool(const CachedResource&)>& handler) const
{
    for (auto property : *this) {
        if (property.value()->traverseSubresources(handler))
            return true;
    }
    return false;
}

void MutableStyleProperties::clear()
{
    m_propertyVector.clear();
}

// This is the list of properties affected by the copyBlockProperties() and removeBlockProperties functions.
// It is the list of CSS properties that apply specially to block-level elements.
static constexpr CSSPropertyID blockProperties[] = {
    CSSPropertyColumnCount,
    CSSPropertyColumnGap,
    CSSPropertyColumnRuleColor,
    CSSPropertyColumnRuleStyle,
    CSSPropertyColumnRuleWidth,
    CSSPropertyColumnWidth,
    CSSPropertyOrphans,
    CSSPropertyOverflow, // This can be also be applied to replaced elements.
    CSSPropertyPageBreakAfter,
    CSSPropertyPageBreakBefore,
    CSSPropertyPageBreakInside,
    CSSPropertyRowGap,
    CSSPropertyTextAlign,
    CSSPropertyTextAlignLast,
    CSSPropertyTextIndent,
    CSSPropertyTextJustify,
    CSSPropertyWebkitColumnBreakAfter,
    CSSPropertyWebkitColumnBreakBefore,
    CSSPropertyWebkitColumnBreakInside,
    CSSPropertyWidows,
};

Ref<MutableStyleProperties> StyleProperties::copyBlockProperties() const
{
    return copyPropertiesInSet(blockProperties);
}

void MutableStyleProperties::removeBlockProperties()
{
    removePropertiesInSet(blockProperties);
}

bool MutableStyleProperties::removePropertiesInSet(Span<const CSSPropertyID> properties)
{
    if (m_propertyVector.isEmpty())
        return false;

    // FIXME: The sets passed in are always known at compile time: constructing a hash table each time is unnecessarily expensive.
    // FIXME: The sets passed in are relatively small. It's possible a linear or binary search instead would be fast enough.
    HashSet<CSSPropertyID> set;
    set.add(properties.begin(), properties.end());
    return m_propertyVector.removeAllMatching([&set] (const CSSProperty& property) {
        return set.contains(property.id());
    }) > 0;
}

int ImmutableStyleProperties::findPropertyIndex(CSSPropertyID propertyID) const
{
    // Convert to the underlying integer type here for better loop efficiency in some compilers.
    uint16_t id = static_cast<uint16_t>(propertyID);
    for (int n = propertyCount() - 1 ; n >= 0; --n) {
        if (metadataArray()[n].propertyIDInteger == id)
            return n;
    }
    return -1;
}

int MutableStyleProperties::findPropertyIndex(CSSPropertyID propertyID) const
{
    // Convert to the underlying integer type here for better loop efficiency in some compilers.
    uint16_t id = static_cast<uint16_t>(propertyID);
    for (int n = m_propertyVector.size() - 1 ; n >= 0; --n) {
        if (m_propertyVector[n].metadata().propertyIDInteger == id)
            return n;
    }
    return -1;
}

int ImmutableStyleProperties::findCustomPropertyIndex(StringView propertyName) const
{
    for (int n = propertyCount() - 1 ; n >= 0; --n) {
        if (metadataArray()[n].propertyIDInteger == CSSPropertyCustom) {
            if (downcast<CSSCustomPropertyValue>(*valueArray()[n]).name() == propertyName)
                return n;
        }
    }
    return -1;
}

int MutableStyleProperties::findCustomPropertyIndex(StringView propertyName) const
{
    for (int n = m_propertyVector.size() - 1 ; n >= 0; --n) {
        if (m_propertyVector[n].metadata().propertyIDInteger == CSSPropertyCustom) {
            if (downcast<CSSCustomPropertyValue>(*m_propertyVector[n].value()).name() == propertyName)
                return n;
        }
    }
    return -1;
}

CSSProperty* MutableStyleProperties::findCSSPropertyWithID(CSSPropertyID propertyID)
{
    int foundPropertyIndex = findPropertyIndex(propertyID);
    if (foundPropertyIndex == -1)
        return nullptr;
    return &m_propertyVector[foundPropertyIndex];
}

CSSProperty* MutableStyleProperties::findCustomCSSPropertyWithName(const String& propertyName)
{
    int foundPropertyIndex = findCustomPropertyIndex(propertyName);
    if (foundPropertyIndex == -1)
        return nullptr;
    return &m_propertyVector[foundPropertyIndex];
}

bool StyleProperties::propertyMatches(CSSPropertyID propertyID, const CSSValue& expectedValue) const
{
    auto value = propertyValue(propertyID);
    if (!value)
        return false;
    if (value->isInitialValuePlaceholder())
        return isInitialValueForLonghand(propertyID, expectedValue);
    if (expectedValue.isInitialValuePlaceholder())
        return false;
    return value->equals(expectedValue);
}

bool StyleProperties::propertyMatches(CSSPropertyID propertyID, CSSValueID expectedValue) const
{
    return propertyAsValueID(propertyID) == expectedValue;
}

Ref<MutableStyleProperties> StyleProperties::mutableCopy() const
{
    return adoptRef(*new MutableStyleProperties(*this));
}

Ref<MutableStyleProperties> StyleProperties::copyPropertiesInSet(Span<const CSSPropertyID> set) const
{
    MutableStyleProperties::PropertyVector vector;
    vector.reserveInitialCapacity(set.size());
    for (auto property : set) {
        if (auto value = propertyValue(property))
            vector.uncheckedAppend(CSSProperty(property, WTFMove(value), false));
    }
    vector.shrinkToFit();
    return MutableStyleProperties::create(WTFMove(vector));
}

CSSStyleDeclaration& MutableStyleProperties::ensureCSSStyleDeclaration()
{
    if (m_cssomWrapper) {
        ASSERT(!static_cast<CSSStyleDeclaration*>(m_cssomWrapper.get())->parentRule());
        ASSERT(!m_cssomWrapper->parentElement());
        return *m_cssomWrapper;
    }
    m_cssomWrapper = makeUnique<PropertySetCSSStyleDeclaration>(*this);
    return *m_cssomWrapper;
}

CSSStyleDeclaration& MutableStyleProperties::ensureInlineCSSStyleDeclaration(StyledElement& parentElement)
{
    if (m_cssomWrapper) {
        ASSERT(m_cssomWrapper->parentElement() == &parentElement);
        return *m_cssomWrapper;
    }
    m_cssomWrapper = makeUnique<InlineCSSStyleDeclaration>(*this, parentElement);
    return *m_cssomWrapper;
}

unsigned StyleProperties::averageSizeInBytes()
{
    // Please update this if the storage scheme changes so that this longer reflects the actual size.
    return sizeForImmutableStylePropertiesWithPropertyCount(4);
}

Ref<MutableStyleProperties> MutableStyleProperties::create()
{
    return adoptRef(*new MutableStyleProperties(HTMLQuirksMode));
}

Ref<MutableStyleProperties> MutableStyleProperties::create(CSSParserMode mode)
{
    return adoptRef(*new MutableStyleProperties(mode));
}

Ref<MutableStyleProperties> MutableStyleProperties::create(PropertyVector&& properties)
{
    return adoptRef(*new MutableStyleProperties(WTFMove(properties)));
}

String StyleProperties::PropertyReference::name() const
{
    if (id() == CSSPropertyCustom)
        return downcast<CSSCustomPropertyValue>(*value()).name();
    return nameString(id());
}

String StyleProperties::PropertyReference::cssText() const
{
    return makeString(name(), ": ", m_value->cssText(), isImportant() ? " !important" : "", ';');
}

#ifndef NDEBUG

void StyleProperties::showStyle()
{
    fprintf(stderr, "%s\n", asText().ascii().data());
}

#endif

} // namespace WebCore
