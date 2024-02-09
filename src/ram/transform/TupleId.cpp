/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TupleId.cpp
 *
 ***********************************************************************/

#include "ram/transform/TupleId.h"
#include "ram/Expression.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Statement.h"
#include "ram/utility/Visitor.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace souffle::ram::transform {

bool TupleIdTransformer::reorderOperations(Program& program) {
    bool changed = false;
    visit(program, [&](const Query& query) {
        // Maps old tupleIds to new tupleIds
        std::map<int, int> reorder;
        int ctr = 0;

        visit(query, [&](const TupleOperation& search) {
            if (ctr != search.getTupleId()) {
                changed = true;
            }
            reorder[search.getTupleId()] = ctr;
            const_cast<TupleOperation*>(&search)->setTupleId(ctr);
            ctr++;
        });

        std::function<Own<Node>(Own<Node>)> elementRewriter = [&](Own<Node> node) -> Own<Node> {
            if (auto* element = as<TupleElement>(node)) {
                if (reorder[element->getTupleId()] != element->getTupleId()) {
                    changed = true;
                    node = mk<TupleElement>(reorder[element->getTupleId()], element->getElement());
                }
            }
            node->apply(makeLambdaRamMapper(elementRewriter));
            return node;
        };
        const_cast<Query*>(&query)->apply(makeLambdaRamMapper(elementRewriter));
    });

    return changed;
}

}  // namespace souffle::ram::transform
