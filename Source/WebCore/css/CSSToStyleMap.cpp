/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "CSSToStyleMap.h"

#include "Animation.h"
#include "CSSBackgroundRepeatValue.h"
#include "CSSBorderImageSliceValue.h"
#include "CSSBorderImageWidthValue.h"
#include "CSSImageSetValue.h"
#include "CSSImageValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSTimingFunctionValue.h"
#include "CSSValueKeywords.h"
#include "CompositeOperation.h"
#include "FillLayer.h"
#include "Pair.h"
#include "Rect.h"
#include "StyleBuilderConverter.h"
#include "StyleResolver.h"

namespace WebCore {

CSSToStyleMap::CSSToStyleMap(Style::BuilderState& builderState)
    : m_builderState(builderState)
{
}

RenderStyle* CSSToStyleMap::style() const
{
    return &m_builderState.style();
}

bool CSSToStyleMap::useSVGZoomRules() const
{
    return m_builderState.useSVGZoomRules();
}

RefPtr<StyleImage> CSSToStyleMap::styleImage(CSSValue& value)
{
    return m_builderState.createStyleImage(value);
}

void CSSToStyleMap::mapFillAttachment(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setAttachment(FillLayer::initialFillAttachment(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).valueID()) {
    case CSSValueFixed:
        layer.setAttachment(FillAttachment::FixedBackground);
        break;
    case CSSValueScroll:
        layer.setAttachment(FillAttachment::ScrollBackground);
        break;
    case CSSValueLocal:
        layer.setAttachment(FillAttachment::LocalBackground);
        break;
    default:
        return;
    }
}

void CSSToStyleMap::mapFillClip(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setClip(FillLayer::initialFillClip(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setClip(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillComposite(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setComposite(FillLayer::initialFillComposite(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setComposite(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillBlendMode(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setBlendMode(FillLayer::initialFillBlendMode(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setBlendMode(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillOrigin(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setOrigin(FillLayer::initialFillOrigin(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    layer.setOrigin(downcast<CSSPrimitiveValue>(value));
}

void CSSToStyleMap::mapFillImage(CSSPropertyID propertyID, FillLayer& layer, CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setImage(FillLayer::initialFillImage(layer.type()));
        return;
    }

    layer.setImage(styleImage(value));
}

void CSSToStyleMap::mapFillRepeat(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setRepeat(FillLayer::initialFillRepeat(layer.type()));
        return;
    }

    if (!is<CSSBackgroundRepeatValue>(value))
        return;

    auto& backgroundRepeatValue = downcast<CSSBackgroundRepeatValue>(value);
    FillRepeat repeatX = backgroundRepeatValue.xValue();
    FillRepeat repeatY = backgroundRepeatValue.yValue();
    layer.setRepeat(FillRepeatXY { repeatX, repeatY });
}

static inline bool convertToLengthSize(const CSSPrimitiveValue& primitiveValue, CSSToLengthConversionData conversionData, LengthSize& size)
{
    if (auto* pair = primitiveValue.pairValue()) {
        size.width = pair->first()->convertToLength<AnyConversion>(conversionData);
        size.height = pair->second()->convertToLength<AnyConversion>(conversionData);
    } else
        size.width = primitiveValue.convertToLength<AnyConversion>(conversionData);
    return !size.width.isUndefined() && !size.height.isUndefined();
}

void CSSToStyleMap::mapFillSize(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setSize(FillLayer::initialFillSize(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    FillSize fillSize;
    switch (primitiveValue.valueID()) {
    case CSSValueContain:
        fillSize.type = FillSizeType::Contain;
        break;
    case CSSValueCover:
        fillSize.type = FillSizeType::Cover;
        break;
    default:
        ASSERT(fillSize.type == FillSizeType::Size);
        if (!convertToLengthSize(primitiveValue, m_builderState.cssToLengthConversionData(), fillSize.size))
            return;
        break;
    }
    layer.setSize(fillSize);
}

void CSSToStyleMap::mapFillXPosition(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setXPosition(FillLayer::initialFillXPosition(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto* primitiveValue = &downcast<CSSPrimitiveValue>(value);
    Pair* pair = primitiveValue->pairValue();
    Length length;
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionX || propertyID == CSSPropertyWebkitMaskPositionX);
        length = Style::BuilderConverter::convertLength(m_builderState, *pair->second());
    } else
        length = Style::BuilderConverter::convertPositionComponentX(m_builderState, value);

    layer.setXPosition(length);
    if (pair)
        layer.setBackgroundXOrigin(*pair->first());
}

void CSSToStyleMap::mapFillYPosition(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(propertyID)) {
        layer.setYPosition(FillLayer::initialFillYPosition(layer.type()));
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto* primitiveValue = &downcast<CSSPrimitiveValue>(value);
    Pair* pair = primitiveValue->pairValue();
    Length length;
    if (pair) {
        ASSERT_UNUSED(propertyID, propertyID == CSSPropertyBackgroundPositionY || propertyID == CSSPropertyWebkitMaskPositionY);
        length = Style::BuilderConverter::convertLength(m_builderState, *pair->second());
    } else
        length = Style::BuilderConverter::convertPositionComponentY(m_builderState, value);
    
    layer.setYPosition(length);
    if (pair)
        layer.setBackgroundYOrigin(*pair->first());
}

void CSSToStyleMap::mapFillMaskMode(CSSPropertyID propertyID, FillLayer& layer, const CSSValue& value)
{
    MaskMode maskMode = FillLayer::initialFillMaskMode(layer.type());
    if (value.treatAsInitialValue(propertyID)) {
        layer.setMaskMode(maskMode);
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).valueID()) {
    case CSSValueAlpha:
        maskMode = MaskMode::Alpha;
        break;
    case CSSValueLuminance:
        maskMode = MaskMode::Luminance;
        break;
    case CSSValueMatchSource:
        ASSERT(propertyID == CSSPropertyMaskMode);
        maskMode = MaskMode::MatchSource;
        break;
    case CSSValueAuto:
        ASSERT(propertyID == CSSPropertyWebkitMaskSourceType);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    layer.setMaskMode(maskMode);
}

void CSSToStyleMap::mapAnimationDelay(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationDelay)) {
        animation.setDelay(Animation::initialDelay());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    animation.setDelay(downcast<CSSPrimitiveValue>(value).computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationDirection(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationDirection)) {
        layer.setDirection(Animation::initialDirection());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).valueID()) {
    case CSSValueNormal:
        layer.setDirection(Animation::AnimationDirectionNormal);
        break;
    case CSSValueAlternate:
        layer.setDirection(Animation::AnimationDirectionAlternate);
        break;
    case CSSValueReverse:
        layer.setDirection(Animation::AnimationDirectionReverse);
        break;
    case CSSValueAlternateReverse:
        layer.setDirection(Animation::AnimationDirectionAlternateReverse);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationDuration(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationDuration)) {
        animation.setDuration(Animation::initialDuration());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    animation.setDuration(downcast<CSSPrimitiveValue>(value).computeTime<double, CSSPrimitiveValue::Seconds>());
}

void CSSToStyleMap::mapAnimationFillMode(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationFillMode)) {
        layer.setFillMode(Animation::initialFillMode());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    switch (downcast<CSSPrimitiveValue>(value).valueID()) {
    case CSSValueNone:
        layer.setFillMode(AnimationFillMode::None);
        break;
    case CSSValueForwards:
        layer.setFillMode(AnimationFillMode::Forwards);
        break;
    case CSSValueBackwards:
        layer.setFillMode(AnimationFillMode::Backwards);
        break;
    case CSSValueBoth:
        layer.setFillMode(AnimationFillMode::Both);
        break;
    default:
        break;
    }
}

void CSSToStyleMap::mapAnimationIterationCount(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationIterationCount)) {
        animation.setIterationCount(Animation::initialIterationCount());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.valueID() == CSSValueInfinite)
        animation.setIterationCount(Animation::IterationCountInfinite);
    else
        animation.setIterationCount(primitiveValue.floatValue());
}

void CSSToStyleMap::mapAnimationName(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationName)) {
        layer.setName(Animation::initialName());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.valueID() == CSSValueNone)
        layer.setIsNoneAnimation(true);
    else
        layer.setName({ primitiveValue.stringValue(), primitiveValue.isCustomIdent() }, m_builderState.styleScopeOrdinal());
}

void CSSToStyleMap::mapAnimationPlayState(Animation& layer, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationPlayState)) {
        layer.setPlayState(Animation::initialPlayState());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    AnimationPlayState playState = (downcast<CSSPrimitiveValue>(value).valueID() == CSSValuePaused) ? AnimationPlayState::Paused : AnimationPlayState::Playing;
    layer.setPlayState(playState);
}

void CSSToStyleMap::mapAnimationProperty(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimation)) {
        animation.setProperty(Animation::initialProperty());
        return;
    }

    if (!is<CSSPrimitiveValue>(value))
        return;

    auto& primitiveValue = downcast<CSSPrimitiveValue>(value);
    if (primitiveValue.valueID() == CSSValueAll) {
        animation.setProperty({ Animation::TransitionMode::All, CSSPropertyInvalid });
        return;
    }
    if (primitiveValue.valueID() == CSSValueNone) {
        animation.setProperty({ Animation::TransitionMode::None, CSSPropertyInvalid });
        return;
    }
    if (primitiveValue.propertyID() == CSSPropertyInvalid) {
        animation.setProperty({ Animation::TransitionMode::UnknownProperty, CSSPropertyInvalid });
        animation.setUnknownProperty(primitiveValue.stringValue());
        return;
    }

    animation.setProperty({ Animation::TransitionMode::SingleProperty, primitiveValue.propertyID() });
}

void CSSToStyleMap::mapAnimationTimingFunction(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationTimingFunction))
        animation.setTimingFunction(Animation::initialTimingFunction());
    else if (auto timingFunction = TimingFunction::createFromCSSValue(value))
        animation.setTimingFunction(WTFMove(timingFunction));
}

void CSSToStyleMap::mapAnimationCompositeOperation(Animation& animation, const CSSValue& value)
{
    if (value.treatAsInitialValue(CSSPropertyAnimationComposition))
        animation.setCompositeOperation(Animation::initialCompositeOperation());
    else if (auto compositeOperation = toCompositeOperation(value))
        animation.setCompositeOperation(*compositeOperation);
}

void CSSToStyleMap::mapNinePieceImage(CSSValue* value, NinePieceImage& image)
{
    // If we're not a value list, then we are "none" and don't need to alter the empty image at all.
    if (!is<CSSValueList>(value))
        return;

    // Retrieve the border image value.
    CSSValueList& borderImage = downcast<CSSValueList>(*value);

    for (auto& current : borderImage) {
        if (current->isImage())
            image.setImage(styleImage(current));
        else if (auto* imageSlice = dynamicDowncast<CSSBorderImageSliceValue>(current.get()))
            mapNinePieceImageSlice(*imageSlice, image);
        else if (auto* slashList = dynamicDowncast<CSSValueList>(current.get()); slashList && slashList->separator() == CSSValueList::SlashSeparator) {
            // Map in the image slices.
            if (auto* imageSlice = dynamicDowncast<CSSBorderImageSliceValue>(slashList->item(0)))
                mapNinePieceImageSlice(*imageSlice, image);

            // Map in the border slices.
            if (auto* borderImageWidth = slashList->item(1))
                mapNinePieceImageWidth(*borderImageWidth, image);

            // Map in the outset.
            if (slashList->item(2))
                image.setOutset(mapNinePieceImageQuad(*slashList->item(2)));
        } else {
            // Set the appropriate rules for stretch/round/repeat of the slices.
            mapNinePieceImageRepeat(current, image);
        }
    }
}

void CSSToStyleMap::mapNinePieceImageSlice(CSSValue& value, NinePieceImage& image)
{
    mapNinePieceImageSlice(downcast<CSSBorderImageSliceValue>(value), image);
}

template<typename LengthConverterType> static LengthBox convertToLengthBox(CSSValue& lengths, const LengthConverterType& convertToLength)
{
    if (auto* singleValue = dynamicDowncast<CSSPrimitiveValue>(lengths)) {
        auto top = convertToLength(*singleValue);
        auto right = top;
        auto bottom = top;
        auto left = top;
        return { WTFMove(top), WTFMove(right), WTFMove(bottom), WTFMove(left) };
    }
    auto& list = downcast<CSSValueList>(lengths);
    ASSERT(list.length() >= 2);
    ASSERT(list.length() <= 4);
    auto valueAtIndex = [&](unsigned index) {
        return convertToLength(downcast<CSSPrimitiveValue>(*list.itemWithoutBoundsCheck(index)));
    };
    switch (list.length()) {
    case 2: {
        auto top = valueAtIndex(0);
        auto right = valueAtIndex(1);
        auto bottom = top;
        auto left = right;
        return { WTFMove(top), WTFMove(right), WTFMove(bottom), WTFMove(left) };
    }
    case 3: {
        auto right = valueAtIndex(1);
        auto left = right;
        return { valueAtIndex(0), WTFMove(right), valueAtIndex(2), WTFMove(left) };
    }
    case 4:
        return { valueAtIndex(0), valueAtIndex(1), valueAtIndex(2), valueAtIndex(3) };
    }
    ASSERT_NOT_REACHED();
    return { };
}

void CSSToStyleMap::mapNinePieceImageSlice(CSSBorderImageSliceValue& value, NinePieceImage& image)
{
    image.setImageSlices(convertToLengthBox(value.slices(), [](const CSSPrimitiveValue& value) {
        if (value.isPercentage())
            return Length(value.doubleValue(), LengthType::Percent);
        return Length(value.intValue(CSSUnitType::CSS_NUMBER), LengthType::Fixed);
    }));
    image.setFill(value.fill());
}

void CSSToStyleMap::mapNinePieceImageWidth(CSSValue& value, NinePieceImage& image)
{
    image.setBorderSlices(mapNinePieceImageQuad(value));
}

LengthBox CSSToStyleMap::mapNinePieceImageQuad(CSSValue& value)
{
    auto conversionData = useSVGZoomRules() ? m_builderState.cssToLengthConversionData().copyWithAdjustedZoom(1.0f) : m_builderState.cssToLengthConversionData();
    return convertToLengthBox(value, [&](const CSSPrimitiveValue& value) -> Length {
        if (value.isNumber())
            return { value.floatValue(), LengthType::Relative };
        if (value.isPercentage())
            return { value.doubleValue(CSSUnitType::CSS_PERCENTAGE), LengthType::Percent };
        if (value.isCalculatedPercentageWithLength())
            return Length { value.cssCalcValue()->createCalculationValue(conversionData) };
        if (value.valueID() != CSSValueAuto)
            return value.computeLength<Length>(conversionData);
        return { };
    });
}

void CSSToStyleMap::mapNinePieceImageRepeat(CSSValue& value, NinePieceImage& image)
{
    auto rule = [](CSSPrimitiveValue& value) {
        switch (value.valueID()) {
        case CSSValueStretch:
            return NinePieceImageRule::Stretch;
        case CSSValueRound:
            return NinePieceImageRule::Round;
        case CSSValueSpace:
            return NinePieceImageRule::Space;
        default:
            ASSERT(value.valueID() == CSSValueRepeat);
            return NinePieceImageRule::Repeat;
        }
    };

    if (auto* primitiveValue = dynamicDowncast<CSSPrimitiveValue>(value)) {
        auto singleRule = rule(*primitiveValue);
        image.setHorizontalRule(singleRule);
        image.setVerticalRule(singleRule);
        return;
    }

    ASSERT(downcast<CSSValueList>(value).size() == 2);
    auto itemAtIndex = [&](unsigned index) {
        return rule(downcast<CSSPrimitiveValue>(*downcast<CSSValueList>(value).itemWithoutBoundsCheck(index)));
    };
    image.setHorizontalRule(itemAtIndex(0));
    image.setVerticalRule(itemAtIndex(1));
}

};
