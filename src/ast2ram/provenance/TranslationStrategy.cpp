/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TranslationStrategy.cpp
 *
 ***********************************************************************/

#include "ast2ram/provenance/TranslationStrategy.h"
#include "ast2ram/provenance/ClauseTranslator.h"
#include "ast2ram/provenance/ConstraintTranslator.h"
#include "ast2ram/provenance/UnitTranslator.h"
#include "ast2ram/seminaive/ValueTranslator.h"
#include "ast2ram/utility/TranslatorContext.h"
#include "ram/Condition.h"
#include "ram/Expression.h"

namespace souffle::ast2ram::provenance {

ast2ram::UnitTranslator* TranslationStrategy::createUnitTranslator() const {
    return new UnitTranslator();
}

ast2ram::ClauseTranslator* TranslationStrategy::createClauseTranslator(
        const TranslatorContext& context) const {
    return new ClauseTranslator(context);
}

ast2ram::ConstraintTranslator* TranslationStrategy::createConstraintTranslator(
        const TranslatorContext& context, const ValueIndex& index) const {
    return new ConstraintTranslator(context, index);
}

ast2ram::ValueTranslator* TranslationStrategy::createValueTranslator(
        const TranslatorContext& context, const ValueIndex& index) const {
    return new ast2ram::seminaive::ValueTranslator(context, index);
}

}  // namespace souffle::ast2ram::provenance
