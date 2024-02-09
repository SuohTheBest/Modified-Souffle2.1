/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file NumericConstant.h
 *
 * Defines a class for evaluating values in the Relational Algebra Machine
 *
 ************************************************************************/

#pragma once

#include "ram/Expression.h"
#include "ram/Node.h"
#include "souffle/RamTypes.h"
#include "souffle/utility/MiscUtil.h"

namespace souffle::ram {

/**
 * @class NumericConstant
 * @brief Represents a Ram Constant
 *
 */
class NumericConstant : public Expression {
public:
    /** @brief Get constant */
    RamDomain getConstant() const {
        return constant;
    }

protected:
    explicit NumericConstant(RamDomain constant) : constant(constant) {}

    bool equal(const Node& node) const override {
        const auto& other = asAssert<NumericConstant>(node);
        return constant == other.constant;
    }

    /** Constant value */
    const RamDomain constant;
};

}  // namespace souffle::ram
