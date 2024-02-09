/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RelationSchedule.cpp
 *
 * Implements method of precedence graph to build the precedence graph,
 * compute strongly connected components of the precedence graph, and
 * build the strongly connected component graph.
 *
 ***********************************************************************/

#include "ast/analysis/RelationSchedule.h"
#include "GraphUtils.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/PrecedenceGraph.h"
#include "ast/analysis/SCCGraph.h"
#include "ast/analysis/TopologicallySortedSCCGraph.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <set>

namespace souffle::ast::analysis {

void RelationScheduleAnalysisStep::print(std::ostream& os) const {
    os << "computed: ";
    for (const Relation* compRel : computed()) {
        os << compRel->getQualifiedName() << ", ";
    }
    os << "\nexpired: ";
    for (const Relation* compRel : expired()) {
        os << compRel->getQualifiedName() << ", ";
    }
    os << "\n";
    if (recursive()) {
        os << "recursive";
    } else {
        os << "not recursive";
    }
    os << "\n";
}

void RelationScheduleAnalysis::run(const TranslationUnit& translationUnit) {
    topsortSCCGraphAnalysis = translationUnit.getAnalysis<TopologicallySortedSCCGraphAnalysis>();
    precedenceGraph = translationUnit.getAnalysis<PrecedenceGraphAnalysis>();

    std::size_t numSCCs = translationUnit.getAnalysis<SCCGraphAnalysis>()->getNumberOfSCCs();
    std::vector<std::set<const Relation*>> relationExpirySchedule =
            computeRelationExpirySchedule(translationUnit);

    relationSchedule.clear();
    for (std::size_t i = 0; i < numSCCs; i++) {
        auto scc = topsortSCCGraphAnalysis->order()[i];
        const std::set<const Relation*> computedRelations =
                translationUnit.getAnalysis<SCCGraphAnalysis>()->getInternalRelations(scc);
        relationSchedule.emplace_back(computedRelations, relationExpirySchedule[i],
                translationUnit.getAnalysis<SCCGraphAnalysis>()->isRecursive(scc));
    }
}

std::vector<std::set<const Relation*>> RelationScheduleAnalysis::computeRelationExpirySchedule(
        const TranslationUnit& translationUnit) {
    std::vector<std::set<const Relation*>> relationExpirySchedule;
    /* Compute for each step in the reverse topological order
       of evaluating the SCC the set of alive relations. */
    std::size_t numSCCs = topsortSCCGraphAnalysis->order().size();

    /* Alive set for each step */
    std::vector<std::set<const Relation*>> alive(numSCCs);
    /* Resize expired relations sets */
    relationExpirySchedule.resize(numSCCs);
    const auto& sccGraph = translationUnit.getAnalysis<SCCGraphAnalysis>();

    /* Compute all alive relations by iterating over all steps in reverse order
       determine the dependencies */
    for (std::size_t orderedSCC = 1; orderedSCC < numSCCs; orderedSCC++) {
        /* Add alive set of previous step */
        alive[orderedSCC].insert(alive[orderedSCC - 1].begin(), alive[orderedSCC - 1].end());

        /* Add predecessors of relations computed in this step */
        auto scc = topsortSCCGraphAnalysis->order()[numSCCs - orderedSCC];
        for (const Relation* r : sccGraph->getInternalRelations(scc)) {
            for (const Relation* predecessor : precedenceGraph->graph().predecessors(r)) {
                alive[orderedSCC].insert(predecessor);
            }
        }

        /* Compute expired relations in reverse topological order using the set difference of the alive sets
           between steps. */
        std::set_difference(alive[orderedSCC].begin(), alive[orderedSCC].end(), alive[orderedSCC - 1].begin(),
                alive[orderedSCC - 1].end(),
                std::inserter(relationExpirySchedule[numSCCs - orderedSCC],
                        relationExpirySchedule[numSCCs - orderedSCC].end()));
    }

    return relationExpirySchedule;
}

void RelationScheduleAnalysis::print(std::ostream& os) const {
    os << "begin schedule\n";
    for (const RelationScheduleAnalysisStep& step : relationSchedule) {
        os << step;
        os << "computed: ";
        for (const Relation* compRel : step.computed()) {
            os << compRel->getQualifiedName() << ", ";
        }
        os << "\nexpired: ";
        for (const Relation* compRel : step.expired()) {
            os << compRel->getQualifiedName() << ", ";
        }
        os << "\n";
        if (step.recursive()) {
            os << "recursive";
        } else {
            os << "not recursive";
        }
        os << "\n";
    }
    os << "end schedule\n";
}

}  // namespace souffle::ast::analysis
