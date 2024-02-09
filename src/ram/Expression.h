/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Expression.h
 *
 * Defines a class for evaluating values in the Relational Algebra Machine
 *
 ************************************************************************/

#pragma once

#include "ram/Node.h"

namespace souffle::ram {

/**
 * @class Expression
 * @brief Abstract class for describing scalar values in RAM
 */
class Expression : public Node {
public:
    Expression* cloning() const override = 0;
};

}  // namespace souffle::ram
