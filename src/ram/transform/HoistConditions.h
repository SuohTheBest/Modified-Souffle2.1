/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file HoistConditions.h
 *
 ***********************************************************************/

#pragma once

#include "ram/Program.h"
#include "ram/TranslationUnit.h"
#include "ram/analysis/Level.h"
#include "ram/transform/Transformer.h"
#include <string>

namespace souffle::ram::transform {

/**
 * @class HoistConditionsTransformer
 * @brief Hosts conditions in a loop-nest to the most-outer/semantically-correct loop
 *
 * Hoists the conditions to the earliest point in the loop nest where their
 * evaluation is still semantically correct.
 *
 * The transformations assumes that filter operations are stored verbose,
 * i.e. a conjunction is expressed by two consecutive filter operations.
 * For example ..
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  QUERY
 *   ...
 *    IF C1 /\ C2 then
 *     ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * should be rewritten / or produced by the translator as
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  QUERY
 *   ...
 *    IF C1
 *     IF C2
 *      ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * otherwise the levelling becomes imprecise, i.e., for both conditions
 * the most outer-level is sought rather than considered separately.
 *
 * If there are transformers prior to hoistConditions() that introduce
 * conjunction, another transformer is required that splits the
 * filter operations. However, at the moment this is not necessary
 * because the translator delivers already the right RAM format.
 *
 * TODO: break-up conditions while transforming so that this requirement
 * is removed.
 */
class HoistConditionsTransformer : public Transformer {
public:
    std::string getName() const override {
        return "HoistConditionsTransformer";
    }

    /**
     * @brief Hoist filter operations.
     * @param program that is transformed
     * @return Flag showing whether the program has been changed by the transformation
     *
     * There are two types of conditions in
     * filter operations. The first type depends on tuples of
     * TupleOperation operations. The second type are independent of
     * tuple access. Both types of conditions will be hoisted to
     * the most out-scope such that the program is still valid.
     */
    bool hoistConditions(Program& program);

protected:
    analysis::LevelAnalysis* rla{nullptr};

    bool transform(TranslationUnit& translationUnit) override {
        rla = translationUnit.getAnalysis<analysis::LevelAnalysis>();
        return hoistConditions(translationUnit.getProgram());
    }
};

}  // namespace souffle::ram::transform
