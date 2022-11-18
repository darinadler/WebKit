/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002-2023 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
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

#include "StyleRuleType.h"
#include <wtf/RefCounted.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

class CSSStyleSheet;
class StyleRuleBase;

struct CSSParserContext;

template<typename> class ExceptionOr;

class CSSRule : public RefCounted<CSSRule> {
public:
    virtual ~CSSRule() = default;

    WEBCORE_EXPORT unsigned short typeForCSSOM() const;

    virtual StyleRuleType styleRuleType() const = 0;
    virtual void serialize(StringBuilder&) const = 0;
    virtual void reattach(StyleRuleBase&) = 0;

    void setParentStyleSheet(CSSStyleSheet*);
    void setParentRule(CSSRule*);
    CSSStyleSheet* parentStyleSheet() const;
    CSSRule* parentRule() const;

    virtual String cssText() const final;
    WEBCORE_EXPORT static ExceptionOr<void> setCssText(const String&);

protected:
    explicit CSSRule(CSSStyleSheet*);

    const CSSParserContext& parserContext() const;

private:
    std::variant<CSSRule*, CSSStyleSheet*> m_parent;
};

inline CSSRule::CSSRule(CSSStyleSheet* parent)
    : m_parent(parent)
{
}

inline void CSSRule::setParentStyleSheet(CSSStyleSheet* styleSheet)
{
    m_parent = styleSheet;
}

inline void CSSRule::setParentRule(CSSRule* rule)
{
    m_parent = rule;
}

inline CSSRule* CSSRule::parentRule() const
{
    return WTF::switchOn(m_parent,
        [](CSSRule* rule) {
            return rule;
        }, [](CSSStyleSheet*) -> CSSRule* {
            return nullptr;
        }
    );
}

inline CSSStyleSheet* CSSRule::parentStyleSheet() const
{
    return WTF::switchOn(m_parent,
        [](CSSRule* rule) {
            return rule ? rule->parentStyleSheet() : nullptr;
        }, [](CSSStyleSheet* sheet) {
            return sheet;
        }
    );
}

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_CSS_RULE(ToValueTypeName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ToValueTypeName) \
    static bool isType(const WebCore::CSSRule& rule) { return rule.styleRuleType() == WebCore::predicate; } \
SPECIALIZE_TYPE_TRAITS_END()
