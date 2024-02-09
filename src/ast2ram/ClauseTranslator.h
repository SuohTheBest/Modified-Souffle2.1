/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ClauseTranslator.h
 *
 * Abstract class providing an interface for translating an
 * ast::Clause into an equivalent ram::Statement.
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"

namespace souffle::ast {
class Clause;
class Relation;
}  // namespace souffle::ast

namespace souffle::ram {
class Statement;
}

namespace souffle::ast2ram {

class TranslatorContext;

class ClauseTranslator {
public:
    ClauseTranslator(const TranslatorContext& context) : context(context) {}
    virtual ~ClauseTranslator() = default;

    virtual Own<ram::Statement> translateNonRecursiveClause(const ast::Clause& clause) = 0;
    virtual Own<ram::Statement> translateRecursiveClause(
            const ast::Clause& clause, const std::set<const ast::Relation*>& scc, std::size_t version) = 0;

protected:
    const TranslatorContext& context;
};

}  // namespace souffle::ast2ram
