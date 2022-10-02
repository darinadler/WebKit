/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/text/StringBuilder.h>

namespace WebCore {

class CSSSerializer {
public:
    StringBuilder& builder() { return m_builder; }
    
    template<typename FirstArgumentType, typename... OtherArgumentTypes> void append(FirstArgumentType, OtherArgumentTypes ...);
    
    void setUseLegacyPrecision() { m_shouldUseLegacyPrecision = true; }
    bool shouldUseLegacyPrecision() const { return m_shouldUseLegacyPrecision; }
    
private:
    StringBuilder m_builder;
    bool m_shouldUseLegacyPrecision { false };
};

template<typename, typename = void> inline constexpr bool HasSerializeMember = false;
template<typename T> inline constexpr bool HasSerializeMember<T, std::void_t<decltype(&T::serialize)>> = true;

template<typename FirstArgumentType, typename... OtherArgumentTypes> void CSSSerializer::append(FirstArgumentType firstArgument, OtherArgumentTypes ...otherArguments)
{
    if constexpr (HasSerializeMember<FirstArgumentType>)
        firstArgument.serialize(*this);
    else
        m_builder.append(firstArgument);
    append(otherArguments...);
}

} // namespace WebCore
