/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ReorderLiterals.h
 *
 ***********************************************************************/

#pragma once

#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/TranslationUnit.h"
#include "ast/transform/Transformer.h"
#include <functional>
#include <string>
#include <vector>

namespace souffle::ast {
class BindingStore;
class SipsMetric;
}  // namespace souffle::ast
namespace souffle::ast::transform {

/**
 * Transformation pass to reorder body literals.
 */
class ReorderLiteralsTransformer : public Transformer {
public:
    std::string getName() const override {
        return "ReorderLiteralsTransformer";
    }

    /**
     * Reorder the clause based on a given SIPS function.
     * @param sipsFunction SIPS metric to use
     * @param clause clause to reorder
     * @return nullptr if no change, otherwise a new reordered clause
     */
    static Clause* reorderClauseWithSips(const SipsMetric& sips, const Clause* clause);

private:
    ReorderLiteralsTransformer* cloning() const override {
        return new ReorderLiteralsTransformer();
    }

    bool transform(TranslationUnit& translationUnit) override;
};

}  // namespace souffle::ast::transform
