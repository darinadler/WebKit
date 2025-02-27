/*
 * Copyright (C) 2024 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CSSUnevaluatedCalc.h"

#include "CSSCalcSymbolTable.h"
#include "CSSNoConversionDataRequiredToken.h"
#include "StyleBuilderState.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {
namespace CSS {

bool unevaluatedCalcEqual(const Ref<CSSCalcValue>& a, const Ref<CSSCalcValue>& b)
{
    return a->equals(b.get());
}

bool unevaluatedCalcRequiresConversionData(const Ref<CSSCalcValue>& calc)
{
    return calc->requiresConversionData();
}

void unevaluatedCalcSerialization(StringBuilder& builder, const Ref<CSSCalcValue>& calc)
{
    builder.append(calc->customCSSText());
}

void unevaluatedCalcCollectComputedStyleDependencies(ComputedStyleDependencies& dependencies, const Ref<CSSCalcValue>& calc)
{
    calc->collectComputedStyleDependencies(dependencies);
}

Ref<CSSCalcValue> unevaluatedCalcSimplify(const Ref<CSSCalcValue>& calc, const CSSToLengthConversionData& conversionData, const CSSCalcSymbolTable& symbolTable)
{
    return calc->copySimplified(conversionData, symbolTable);
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, const Style::BuilderState& state)
{
    return unevaluatedCalcEvaluate(calc, category, state.cssToLengthConversionData(), { });
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, const Style::BuilderState& state, const CSSCalcSymbolTable& symbolTable)
{
    return unevaluatedCalcEvaluate(calc, category, state.cssToLengthConversionData(), symbolTable);
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, const CSSToLengthConversionData& conversionData)
{
    return unevaluatedCalcEvaluate(calc, category, conversionData, { });
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, const CSSToLengthConversionData& conversionData, const CSSCalcSymbolTable& symbolTable)
{
    ASSERT_UNUSED(category, calc->category() == category);
    return calc->doubleValue(conversionData, symbolTable);
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, NoConversionDataRequiredToken token)
{
    return unevaluatedCalcEvaluate(calc, category, token, { });
}

double unevaluatedCalcEvaluate(const Ref<CSSCalcValue>& calc, Calculation::Category category, NoConversionDataRequiredToken token, const CSSCalcSymbolTable& symbolTable)
{
    ASSERT_UNUSED(category, calc->category() == category);
    return calc->doubleValue(token, symbolTable);
}

} // namespace CSS
} // namespace WebCore
