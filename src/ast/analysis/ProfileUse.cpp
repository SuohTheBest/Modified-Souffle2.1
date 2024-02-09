/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProfileUse.cpp
 *
 * Implements an Analysis that provides profile information
 * from a profile log file for profile-guided optimisations.
 *
 ***********************************************************************/

#include "ast/analysis/ProfileUse.h"
#include "Global.h"
#include "ast/QualifiedName.h"
#include "souffle/profile/ProgramRun.h"
#include "souffle/profile/Reader.h"
#include "souffle/profile/Relation.h"
#include <limits>
#include <string>

namespace souffle::ast::analysis {

/**
 * Run analysis, i.e., retrieve profile information
 */
void ProfileUseAnalysis::run(const TranslationUnit&) {
    if (Global::config().has("profile-use")) {
        std::string filename = Global::config().get("profile-use");
        profile::Reader(filename, programRun).processFile();
    }
}

/**
 * Print analysis
 */
void ProfileUseAnalysis::print(std::ostream&) const {}

/**
 * Check whether relation size is defined in profile
 */
bool ProfileUseAnalysis::hasRelationSize(const QualifiedName& rel) const {
    return programRun->getRelation(rel.toString()) != nullptr;
}

/**
 * Get relation size from profile
 */
std::size_t ProfileUseAnalysis::getRelationSize(const QualifiedName& rel) const {
    if (const auto* profRel = programRun->getRelation(rel.toString())) {
        return profRel->size();
    } else {
        return std::numeric_limits<std::size_t>::max();
    }
}

}  // namespace souffle::ast::analysis
