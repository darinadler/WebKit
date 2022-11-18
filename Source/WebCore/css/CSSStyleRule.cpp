/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002-2023 Apple Inc. All rights reserved.
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
#include "CSSStyleRule.h"

#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleSheet.h"
#include "DeclaredStylePropertyMap.h"
#include "PropertySetCSSStyleDeclaration.h"
#include "RuleSet.h"
#include "StyleProperties.h"
#include "StyleRule.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

CSSStyleRule::CSSStyleRule(StyleRule& styleRule, CSSStyleSheet* parent, size_t childRuleCount)
    : CSSRule(parent)
    , m_styleRule(styleRule)
    , m_styleMap(DeclaredStylePropertyMap::create(*this))
    , m_childRuleCSSOMWrappers(childRuleCount)
{
}

Ref<CSSStyleRule> CSSStyleRule::create(StyleRule& rule, CSSStyleSheet* sheet)
{
    return adoptRef(*new CSSStyleRule(rule, sheet, 0));
}

Ref<CSSStyleRule> CSSStyleRule::create(StyleRuleWithNesting& rule, CSSStyleSheet* sheet)
{
    return adoptRef(*new CSSStyleRule(rule, sheet, rule.nestedRules().size()));
}

CSSStyleRule::~CSSStyleRule()
{
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->clearParentRule();
}

CSSStyleDeclaration& CSSStyleRule::style()
{
    if (!m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper = StyleRuleCSSStyleDeclaration::create(m_styleRule->mutableProperties(), *this);
    return *m_propertiesCSSOMWrapper;
}

StylePropertyMap& CSSStyleRule::styleMap()
{
    return m_styleMap.get();
}

String CSSStyleRule::selectorText() const
{
    if (m_cachedSelectorText.isNull())
        m_cachedSelectorText = m_styleRule->selectorList().selectorsText();
    return m_cachedSelectorText;
}

void CSSStyleRule::setSelectorText(const String& selectorText)
{
    // FIXME: getMatchedCSSRules can return CSSStyleRules that are missing parent stylesheet pointer while
    // referencing StyleRules that are part of stylesheet. Disallow mutations in this case.
    if (!parentStyleSheet())
        return;

    CSSParser p(parserContext());
    auto* sheet = parentStyleSheet();
    auto selectorList = p.parseSelector(selectorText, sheet ? &sheet->contents() : nullptr);
    if (!selectorList)
        return;

    // The selector list has to fit into RuleData. <http://webkit.org/b/118369>
    if (selectorList->componentCount() > Style::RuleData::maximumSelectorComponentCount)
        return;

    CSSStyleSheet::RuleMutationScope mutationScope(this);
    m_styleRule->wrapperAdoptSelectorList(WTFMove(*selectorList));

    m_cachedSelectorText = { };
}

Vector<Ref<StyleRuleBase>> CSSStyleRule::nestedRules() const
{
    if (m_styleRule->isStyleRuleWithNesting())
        return downcast<StyleRuleWithNesting>(m_styleRule.get()).nestedRules();

    return { };
}

String CSSStyleRule::serialize(StringBuilder& builder) const
{
    builder.append(selectorText(), " {");

    auto declarations = m_styleRule->properties().asText();

    if (nestedRules().isEmpty()) {
        if (!declarations.isEmpty())
            builder.append(' ', declarations, " }");
        else
            builder.append(" }");
        return;
    }

    builder.append("\n  ", declarations);
    for (auto& nestedRule : nestedRules()) {
        // FIXME: Move serialization out of the wrappers into the actual rules!
        nestedRule->createCSSOMWrapper()->serialize(builder);
        builder.append(text, "\n}");
    }
}

void CSSStyleRule::reattach(StyleRuleBase& rule)
{
    m_styleRule = downcast<StyleRule>(rule);
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->reattach(m_styleRule->mutableProperties());

    m_cachedSelectorText = { };
}

unsigned CSSStyleRule::length() const
{
    return nestedRules().size();
}

CSSRule* CSSStyleRule::item(unsigned index) const
{
    if (index >= length())
        return nullptr;

    ASSERT(m_childRuleCSSOMWrappers.size() == nestedRules().size());

    auto& rule = m_childRuleCSSOMWrappers[index];
    if (!rule)
        rule = nestedRules()[index]->createCSSOMWrapper(const_cast<CSSStyleRule&>(*this));

    return rule.get();
}

CSSRuleList& CSSStyleRule::cssRules() const
{
    if (!m_ruleListCSSOMWrapper)
        m_ruleListCSSOMWrapper = makeUnique<LiveCSSRuleList<CSSStyleRule>>(const_cast<CSSStyleRule&>(*this));

    return *m_ruleListCSSOMWrapper;
}

} // namespace WebCore
