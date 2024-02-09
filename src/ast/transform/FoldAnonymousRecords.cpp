/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file FoldAnonymousRecords.cpp
 *
 ***********************************************************************/

#include "ast/transform/FoldAnonymousRecords.h"
#include "ast/Argument.h"
#include "ast/BinaryConstraint.h"
#include "ast/BooleanConstraint.h"
#include "ast/Clause.h"
#include "ast/Literal.h"
#include "ast/Program.h"
#include "ast/RecordInit.h"
#include "ast/TranslationUnit.h"
#include "ast/utility/Visitor.h"
#include "souffle/BinaryConstraintOps.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

namespace souffle::ast::transform {

bool FoldAnonymousRecords::isValidRecordConstraint(const Literal* literal) {
    auto constraint = as<BinaryConstraint>(literal);

    if (constraint == nullptr) {
        return false;
    }

    const auto* left = constraint->getLHS();
    const auto* right = constraint->getRHS();

    const auto* leftRecord = as<RecordInit>(left);
    const auto* rightRecord = as<RecordInit>(right);

    // Check if arguments are records records.
    if ((leftRecord == nullptr) || (rightRecord == nullptr)) {
        return false;
    }

    // Check if records are of the same size.
    if (leftRecord->getChildNodes().size() != rightRecord->getChildNodes().size()) {
        return false;
    }

    // Check if operator is "=" or "!="
    auto op = constraint->getBaseOperator();

    return isEqConstraint(op) || isEqConstraint(negatedConstraintOp(op));
}

bool FoldAnonymousRecords::containsValidRecordConstraint(const Clause& clause) {
    bool contains = false;
    visit(clause, [&](const BinaryConstraint& binary) {
        contains = (contains || isValidRecordConstraint(&binary));
    });
    return contains;
}

VecOwn<Literal> FoldAnonymousRecords::expandRecordBinaryConstraint(const BinaryConstraint& constraint) {
    VecOwn<Literal> replacedContraint;

    const auto* left = as<RecordInit>(constraint.getLHS());
    const auto* right = as<RecordInit>(constraint.getRHS());
    assert(left != nullptr && "Non-record passed to record method");
    assert(right != nullptr && "Non-record passed to record method");

    auto leftChildren = left->getArguments();
    auto rightChildren = right->getArguments();

    assert(leftChildren.size() == rightChildren.size());

    // [a, b..] = [c, d...] → a = c, b = d ...
    for (std::size_t i = 0; i < leftChildren.size(); ++i) {
        auto newConstraint = mk<BinaryConstraint>(
                constraint.getBaseOperator(), clone(leftChildren[i]), clone(rightChildren[i]));
        replacedContraint.push_back(std::move(newConstraint));
    }

    // Handle edge case. Empty records.
    if (leftChildren.size() == 0) {
        if (isEqConstraint(constraint.getBaseOperator())) {
            replacedContraint.emplace_back(new BooleanConstraint(true));
        } else {
            replacedContraint.emplace_back(new BooleanConstraint(false));
        }
    }

    return replacedContraint;
}

void FoldAnonymousRecords::transformClause(const Clause& clause, VecOwn<Clause>& newClauses) {
    // If we have an inequality constraint, we need to create new clauses
    // At most one inequality constraint will be expanded in a single pass.
    BinaryConstraint* neqConstraint = nullptr;

    VecOwn<Literal> newBody;
    for (auto* literal : clause.getBodyLiterals()) {
        if (isValidRecordConstraint(literal)) {
            auto& constraint = asAssert<BinaryConstraint>(literal);

            // Simple case, [a_0, ..., a_n] = [b_0, ..., b_n]
            if (isEqConstraint(constraint.getBaseOperator())) {
                auto transformedLiterals = expandRecordBinaryConstraint(constraint);
                std::move(std::begin(transformedLiterals), std::end(transformedLiterals),
                        std::back_inserter(newBody));

                // else if: Case [a_0, ..., a_n] != [b_0, ..., b_n].
                // track single such case, it will be expanded in the end.
            } else if (neqConstraint == nullptr) {
                neqConstraint = as<BinaryConstraint>(literal);

                // Else: repeated inequality.
            } else {
                newBody.push_back(clone(literal));
            }

            // else, we simply copy the literal.
        } else {
            newBody.push_back(clone(literal));
        }
    }

    // If no inequality: create a single modified clause.
    if (neqConstraint == nullptr) {
        auto newClause = clone(clause);
        newClause->setBodyLiterals(std::move(newBody));
        newClauses.emplace_back(std::move(newClause));

        // Else: For each pair in negation, we need an extra clause.
    } else {
        auto transformedLiterals = expandRecordBinaryConstraint(*neqConstraint);

        for (auto it = begin(transformedLiterals); it != end(transformedLiterals); ++it) {
            auto newClause = clone(clause);
            auto copyBody = clone(newBody);
            copyBody.push_back(std::move(*it));

            newClause->setBodyLiterals(std::move(copyBody));

            newClauses.push_back(std::move(newClause));
        }
    }
}

bool FoldAnonymousRecords::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    Program& program = translationUnit.getProgram();

    VecOwn<Clause> newClauses;

    for (const auto* clause : program.getClauses()) {
        if (containsValidRecordConstraint(*clause)) {
            changed = true;
            transformClause(*clause, newClauses);
        } else {
            newClauses.emplace_back(clone(clause));
        }
    }

    // Update Program.
    if (changed) {
        program.setClauses(std::move(newClauses));
    }
    return changed;
}

}  // namespace souffle::ast::transform
