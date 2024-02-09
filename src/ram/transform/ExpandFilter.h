/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExpandFilter.h
 *
 ***********************************************************************/

#pragma once

#include "ram/Program.h"
#include "ram/TranslationUnit.h"
#include "ram/transform/Transformer.h"
#include <string>

namespace souffle::ram::transform {

/**
 * @class ExpandFilterTransformer
 * @brief Transforms Conjunctions into consecutive filter operations.
 *
 * For example ..
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  QUERY
 *   ...
 *    IF C1 /\ C2 then
 *     ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * will be rewritten to
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  QUERY
 *   ...
 *    IF C1
 *     IF C2
 *      ...
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */
class ExpandFilterTransformer : public Transformer {
public:
    std::string getName() const override {
        return "ExpandFilterTransformer";
    }

    /**
     * @brief Expand filter operations
     * @param program Program that is transformed
     * @return Flag showing whether the program has been changed by the transformation
     */
    bool expandFilters(Program& program);

protected:
    bool transform(TranslationUnit& translationUnit) override {
        return expandFilters(translationUnit.getProgram());
    }
};

}  // namespace souffle::ram::transform
