/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RemoveRedundantRelations.cpp
 *
 ***********************************************************************/

#include "ast/transform/RemoveRedundantRelations.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/RedundantRelations.h"
#include "ast/utility/Utils.h"
#include <set>

namespace souffle::ast::transform {

bool RemoveRedundantRelationsTransformer::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    auto* redundantRelationsAnalysis = translationUnit.getAnalysis<analysis::RedundantRelationsAnalysis>();
    std::set<QualifiedName> redundantRelations = redundantRelationsAnalysis->getRedundantRelations();
    for (auto name : redundantRelations) {
        removeRelation(translationUnit, name);
        changed = true;
    }
    return changed;
}

}  // namespace souffle::ast::transform
