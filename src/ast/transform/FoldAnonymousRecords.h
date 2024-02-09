/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file FoldAnonymousRecords.h
 *
 * Defines AST transformation passes.
 *
 ***********************************************************************/

#pragma once

#include "ast/BinaryConstraint.h"
#include "ast/Clause.h"
#include "ast/Literal.h"
#include "ast/TranslationUnit.h"
#include "ast/transform/Transformer.h"
#include "souffle/utility/ContainerUtil.h"
#include <memory>
#include <string>
#include <vector>

namespace souffle::ast::transform {

/**
 * Transformation pass that removes (binary) constraints on the anonymous records.
 * After resolving aliases this is equivalent to completely removing anonymous records.
 *
 * e.g.
 * [a, b, c] = [x, y, z] → a = x, b = y, c = z.
 * [a, b, c] != [x, y, z] →  a != x  b != y  c != z (expanded to three new clauses)
 *
 * In a single pass, in case of equalities  a transformation expands a single level
 * of records in every clause. (e.g. [[a]] = [[1]] => [a] = [1])
 * In case of inequalities, it expands at most a single inequality in every clause
 *
 *
 * This transformation does not resolve aliases.
 * E.g. A = [a, b], A = [c, d]
 * Thus it should be called in conjunction with ResolveAnonymousRecordAliases.
 */
class FoldAnonymousRecords : public Transformer {
public:
    std::string getName() const override {
        return "FoldAnonymousRecords";
    }

private:
    FoldAnonymousRecords* cloning() const override {
        return new FoldAnonymousRecords();
    }

    bool transform(TranslationUnit& translationUnit) override;

    /**
     * Process a single clause.
     *
     * @parem clause Clause to be processed.
     * @param newClauses a destination for the newly produced clauses.
     */
    void transformClause(const Clause& clause, VecOwn<Clause>& newClauses);

    /**
     * Expand constraint on records position-wise.
     *
     * eg.
     * [1, 2, 3] = [a, b, c] => vector(1 = a, 2 = b, 3 = c)
     * [x, y, z] != [a, b, c] => vector(x != a, x != b, z != c)
     *
     * Procedure assumes that argument has a valid operation,
     * that children are of type RecordInit and that the size
     * of both sides is the same
     */
    VecOwn<Literal> expandRecordBinaryConstraint(const BinaryConstraint&);

    /**
     * Determine if the clause contains at least one binary constraint which can be expanded.
     */
    bool containsValidRecordConstraint(const Clause&);

    /**
     * Determine if binary constraint can be expanded.
     */
    bool isValidRecordConstraint(const Literal* literal);
};

}  // namespace souffle::ast::transform
