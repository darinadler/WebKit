/*
 * Copyright (c) 2012-2014, Google Inc. All rights reserved.
 * Copyright (c) 2012-2023, Apple Inc. All rights reserved.
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

#include <limits>
#include <math.h>
#include <stdlib.h>
#include <wtf/HashTraits.h>
#include <wtf/MathExtras.h>
#include <wtf/SaturatedArithmetic.h>

namespace WTF {
class TextStream;
}

namespace WebCore {

constexpr int kLayoutUnitFractionalBits = 6;
constexpr int kFixedPointDenominator = 1 << kLayoutUnitFractionalBits;

class LayoutUnit {
public:
    LayoutUnit() = default;
    constexpr LayoutUnit(int value) : m_rawValue(makeRawValue(value)) { }
    constexpr LayoutUnit(unsigned short value) : m_rawValue(value * kFixedPointDenominator) { }
    constexpr LayoutUnit(unsigned value) : m_rawValue(makeRawValue(value)) { }
    explicit LayoutUnit(double value) : m_rawValue(clampToInteger(value * kFixedPointDenominator)) { }

    // FIXME: These can overflow since they clamp after multiplying, but the constructors above all clamp safely. Consider deleting these or fixing them.
    explicit constexpr LayoutUnit(unsigned long value) : m_rawValue(clampTo<int>(value * kFixedPointDenominator)) { }
    explicit constexpr LayoutUnit(unsigned long long value) : m_rawValue(clampTo<int>(value * kFixedPointDenominator)) { }

    // FIXME: Why do we have an assignment operator that takes a float but no constructor? If we want to be explicit about ceil/floor/round, then we should not have an assignment operator.
    LayoutUnit& operator=(float other) { return *this = LayoutUnit(other); }

    static LayoutUnit fromFloatCeil(float value)
    {
        return fromRawValue(clampToInteger(std::ceil(value * kFixedPointDenominator)));
    }

    static LayoutUnit fromFloatFloor(float value)
    {
        return fromRawValue(clampToInteger(std::floor(value * kFixedPointDenominator)));
    }

    static LayoutUnit fromFloatRound(float value)
    {
        if (value >= 0)
            return clamp(value + epsilon() / 2.0f);
        return clamp(value - epsilon() / 2.0f);
    }

    static constexpr LayoutUnit fromRawValue(int rawValue)
    {
        LayoutUnit result;
        result.m_rawValue = rawValue;
        return result;
    }

    constexpr int toInt() const { return m_rawValue / kFixedPointDenominator; }
    constexpr float toFloat() const { return static_cast<float>(m_rawValue) / kFixedPointDenominator; }
    constexpr double toDouble() const { return static_cast<double>(m_rawValue) / kFixedPointDenominator; }
    constexpr unsigned toUnsigned() const { ASSERT_UNDER_CONSTEXPR_CONTEXT(m_rawValue >= 0); return toInt(); }

    constexpr operator int() const { return toInt(); }
    constexpr operator float() const { return toFloat(); }
    constexpr operator double() const { return toDouble(); }
    explicit constexpr operator bool() const { return m_rawValue; }

    LayoutUnit& operator++()
    {
        m_rawValue += kFixedPointDenominator;
        return *this;
    }

    constexpr int rawValue() const { return m_rawValue; }

    constexpr LayoutUnit abs() const { return fromRawValue(std::abs(m_rawValue)); }

    constexpr int ceil() const
    {
        if (UNLIKELY(m_rawValue >= std::numeric_limits<int>::max() - kFixedPointDenominator + 1))
            return max();
        if (m_rawValue >= 0)
            return (m_rawValue + kFixedPointDenominator - 1) / kFixedPointDenominator;
        return toInt();
    }

    constexpr int round() const
    {
        return saturatedSum(m_rawValue, kFixedPointDenominator / 2) >> kLayoutUnitFractionalBits;
    }

    constexpr int floor() const
    {
        if (UNLIKELY(m_rawValue <= std::numeric_limits<int>::min() + kFixedPointDenominator - 1))
            return min();
        return m_rawValue >> kLayoutUnitFractionalBits;
    }

    float ceilToFloat() const
    {
        float floatValue = toFloat();
        if (static_cast<int>(floatValue * kFixedPointDenominator) == m_rawValue)
            return floatValue;
        if (floatValue > 0)
            return std::nextafter(floatValue, std::numeric_limits<float>::max());
        return std::nextafter(floatValue, std::numeric_limits<float>::min());
    }

    constexpr LayoutUnit fraction() const
    {
        // Compute fraction using the mod operator to preserve the sign of the value as it may affect rounding.
        return fromRawValue(m_rawValue % kFixedPointDenominator);
    }

    constexpr bool mightBeSaturated() const { return *this == min() || *this == max(); }

    static float epsilon() { return 1.0f / kFixedPointDenominator; }

    static constexpr LayoutUnit max() { return fromRawValue(std::numeric_limits<int>::max()); }
    static constexpr LayoutUnit min() { return fromRawValue(std::numeric_limits<int>::min()); }

    // Versions of max/min that are slightly smaller/larger than max/min() to allow for rounding without overflowing.
    static const LayoutUnit nearlyMax()
    {
        return fromRawValue(std::numeric_limits<int>::max() - kFixedPointDenominator / 2);
    }
    static const LayoutUnit nearlyMin()
    {
        return fromRawValue(std::numeric_limits<int>::min() + kFixedPointDenominator / 2);
    }

    // FIXME: This seems to do exactly the same thing as the constructor that takes a double.
    static LayoutUnit clamp(double value)
    {
        return clampTo<LayoutUnit>(value, LayoutUnit::min(), LayoutUnit::max());
    }

    friend auto operator<=>(const LayoutUnit&, const LayoutUnit&) = default;

private:
    static bool isInBounds(int value)
    {
        return std::abs(value) <= std::numeric_limits<int>::max() / kFixedPointDenominator;
    }
    static bool isInBounds(unsigned value)
    {
        return value <= static_cast<unsigned>(std::numeric_limits<int>::max()) / kFixedPointDenominator;
    }
    static bool isInBounds(double value)
    {
        return std::abs(value) <= std::numeric_limits<int>::max() / kFixedPointDenominator;
    }

    static constexpr int makeRawValue(int value)
    {
        if (value >= max().toInt())
            return std::numeric_limits<int>::max();
        if (value <= min().toInt())
            return std::numeric_limits<int>::min();
        return value * kFixedPointDenominator;
    }
    static constexpr int makeRawValue(unsigned value)
    {
        if (value >= max().toUnsigned())
            return std::numeric_limits<int>::max();
        return value * kFixedPointDenominator;
    }

    int m_rawValue { 0 };
};

// FIXME: Remove these since LayoutUnit::max() and LayoutUnit::min() are the same values.
constexpr int intMaxForLayoutUnit = LayoutUnit::max();
constexpr int intMinForLayoutUnit = LayoutUnit::min();

constexpr LayoutUnit operator+(LayoutUnit a, LayoutUnit b)
{
    return LayoutUnit::fromRawValue(saturatedSum(a.rawValue(), b.rawValue()));
}

constexpr LayoutUnit operator-(LayoutUnit a, LayoutUnit b)
{
    return LayoutUnit::fromRawValue(saturatedDifference(a.rawValue(), b.rawValue()));
}

constexpr LayoutUnit operator*(LayoutUnit a, LayoutUnit b)
{
    // Saturated arithmetic.
    int64_t product = static_cast<int64_t>(a.rawValue()) * b.rawValue() / kFixedPointDenominator;
    int32_t result = product;
    // If the high 32 bits are not all the same as the high bit of the result, we had overflow.
    if (product >> 32 != result >> 31)
        result = (static_cast<uint32_t>(a.rawValue() ^ b.rawValue()) >> 31) + std::numeric_limits<int>::max();
    return LayoutUnit::fromRawValue(result);
}

constexpr LayoutUnit operator/(LayoutUnit a, LayoutUnit b)
{
    return LayoutUnit::fromRawValue(clampTo<int>(static_cast<long long>(kFixedPointDenominator) * a.rawValue() / b.rawValue()));
}

// For returning the remainder after a division with integer results.
constexpr LayoutUnit intMod(LayoutUnit a, LayoutUnit b)
{
    // This calculates the modulo so that: a = static_cast<int>(a / b) * b + intMod(a, b).
    return LayoutUnit::fromRawValue(a.rawValue() % b.rawValue());
}

inline LayoutUnit operator%(LayoutUnit a, LayoutUnit b)
{
    // This calculates the modulo so that: a = (a / b) * b + a % b.
    long long rawResult = ((static_cast<long long>(kFixedPointDenominator) * a.rawValue()) % b.rawValue()) / kFixedPointDenominator;
    ASSERT(rawResult >= std::numeric_limits<int>::min() && rawResult <= std::numeric_limits<int>::max());
    return LayoutUnit::fromRawValue(rawResult);
}

constexpr bool operator==(LayoutUnit a, int b) { return a.toInt() == b; }
inline bool operator==(LayoutUnit a, float b) { return a.toFloat() == b; }
inline bool operator==(LayoutUnit a, double b) { return a.toDouble() == b; }

constexpr auto operator<=>(LayoutUnit a, int b) { return a.toInt() <=> b; }
inline auto operator<=>(LayoutUnit a, float b) { return a.toFloat() <=> b; }
inline auto operator<=>(LayoutUnit a, double b) { return a.toDouble() <=> b; }

constexpr LayoutUnit operator+(LayoutUnit a, int b) { return a + LayoutUnit(b); }
template<typename NumericType> requires std::is_arithmetic_v<NumericType> LayoutUnit operator+(NumericType a, LayoutUnit b) { return b + a; }

inline float operator+(LayoutUnit a, float b) { return a.toFloat() + b; }
inline double operator+(LayoutUnit a, double b) { return a.toDouble() + b; }
inline float operator+(float a, LayoutUnit b) { return b + a; }
inline double operator+(double a, LayoutUnit b) { return b + a; }

constexpr LayoutUnit operator-(LayoutUnit a, int b) { return a - LayoutUnit(b); }
constexpr LayoutUnit operator-(LayoutUnit a, unsigned b) { return a - LayoutUnit(b); }
constexpr LayoutUnit operator-(int a, LayoutUnit b) { return LayoutUnit(a) - b; }

inline float operator-(LayoutUnit a, float b) { return a.toFloat() - b; }
inline float operator-(float a, LayoutUnit b) { return a - b.toFloat(); }

constexpr LayoutUnit operator-(LayoutUnit a) { return LayoutUnit::fromRawValue(-a.rawValue()); }

template<typename IntegralType> requires std::is_integral_v<IntegralType> constexpr LayoutUnit operator*(LayoutUnit a, IntegralType b) { return a * LayoutUnit(b); }
template<typename IntegralType> requires std::is_integral_v<IntegralType> constexpr LayoutUnit operator*(IntegralType a, LayoutUnit b) { return b * a; }

inline float operator*(LayoutUnit a, float b) { return a.toFloat() * b; }
inline double operator*(LayoutUnit a, double b) { return a.toDouble() * b; }
inline float operator*(float a, LayoutUnit b) { return b * a; }
inline double operator*(double a, LayoutUnit b) { return b * a; }

template<typename IntegralType> requires std::is_integral_v<IntegralType> constexpr LayoutUnit operator/(LayoutUnit a, IntegralType b) { return a / LayoutUnit(b); }
template<typename IntegralType> requires std::is_integral_v<IntegralType> constexpr LayoutUnit operator/(IntegralType a, LayoutUnit b) { return LayoutUnit(a) / b; }

inline float operator/(LayoutUnit a, float b) { return a.toFloat() / b; }
inline double operator/(LayoutUnit a, double b) { return a.toDouble() / b; }
inline float operator/(float a, LayoutUnit b) { return a / b.toFloat(); }
inline double operator/(double a, LayoutUnit b) { return a / b.toDouble(); }

constexpr LayoutUnit operator%(LayoutUnit a, int b) { return a % LayoutUnit(b); }
constexpr LayoutUnit operator%(int a, LayoutUnit b) { return LayoutUnit(a) % b; }

template<typename Type> constexpr LayoutUnit& operator+=(LayoutUnit& a, Type b) { return a = a + b; }
template<typename Type> constexpr LayoutUnit& operator-=(LayoutUnit& a, Type b) { return a = a - b; }
template<typename Type> constexpr LayoutUnit& operator*=(LayoutUnit& a, Type b) { return a = a * b; }
template<typename Type> constexpr LayoutUnit& operator/=(LayoutUnit& a, Type b) { return a = a / b; }

inline float& operator+=(float& a, LayoutUnit b) { return a = a + b; }
inline float& operator-=(float& a, LayoutUnit b) { return a = a - b; }
inline float& operator*=(float& a, LayoutUnit b) { return a = a * b; }
inline float& operator/=(float& a, LayoutUnit b) { return a = a / b; }

WEBCORE_EXPORT WTF::TextStream& operator<<(WTF::TextStream&, LayoutUnit);

constexpr int roundToInt(LayoutUnit value) { return value.round(); }
constexpr int floorToInt(LayoutUnit value) { return value.floor(); }
constexpr LayoutUnit absoluteValue(LayoutUnit value) { return value.abs(); }
constexpr bool isIntegerValue(LayoutUnit value) { return value.toInt() == value; }

void roundToInt(int) = delete;
void floorToInt(int) = delete;
void absoluteValue(int) = delete;
void isIntegerValue(int) = delete;

inline float roundToDevicePixel(LayoutUnit value, float pixelSnappingFactor, bool needsDirectionalRounding = false)
{
    double valueToRound = value.toDouble();
    if (needsDirectionalRounding)
        valueToRound -= LayoutUnit::epsilon() / (2 * kFixedPointDenominator);

    if (valueToRound >= 0)
        return std::round(valueToRound * pixelSnappingFactor) / pixelSnappingFactor;

    // This adjusts directional rounding on negative halfway values. It produces the same direction for both negative and positive values.
    // Instead of rounding negative halfway cases away from zero, we translate them to positive values before rounding.
    // It helps snapping relative negative coordinates to the same position as if they were positive absolute coordinates.
    unsigned translateOrigin = -value.rawValue();
    return std::round((valueToRound + translateOrigin) * pixelSnappingFactor) / pixelSnappingFactor - translateOrigin;
}

inline float floorToDevicePixel(LayoutUnit value, float pixelSnappingFactor)
{
    return std::floor((value.rawValue() * pixelSnappingFactor) / kFixedPointDenominator) / pixelSnappingFactor;
}

inline float ceilToDevicePixel(LayoutUnit value, float pixelSnappingFactor)
{
    return std::ceil((value.rawValue() * pixelSnappingFactor) / kFixedPointDenominator) / pixelSnappingFactor;
}

// FIXME: These are strange functions. They don't mention LayoutUnit at all, but are tied to LayoutUnit's numeric limits; especially strange for rountToInt.
inline int roundToInt(float value) { return roundToInt(LayoutUnit(value)); }
inline float roundToDevicePixel(float value, float pixelSnappingFactor, bool needsDirectionalRounding = false) { return roundToDevicePixel(LayoutUnit(value), pixelSnappingFactor, needsDirectionalRounding); }
inline float floorToDevicePixel(float value, float pixelSnappingFactor) { return floorToDevicePixel(LayoutUnit(value), pixelSnappingFactor); }
inline float ceilToDevicePixel(float value, float pixelSnappingFactor) { return ceilToDevicePixel(LayoutUnit(value), pixelSnappingFactor); }

inline namespace StringLiterals {

constexpr LayoutUnit operator"" _lu(unsigned long long value) { return LayoutUnit(value); }

}

} // namespace WebCore

namespace WTF {

template<> struct DefaultHash<WebCore::LayoutUnit> {
    static unsigned hash(const WebCore::LayoutUnit& p) { return DefaultHash<int>::hash(p.rawValue()); }
    static bool equal(const WebCore::LayoutUnit& a, const WebCore::LayoutUnit& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

// The empty value is min, the deleted value is max. During the course of layout
// these values are typically only used to represent uninitialized values, so they are
// good candidates to represent the deleted and empty values in HashMaps as well.
template<> struct HashTraits<WebCore::LayoutUnit> : GenericHashTraits<WebCore::LayoutUnit> {
    static constexpr bool emptyValueIsZero = false;
    static WebCore::LayoutUnit emptyValue() { return WebCore::LayoutUnit::min(); }
    static void constructDeletedValue(WebCore::LayoutUnit& slot) { slot = WebCore::LayoutUnit::max(); }
    static bool isDeletedValue(WebCore::LayoutUnit value) { return value == WebCore::LayoutUnit::max(); }
};

} // namespace WTF
