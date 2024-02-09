/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file HoistConditions.cpp
 *
 ***********************************************************************/

#include "ram/transform/HoistConditions.h"
#include "ram/Condition.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Statement.h"
#include "ram/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace souffle::ram::transform {

bool HoistConditionsTransformer::hoistConditions(Program& program) {
    bool changed = false;

    // helper for collecting conditions from filter operations
    auto addCondition = [](Own<Condition> condition, Own<Condition> c) -> Own<Condition> {
        if (condition == nullptr) {
            return c;
        } else {
            return mk<Conjunction>(std::move(condition), std::move(c));
        }
    };

    // hoist conditions to the most outer scope if they
    // don't depend on TupleOperations
    visit(program, [&](const Query& query) {
        Own<Condition> newCondition;
        std::function<Own<Node>(Own<Node>)> filterRewriter = [&](Own<Node> node) -> Own<Node> {
            if (auto* filter = as<Filter>(node)) {
                const Condition& condition = filter->getCondition();
                // if filter condition is independent of any TupleOperation,
                // delete the filter operation and collect condition
                if (rla->getLevel(&condition) == -1) {
                    changed = true;
                    newCondition = addCondition(std::move(newCondition), clone(condition));
                    node->apply(makeLambdaRamMapper(filterRewriter));
                    return clone(filter->getOperation());
                }
            }
            node->apply(makeLambdaRamMapper(filterRewriter));
            return node;
        };
        auto* mQuery = const_cast<Query*>(&query);
        mQuery->apply(makeLambdaRamMapper(filterRewriter));
        if (newCondition != nullptr) {
            // insert new filter operation at outer-most level of the query
            changed = true;
            auto* nestedOp = const_cast<Operation*>(&mQuery->getOperation());
            mQuery->rewrite(nestedOp, mk<Filter>(std::move(newCondition), clone(nestedOp)));
        }
    });

    // hoist conditions for each TupleOperation operation
    visit(program, [&](const TupleOperation& search) {
        Own<Condition> newCondition;
        std::function<Own<Node>(Own<Node>)> filterRewriter = [&](Own<Node> node) -> Own<Node> {
            if (auto* filter = as<Filter>(node)) {
                const Condition& condition = filter->getCondition();
                // if filter condition matches level of TupleOperation,
                // delete the filter operation and collect condition
                if (rla->getLevel(&condition) == search.getTupleId()) {
                    changed = true;
                    newCondition = addCondition(std::move(newCondition), clone(condition));
                    node->apply(makeLambdaRamMapper(filterRewriter));
                    return clone(filter->getOperation());
                }
            }
            node->apply(makeLambdaRamMapper(filterRewriter));
            return node;
        };
        auto* tupleOp = const_cast<TupleOperation*>(&search);
        tupleOp->apply(makeLambdaRamMapper(filterRewriter));
        if (newCondition != nullptr) {
            // insert new filter operation after the search operation
            changed = true;
            tupleOp->rewrite(&tupleOp->getOperation(),
                    mk<Filter>(std::move(newCondition), clone(tupleOp->getOperation())));
        }
    });
    return changed;
}

}  // namespace souffle::ram::transform
