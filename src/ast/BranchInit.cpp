/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/BranchInit.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/tinyformat.h"
#include <ostream>
#include <utility>

namespace souffle::ast {

BranchInit::BranchInit(std::string constructor, VecOwn<Argument> args, SrcLocation loc)
        : Term(std::move(args), std::move(loc)), constructor(std::move(constructor)) {}

void BranchInit::print(std::ostream& os) const {
    os << tfm::format("$%s(%s)", constructor, join(args, ", "));
}

bool BranchInit::equal(const Node& node) const {
    const auto& other = asAssert<BranchInit>(node);
    return (constructor == other.constructor) && equal_targets(args, other.args);
}

BranchInit* BranchInit::cloning() const {
    return new BranchInit(constructor, clone(args), getSrcLoc());
}

}  // namespace souffle::ast
