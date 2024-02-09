/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RecursiveClauses.cpp
 *
 * Implements method of precedence graph to build the precedence graph,
 * compute strongly connected components of the precedence graph, and
 * build the strongly connected component graph.
 *
 ***********************************************************************/

#include "ast/analysis/RecursiveClauses.h"
#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/Node.h"
#include "ast/Program.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/RelationDetailCache.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace souffle::ast::analysis {

void RecursiveClausesAnalysis::run(const TranslationUnit& translationUnit) {
    Program& program = translationUnit.getProgram();
    visit(program, [&](const Clause& clause) {
        if (computeIsRecursive(clause, translationUnit)) {
            recursiveClauses.insert(&clause);
        }
    });
}

void RecursiveClausesAnalysis::print(std::ostream& os) const {
    os << recursiveClauses << std::endl;
}

bool RecursiveClausesAnalysis::computeIsRecursive(
        const Clause& clause, const TranslationUnit& translationUnit) const {
    const auto& relationDetail = *translationUnit.getAnalysis<RelationDetailCacheAnalysis>();
    const Program& program = translationUnit.getProgram();

    // we want to reach the atom of the head through the body
    const Relation* trg = getHeadRelation(&clause, &program);

    std::set<const Relation*> reached;
    std::vector<const Relation*> worklist;

    // set up start list
    for (const auto* cur : getBodyLiterals<Atom>(clause)) {
        auto rel = relationDetail.getRelation(cur->getQualifiedName());
        if (rel == trg) {
            return true;
        }
        worklist.push_back(rel);
    }

    // process remaining elements
    while (!worklist.empty()) {
        // get next to process
        const Relation* cur = worklist.back();
        worklist.pop_back();

        // skip null pointers (errors in the input code)
        if (cur == nullptr) {
            continue;
        }

        // check whether this one has been checked before
        if (!reached.insert(cur).second) {
            continue;
        }

        // check all atoms in the relations
        for (const Clause* cl : relationDetail.getClauses(cur)) {
            for (const Atom* at : getBodyLiterals<Atom>(*cl)) {
                auto rel = relationDetail.getRelation(at->getQualifiedName());
                if (rel == trg) {
                    return true;
                }
                worklist.push_back(rel);
            }
        }
    }

    // no cycles found
    return false;
}

}  // namespace souffle::ast::analysis
