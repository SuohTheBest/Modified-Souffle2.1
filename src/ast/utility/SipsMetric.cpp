/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SipsMetric.cpp
 *
 * Defines the SipsMetric class, which specifies cost functions for atom orderings in a clause.
 *
 ***********************************************************************/

#include "ast/utility/SipsMetric.h"
#include "ast/Clause.h"
#include "ast/TranslationUnit.h"
#include "ast/Variable.h"
#include "ast/analysis/IOType.h"
#include "ast/analysis/ProfileUse.h"
#include "ast/analysis/RelationDetailCache.h"
#include "ast/utility/BindingStore.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include <cmath>
#include <limits>
#include <vector>

namespace souffle::ast {

std::vector<unsigned int> SipsMetric::getReordering(const Clause* clause) const {
    BindingStore bindingStore(clause);
    auto atoms = getBodyLiterals<Atom>(*clause);
    std::vector<unsigned int> newOrder(atoms.size());

    std::size_t numAdded = 0;
    while (numAdded < atoms.size()) {
        // grab the index of the next atom, based on the SIPS function
        const auto& costs = evaluateCosts(atoms, bindingStore);
        auto minIdx = std::distance(costs.begin(), std::min_element(costs.begin(), costs.end()));
        const auto* nextAtom = atoms[minIdx];
        assert(nextAtom != nullptr && "nullptr atoms should have maximal cost");

        // set all arguments that are variables as bound
        for (const auto* arg : nextAtom->getArguments()) {
            if (const auto* var = as<Variable>(arg)) {
                bindingStore.bindVariableStrongly(var->getName());
            }
        }

        newOrder[numAdded] = minIdx;  // add to the ordering
        atoms[minIdx] = nullptr;      // mark as done
        numAdded++;                   // move on
    }

    return newOrder;
}

/** Create a SIPS metric based on a given heuristic. */
std::unique_ptr<SipsMetric> SipsMetric::create(const std::string& heuristic, const TranslationUnit& tu) {
    if (heuristic == "strict")
        return mk<StrictSips>();
    else if (heuristic == "all-bound")
        return mk<AllBoundSips>();
    else if (heuristic == "naive")
        return mk<NaiveSips>();
    else if (heuristic == "max-bound")
        return mk<MaxBoundSips>();
    else if (heuristic == "max-bound-delta")
        return mk<MaxBoundDeltaSips>();
    else if (heuristic == "max-ratio")
        return mk<MaxRatioSips>();
    else if (heuristic == "least-free")
        return mk<LeastFreeSips>();
    else if (heuristic == "least-free-vars")
        return mk<LeastFreeVarsSips>();
    else if (heuristic == "profile-use")
        return mk<ProfileUseSips>(*tu.getAnalysis<analysis::ProfileUseAnalysis>());
    else if (heuristic == "delta")
        return mk<DeltaSips>();
    else if (heuristic == "input")
        return mk<InputSips>(*tu.getAnalysis<analysis::RelationDetailCacheAnalysis>(),
                *tu.getAnalysis<analysis::IOTypeAnalysis>());
    else if (heuristic == "delta-input")
        return mk<DeltaInputSips>(*tu.getAnalysis<analysis::RelationDetailCacheAnalysis>(),
                *tu.getAnalysis<analysis::IOTypeAnalysis>());

    // default is all-bound
    return create("all-bound", tu);
}

std::vector<double> StrictSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& /* bindingStore */) const {
    // Goal: Always choose the left-most atom
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        cost.push_back(atom == nullptr ? std::numeric_limits<double>::max() : 0);
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> AllBoundSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: Prioritise atoms with all arguments bound
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        cost.push_back(arity == numBound ? 0 : 1);
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> NaiveSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: Prioritise (1) all bound, then (2) atoms with at least one bound argument, then (3) left-most
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            cost.push_back(0);
        } else if (numBound >= 1) {
            cost.push_back(1);
        } else {
            cost.push_back(2);
        }
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> MaxBoundSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise (1) all-bound, then (2) max number of bound vars, then (3) left-most
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            // Always better than anything else
            cost.push_back(0);
        } else if (numBound == 0) {
            // Always worse than any number of bound vars
            cost.push_back(2);
        } else {
            // Between 0 and 1, decreasing with more num bound
            cost.push_back(1 / numBound);
        }
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> MaxBoundDeltaSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise (1) all-bound, then (2) max number of bound vars, then (3) left-most, but use deltas
    // as a tiebreaker between these.
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        // If the atom is a delta, this should act as a tie-breaker for the
        // other conditions. This is small so that it doesn't override the (1 /
        // numBound) factor below.
        auto delta = 0.0001;
        if (isDeltaRelation(atom->getQualifiedName())) {
            delta = 0.0;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            // Always better than anything else
            cost.push_back(delta + 0);
        } else if (numBound == 0) {
            // Always worse than any number of bound vars
            cost.push_back(delta + 3);
        } else {
            // Between 1 and (2 + delta), decreasing with more num bound
            cost.push_back(delta + 1 + (1 / numBound));
        }
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> MaxRatioSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise max ratio of bound args
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == 0) {
            // Always better than anything else
            cost.push_back(0);
        } else if (numBound == 0) {
            // Always worse than anything else
            cost.push_back(2);
        } else {
            // Between 0 and 1, decreasing as the ratio increases
            cost.push_back(1 - numBound / arity);
        }
    }
    assert(atoms.size() == cost.size() && "each atom should have exactly one cost");
    return cost;
}

std::vector<double> LeastFreeSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: choose the atom with the least number of unbound arguments
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        cost.push_back(atom->getArity() - bindingStore.numBoundArguments(atom));
    }
    return cost;
}

std::vector<double> LeastFreeVarsSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: choose the atom with the least amount of unbound variables
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        // use a set to hold all free variables to avoid double-counting
        std::set<std::string> freeVars;
        visit(*atom, [&](const Variable& var) {
            if (bindingStore.isBound(var.getName())) {
                freeVars.insert(var.getName());
            }
        });
        cost.push_back(freeVars.size());
    }
    return cost;
}

std::vector<double> ProfileUseSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: reorder based on the given profiling information
    // Metric: cost(atom_R) = log(|atom_R|) * #free/#args
    //         - exception: propositions are prioritised
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        // prioritise propositions
        int arity = atom->getArity();
        if (arity == 0) {
            cost.push_back(0);
            continue;
        }

        // calculate log(|R|) * #free/#args
        int numBound = bindingStore.numBoundArguments(atom);
        int numFree = arity - numBound;
        double value = log(profileUse.getRelationSize(atom->getQualifiedName()));
        value *= (numFree * 1.0) / arity;
    }
    return cost;
}

std::vector<double> DeltaSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise (1) all-bound, then (2) deltas, and then (3) left-most
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            // prioritise all-bound
            cost.push_back(0);
        } else if (isDeltaRelation(atom->getQualifiedName())) {
            // then deltas
            cost.push_back(1);
        } else {
            cost.push_back(2);
        }
    }
    return cost;
}

std::vector<double> InputSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise (1) all-bound, (2) input, then (3) rest
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        const auto& relName = atom->getQualifiedName();
        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            // prioritise all-bound
            cost.push_back(0);
        } else if (ioTypes.isInput(relDetail.getRelation(relName))) {
            // then input
            cost.push_back(1);
        } else {
            cost.push_back(2);
        }
    }
    return cost;
}

std::vector<double> DeltaInputSips::evaluateCosts(
        const std::vector<Atom*> atoms, const BindingStore& bindingStore) const {
    // Goal: prioritise (1) all-bound, (2) deltas, (3) input, then (4) rest
    std::vector<double> cost;
    for (const auto* atom : atoms) {
        if (atom == nullptr) {
            cost.push_back(std::numeric_limits<double>::max());
            continue;
        }

        const auto& relName = atom->getQualifiedName();
        int arity = atom->getArity();
        int numBound = bindingStore.numBoundArguments(atom);
        if (arity == numBound) {
            // prioritise all-bound
            cost.push_back(0);
        } else if (isDeltaRelation(relName)) {
            // then deltas
            cost.push_back(1);
        } else if (ioTypes.isInput(relDetail.getRelation(relName))) {
            // then input
            cost.push_back(2);
        } else {
            cost.push_back(3);
        }
    }
    return cost;
}

}  // namespace souffle::ast
