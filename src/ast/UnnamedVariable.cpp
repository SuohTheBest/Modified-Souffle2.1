/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/UnnamedVariable.h"
#include <ostream>

namespace souffle::ast {

void UnnamedVariable::print(std::ostream& os) const {
    os << "_";
}

UnnamedVariable* UnnamedVariable::cloning() const {
    return new UnnamedVariable(getSrcLoc());
}

}  // namespace souffle::ast
