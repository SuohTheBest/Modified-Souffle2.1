/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/Counter.h"
#include <ostream>

namespace souffle::ast {

void Counter::print(std::ostream& os) const {
    os << "$";
}

Counter* Counter::cloning() const {
    return new Counter(getSrcLoc());
}

}  // namespace souffle::ast
