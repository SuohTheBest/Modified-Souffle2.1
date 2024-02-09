/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ReorderFilterBreak.cpp
 *
 ***********************************************************************/

#include "ram/transform/ReorderFilterBreak.h"
#include "ram/Condition.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Statement.h"
#include "ram/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <functional>
#include <memory>
#include <vector>

namespace souffle::ram::transform {

bool ReorderFilterBreak::reorderFilterBreak(Program& program) {
    bool changed = false;
    visit(program, [&](const Query& query) {
        std::function<Own<Node>(Own<Node>)> filterRewriter = [&](Own<Node> node) -> Own<Node> {
            // find filter-break nesting
            if (const Filter* filter = as<Filter>(node)) {
                if (const Break* br = as<Break>(filter->getOperation())) {
                    changed = true;
                    // convert to break-filter nesting
                    node = mk<Break>(clone(br->getCondition()),
                            mk<Filter>(clone(filter->getCondition()), clone(br->getOperation())));
                }
            }
            node->apply(makeLambdaRamMapper(filterRewriter));
            return node;
        };
        const_cast<Query*>(&query)->apply(makeLambdaRamMapper(filterRewriter));
    });
    return changed;
}

}  // namespace souffle::ram::transform
