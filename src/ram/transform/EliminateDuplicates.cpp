/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file EliminateDuplicates.cpp
 *
 ***********************************************************************/

#include "ram/transform/EliminateDuplicates.h"
#include "ram/Condition.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Statement.h"
#include "ram/utility/Utils.h"
#include "ram/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace souffle::ram::transform {

bool EliminateDuplicatesTransformer::eliminateDuplicates(Program& program) {
    bool changed = false;
    visit(program, [&](const Query& query) {
        std::function<Own<Node>(Own<Node>)> filterRewriter = [&](Own<Node> node) -> Own<Node> {
            if (const Filter* filter = as<Filter>(node)) {
                const Condition* condition = &filter->getCondition();
                VecOwn<Condition> conds = toConjunctionList(condition);
                bool eliminatedDuplicate = false;
                for (std::size_t i = 0; i < conds.size(); i++) {
                    for (std::size_t j = i + 1; j < conds.size(); j++) {
                        if (*conds[i] == *conds[j]) {
                            conds.erase(conds.begin() + j);
                            i = -1;
                            eliminatedDuplicate = true;
                            break;
                        }
                    }
                }
                if (eliminatedDuplicate) {
                    changed = true;
                    node = mk<Filter>(Own<Condition>(toCondition(conds)), clone(filter->getOperation()));
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
