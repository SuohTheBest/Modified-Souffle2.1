/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Constraint.h
 *
 * Contains AST Constraint Analysis Infrastructure for doing constraint analysis on AST objects
 *
 ***********************************************************************/

#pragma once

#include "ConstraintSystem.h"
#include "ast/Argument.h"
#include "ast/Clause.h"
#include "ast/Node.h"
#include "ast/Variable.h"
#include "ast/utility/Visitor.h"
#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace souffle::ast::analysis {

/**
 * A variable type to be utilized by AST constraint analysis. Each such variable is
 * associated with an Argument which's property it is describing.
 *
 * @tparam PropertySpace the property space associated to the analysis
 */
template <typename PropertySpace>
struct ConstraintAnalysisVar : public Variable<const Argument*, PropertySpace> {
    explicit ConstraintAnalysisVar(const Argument* arg) : Variable<const Argument*, PropertySpace>(arg) {}
    explicit ConstraintAnalysisVar(const Argument& arg) : Variable<const Argument*, PropertySpace>(&arg) {}

    /** adds print support */
    void print(std::ostream& out) const override {
        out << "var(" << *(this->id) << ")";
    }
};

/**
 * A base class for ConstraintAnalysis collecting constraints for an analysis
 * by visiting every node of a given AST. The collected constraints are
 * then utilized to obtain the desired analysis result.
 *
 * @tparam AnalysisVar the type of variable (and included property space)
 *      to be utilized by this analysis.
 */
template <typename AnalysisVar>
class ConstraintAnalysis : public Visitor<void> {
public:
    using value_type = typename AnalysisVar::property_space::value_type;
    using constraint_type = std::shared_ptr<Constraint<AnalysisVar>>;
    using solution_type = std::map<const Argument*, value_type>;

    virtual void collectConstraints(const Clause& clause) {
        visit(clause, *this);
    }

    /**
     * Runs this constraint analysis on the given clause.
     *
     * @param clause the close to be analysed
     * @param debug a flag enabling the printing of debug information
     * @return an assignment mapping a property to each argument in the given clause
     */
    solution_type analyse(const Clause& clause, std::ostream* debugOutput = nullptr) {
        collectConstraints(clause);

        assignment = constraints.solve();

        // print debug information if desired
        if (debugOutput != nullptr) {
            *debugOutput << "Clause: " << clause << "\n";
            *debugOutput << "Problem:\n" << constraints << "\n";
            *debugOutput << "Solution:\n" << assignment << "\n";
        }

        // convert assignment to result
        solution_type solution;
        visit(clause, [&](const Argument& arg) { solution[&arg] = assignment[getVar(arg)]; });
        return solution;
    }

protected:
    /**
     * A utility function mapping an Argument to its associated analysis variable.
     *
     * @param arg the AST argument to be mapped
     * @return the analysis variable representing its associated value
     */
    AnalysisVar getVar(const Argument& arg) {
        const auto* var = as<ast::Variable>(arg);
        if (var == nullptr) {
            // no mapping required
            return AnalysisVar(arg);
        }

        // filter through map => always take the same variable
        auto res = variables.insert({var->getName(), AnalysisVar(var)}).first;
        return res->second;
    }

    /**
     * A utility function mapping an Argument to its associated analysis variable.
     *
     * @param arg the AST argument to be mapped
     * @return the analysis variable representing its associated value
     */
    AnalysisVar getVar(const Argument* arg) {
        return getVar(*arg);
    }

    /** Adds another constraint to the internally maintained list of constraints */
    void addConstraint(const constraint_type& constraint) {
        constraints.add(constraint);
    }

    Assignment<AnalysisVar> assignment;

    /** The list of constraints making underlying this analysis */
    Problem<AnalysisVar> constraints;

    /** A map mapping variables to unique instances to facilitate the unification of variables */
    std::map<std::string, AnalysisVar> variables;
};

}  // namespace souffle::ast::analysis
