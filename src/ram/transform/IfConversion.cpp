/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file IfConversion.cpp
 *
 ***********************************************************************/

#include "ram/transform/IfConversion.h"
#include "ram/Condition.h"
#include "ram/Expression.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Relation.h"
#include "ram/Statement.h"
#include "ram/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace souffle::ram::transform {

Own<Operation> IfConversionTransformer::rewriteIndexScan(const IndexScan* indexScan) {
    // check whether tuple is used in subsequent operations
    bool tupleNotUsed = true;
    visit(*indexScan, [&](const TupleElement& element) {
        if (element.getTupleId() == indexScan->getTupleId()) {
            tupleNotUsed = false;
        }
    });

    // if not used, transform the IndexScan operation to an existence check
    if (tupleNotUsed) {
        // existence check is only supported for equality predicates on each attribute
        std::size_t arity = indexScan->getRangePattern().first.size();
        for (std::size_t i = 0; i < arity; ++i) {
            if (*(indexScan->getRangePattern().first[i]) != *(indexScan->getRangePattern().second[i])) {
                return nullptr;
            }
        }
        // replace IndexScan with an Filter/Existence check
        RamBound newValues = clone(indexScan->getRangePattern().first);

        // check if there is a break statement nested in the Scan - if so, remove it
        Operation* newOp;
        if (const auto* breakOp = as<Break>(indexScan->getOperation())) {
            newOp = breakOp->getOperation().cloning();
        } else {
            newOp = indexScan->getOperation().cloning();
        }

        return mk<Filter>(mk<ExistenceCheck>(indexScan->getRelation(), std::move(newValues)),
                Own<Operation>(newOp), indexScan->getProfileText());
    }
    return nullptr;
}

bool IfConversionTransformer::convertIndexScans(Program& program) {
    bool changed = false;
    visit(program, [&](const Query& query) {
        std::function<Own<Node>(Own<Node>)> scanRewriter = [&](Own<Node> node) -> Own<Node> {
            if (const IndexScan* scan = as<IndexScan>(node)) {
                if (Own<Operation> op = rewriteIndexScan(scan)) {
                    changed = true;
                    node = std::move(op);
                }
            }
            node->apply(makeLambdaRamMapper(scanRewriter));
            return node;
        };
        const_cast<Query*>(&query)->apply(makeLambdaRamMapper(scanRewriter));
    });
    return changed;
}

}  // namespace souffle::ram::transform
