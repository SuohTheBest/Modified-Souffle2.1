/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/Type.h"
#include <utility>

namespace souffle::ast {

Type::Type(QualifiedName name, SrcLocation loc) : Node(std::move(loc)), name(std::move(name)) {}

void Type::setQualifiedName(QualifiedName name) {
    this->name = std::move(name);
}

}  // namespace souffle::ast
