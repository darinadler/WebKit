/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "CSSLayerBlockRule.h"

#include "CSSMarkup.h"
#include "CSSStyleSheet.h"
#include "StyleRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

CSSLayerBlockRule::CSSLayerBlockRule(StyleRuleLayer& rule, CSSStyleSheet* parent)
    : CSSGroupingRule(rule, parent)
{
    ASSERT(!rule.isStatement());
}

Ref<CSSLayerBlockRule> CSSLayerBlockRule::create(StyleRuleLayer& rule, CSSStyleSheet* parent)
{
    return adoptRef(*new CSSLayerBlockRule(rule, parent));
}

void CSSLayerBlockRule::serialize(StringBuilder& builder) const
{
    builder.append("@layer"_s);
    serializeName(builder, " "_s);
    serializeItems(builder);
}

String CSSLayerBlockRule::name() const
{
    StringBuilder builder;
    serializeName(builder, ""_s);
    return builder.toString();
}

void serialize(StringBuilder& builder, const CascadeLayerName& name)
{
    auto separator = ""_s;
    for (auto& segment : name) {
        builder.append(std::exchange(separator, "."_s));
        serializeIdentifier(segment, builder);
    }
}

void CSSLayerBlockRule::serializeName(StringBuilder& builder, ASCIILiteral prefix) const
{
    auto& layer = downcast<StyleRuleLayer>(groupRule());
    if (layer.name().isEmpty())
        return;

    builder.append(prefix);
    WebCore::serialize(builder, layer.name());
}

String stringFromCascadeLayerName(const CascadeLayerName& name)
{
    StringBuilder builder;
    serialize(builder, name);
    return builder.toString();
}

} // namespace WebCore

