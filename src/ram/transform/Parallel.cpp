/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Parallel.cpp
 *
 ***********************************************************************/

#include "ram/transform/Parallel.h"
#include "ram/Condition.h"
#include "ram/Expression.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Relation.h"
#include "ram/Statement.h"
#include "ram/utility/Visitor.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace souffle::ram::transform {

bool ParallelTransformer::parallelizeOperations(Program& program) {
    bool changed = false;

    // parallelize the most outer loop only
    // most outer loops can be scan/if-exists/indexScan/indexIfExists
    visit(program, [&](const Query& query) {
        std::function<Own<Node>(Own<Node>)> parallelRewriter = [&](Own<Node> node) -> Own<Node> {
            if (const Scan* scan = as<Scan>(node)) {
                const Relation& rel = relAnalysis->lookup(scan->getRelation());
                if (scan->getTupleId() == 0 && rel.getArity() > 0) {
                    if (!isA<Insert>(&scan->getOperation())) {
                        changed = true;
                        return mk<ParallelScan>(scan->getRelation(), scan->getTupleId(),
                                clone(scan->getOperation()), scan->getProfileText());
                    }
                }
            } else if (const IfExists* ifexists = as<IfExists>(node)) {
                if (ifexists->getTupleId() == 0) {
                    changed = true;
                    return mk<ParallelIfExists>(ifexists->getRelation(), ifexists->getTupleId(),
                            clone(ifexists->getCondition()), clone(ifexists->getOperation()),
                            ifexists->getProfileText());
                }
            } else if (const IndexScan* indexScan = as<IndexScan>(node)) {
                if (indexScan->getTupleId() == 0) {
                    changed = true;
                    RamPattern queryPattern = clone(indexScan->getRangePattern());
                    return mk<ParallelIndexScan>(indexScan->getRelation(), indexScan->getTupleId(),
                            std::move(queryPattern), clone(indexScan->getOperation()),
                            indexScan->getProfileText());
                }
            } else if (const IndexIfExists* indexIfExists = as<IndexIfExists>(node)) {
                if (indexIfExists->getTupleId() == 0) {
                    changed = true;
                    RamPattern queryPattern = clone(indexIfExists->getRangePattern());
                    return mk<ParallelIndexIfExists>(indexIfExists->getRelation(),
                            indexIfExists->getTupleId(), clone(indexIfExists->getCondition()),
                            std::move(queryPattern), clone(indexIfExists->getOperation()),
                            indexIfExists->getProfileText());
                }
            } else if (const Aggregate* aggregate = as<Aggregate>(node)) {
                const Relation& rel = relAnalysis->lookup(aggregate->getRelation());
                if (aggregate->getTupleId() == 0 && !rel.isNullary()) {
                    changed = true;
                    return mk<ParallelAggregate>(Own<Operation>(aggregate->getOperation().cloning()),
                            aggregate->getFunction(), aggregate->getRelation(),
                            Own<Expression>(aggregate->getExpression().cloning()),
                            Own<Condition>(aggregate->getCondition().cloning()), aggregate->getTupleId());
                }
            } else if (const IndexAggregate* indexAggregate = as<IndexAggregate>(node)) {
                const Relation& rel = relAnalysis->lookup(indexAggregate->getRelation());
                if (indexAggregate->getTupleId() == 0 && !rel.isNullary()) {
                    changed = true;
                    RamPattern queryPattern = clone(indexAggregate->getRangePattern());
                    return mk<ParallelIndexAggregate>(
                            Own<Operation>(indexAggregate->getOperation().cloning()),
                            indexAggregate->getFunction(), indexAggregate->getRelation(),
                            Own<Expression>(indexAggregate->getExpression().cloning()),
                            Own<Condition>(indexAggregate->getCondition().cloning()), std::move(queryPattern),
                            indexAggregate->getTupleId());
                }
            }
            node->apply(makeLambdaRamMapper(parallelRewriter));
            return node;
        };
        // guardedInsert cannot be parallelized
        bool isGuardedInsert = false;
        visit(query, [&](const GuardedInsert&) { isGuardedInsert = true; });
        if (isGuardedInsert == false) {
            const_cast<Query*>(&query)->apply(makeLambdaRamMapper(parallelRewriter));
        }
    });
    return changed;
}

}  // namespace souffle::ram::transform
