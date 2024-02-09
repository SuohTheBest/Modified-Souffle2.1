/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/QualifiedName.h"
#include "souffle/utility/StreamUtil.h"
#include <algorithm>
#include <ostream>
#include <sstream>
#include <utility>

namespace souffle::ast {

QualifiedName::QualifiedName() {}
QualifiedName::QualifiedName(std::string name) {
    qualifiers.emplace_back(std::move(name));
}
QualifiedName::QualifiedName(const char* name) : QualifiedName(std::string(name)) {}
QualifiedName::QualifiedName(std::vector<std::string> qualifiers) : qualifiers(std::move(qualifiers)) {}

void QualifiedName::append(std::string name) {
    qualifiers.push_back(std::move(name));
}

void QualifiedName::prepend(std::string name) {
    qualifiers.insert(qualifiers.begin(), std::move(name));
}

/** convert to a string separated by fullstop */
std::string QualifiedName::toString() const {
    std::stringstream ss;
    print(ss);
    return ss.str();
}

bool QualifiedName::operator<(const QualifiedName& other) const {
    return std::lexicographical_compare(
            qualifiers.begin(), qualifiers.end(), other.qualifiers.begin(), other.qualifiers.end());
}

void QualifiedName::print(std::ostream& out) const {
    out << join(qualifiers, ".");
}

std::ostream& operator<<(std::ostream& out, const QualifiedName& id) {
    id.print(out);
    return out;
}

}  // namespace souffle::ast
