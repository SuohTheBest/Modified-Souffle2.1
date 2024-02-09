/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/BranchDeclaration.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/tinyformat.h"
#include <ostream>
#include <utility>

namespace souffle::ast {

BranchDeclaration::BranchDeclaration(std::string constructor, VecOwn<Attribute> fields, SrcLocation loc)
        : Node(std::move(loc)), constructor(std::move(constructor)), fields(std::move(fields)) {
    assert(allValidPtrs(this->fields));
}

std::vector<Attribute*> BranchDeclaration::getFields() {
    return toPtrVector(fields);
}

void BranchDeclaration::print(std::ostream& os) const {
    os << tfm::format("%s {%s}", constructor, join(fields, ", "));
}

BranchDeclaration* BranchDeclaration::cloning() const {
    return new BranchDeclaration(constructor, clone(fields), getSrcLoc());
}

}  // namespace souffle::ast
