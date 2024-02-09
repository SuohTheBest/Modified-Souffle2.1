/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Constraint.h
 *
 * Defines an abstract class for constraints
 *
 ***********************************************************************/

#pragma once

#include "ast/Literal.h"

namespace souffle::ast {

/**
 * @class Constraint
 * @brief Abstract class for AST constraints
 */
class Constraint : public Literal {
public:
    using Literal::Literal;
};

}  // namespace souffle::ast
