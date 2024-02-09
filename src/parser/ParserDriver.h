/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ParserDriver.h
 *
 * Defines the parser driver.
 *
 ***********************************************************************/

#pragma once

#include "RelationTag.h"
#include "ast/Clause.h"
#include "ast/Component.h"
#include "ast/ComponentInit.h"
#include "ast/Counter.h"
#include "ast/Directive.h"
#include "ast/FunctorDeclaration.h"
#include "ast/Pragma.h"
#include "ast/QualifiedName.h"
#include "ast/Relation.h"
#include "ast/SubsetType.h"
#include "ast/TranslationUnit.h"
#include "ast/Type.h"
#include "parser/SrcLocation.h"
#include "reports/DebugReport.h"
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace souffle {

class ParserDriver {
public:
    virtual ~ParserDriver() = default;

    Own<ast::TranslationUnit> translationUnit;

    void addRelation(Own<ast::Relation> r);
    void addFunctorDeclaration(Own<ast::FunctorDeclaration> f);
    void addDirective(Own<ast::Directive> d);
    void addType(Own<ast::Type> type);
    void addClause(Own<ast::Clause> c);
    void addComponent(Own<ast::Component> c);
    void addInstantiation(Own<ast::ComponentInit> ci);
    void addPragma(Own<ast::Pragma> p);

    void addIoFromDeprecatedTag(ast::Relation& r);
    Own<ast::SubsetType> mkDeprecatedSubType(
            ast::QualifiedName name, ast::QualifiedName attr, SrcLocation loc);

    std::set<RelationTag> addReprTag(RelationTag tag, SrcLocation tagLoc, std::set<RelationTag> tags);
    std::set<RelationTag> addDeprecatedTag(RelationTag tag, SrcLocation tagLoc, std::set<RelationTag> tags);
    std::set<RelationTag> addTag(RelationTag tag, SrcLocation tagLoc, std::set<RelationTag> tags);
    std::set<RelationTag> addTag(RelationTag tag, std::vector<RelationTag> incompatible, SrcLocation tagLoc,
            std::set<RelationTag> tags);

    Own<ast::Counter> addDeprecatedCounter(SrcLocation tagLoc);

    bool trace_scanning = false;

    Own<ast::TranslationUnit> parse(
            const std::string& filename, FILE* in, ErrorReport& errorReport, DebugReport& debugReport);
    Own<ast::TranslationUnit> parse(
            const std::string& code, ErrorReport& errorReport, DebugReport& debugReport);
    static Own<ast::TranslationUnit> parseTranslationUnit(
            const std::string& filename, FILE* in, ErrorReport& errorReport, DebugReport& debugReport);
    static Own<ast::TranslationUnit> parseTranslationUnit(
            const std::string& code, ErrorReport& errorReport, DebugReport& debugReport);

    void warning(const SrcLocation& loc, const std::string& msg);
    void error(const SrcLocation& loc, const std::string& msg);
    void error(const std::string& msg);
};

}  // end of namespace souffle
