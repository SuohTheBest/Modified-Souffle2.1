/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/Clause.h"
#include "ast/utility/NodeMapper.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include <cassert>
#include <ostream>
#include <utility>

namespace souffle::ast {

Clause::Clause(Own<Atom> head, VecOwn<Literal> bodyLiterals, Own<ExecutionPlan> plan, SrcLocation loc)
        : Node(std::move(loc)), head(std::move(head)), bodyLiterals(std::move(bodyLiterals)),
          plan(std::move(plan)) {
    assert(this->head != nullptr);
    assert(allValidPtrs(this->bodyLiterals));
    // Execution plan can be null
}

Clause::Clause(Own<Atom> head, SrcLocation loc) : Clause(std::move(head), {}, {}, std::move(loc)) {}

Clause::Clause(QualifiedName name, SrcLocation loc) : Clause(mk<Atom>(name), std::move(loc)) {}

void Clause::addToBody(Own<Literal> literal) {
    assert(literal != nullptr);
    bodyLiterals.push_back(std::move(literal));
}
void Clause::addToBody(VecOwn<Literal>&& literals) {
    assert(allValidPtrs(literals));
    bodyLiterals.insert(bodyLiterals.end(), std::make_move_iterator(literals.begin()),
            std::make_move_iterator(literals.end()));
}

void Clause::setHead(Own<Atom> h) {
    assert(h != nullptr);
    head = std::move(h);
}

void Clause::setBodyLiterals(VecOwn<Literal> body) {
    assert(allValidPtrs(body));
    bodyLiterals = std::move(body);
}

std::vector<Literal*> Clause::getBodyLiterals() const {
    return toPtrVector(bodyLiterals);
}

void Clause::setExecutionPlan(Own<ExecutionPlan> plan) {
    this->plan = std::move(plan);
}

void Clause::apply(const NodeMapper& map) {
    head = map(std::move(head));
    mapAll(bodyLiterals, map);
}

Node::NodeVec Clause::getChildNodesImpl() const {
    std::vector<const Node*> res = {head.get()};
    append(res, makePtrRange(bodyLiterals));
    return res;
}

void Clause::print(std::ostream& os) const {
    if (head != nullptr) {
        os << *head;
    }
    if (!bodyLiterals.empty()) {
        os << " :- \n   " << join(bodyLiterals, ",\n   ");
    }
    os << ".";
    if (plan != nullptr) {
        os << *plan;
    }
}

bool Clause::equal(const Node& node) const {
    const auto& other = asAssert<Clause>(node);
    return equal_ptr(head, other.head) && equal_targets(bodyLiterals, other.bodyLiterals) &&
           equal_ptr(plan, other.plan);
}

Clause* Clause::cloning() const {
    return new Clause(clone(head), clone(bodyLiterals), clone(plan), getSrcLoc());
}

}  // namespace souffle::ast
