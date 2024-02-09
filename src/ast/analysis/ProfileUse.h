/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProfileUse.h
 *
 * Defines a simple class to query profile data from a profile
 * for profile-guided optimisation.
 *
 ***********************************************************************/

#pragma once

#include "ast/QualifiedName.h"
#include "ast/analysis/Analysis.h"
#include "souffle/profile/ProgramRun.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

namespace souffle::ast {
class TranslationUnit;

namespace analysis {

/**
 * Analysis that loads profile data and has a profile query interface.
 */
class ProfileUseAnalysis : public Analysis {
public:
    /** Name of analysis */
    static constexpr const char* name = "profile-use";

    ProfileUseAnalysis()
            : Analysis(name), programRun(std::make_shared<profile::ProgramRun>(profile::ProgramRun())) {}

    /** Run analysis */
    void run(const TranslationUnit& translationUnit) override;

    /** Output some profile information */
    void print(std::ostream& os) const override;

    /** Check whether the relation size exists in profile */
    bool hasRelationSize(const QualifiedName& rel) const;

    /** Return size of relation in the profile */
    std::size_t getRelationSize(const QualifiedName& rel) const;

private:
    /** performance model of profile run */
    std::shared_ptr<profile::ProgramRun> programRun;
};

}  // namespace analysis
}  // namespace souffle::ast
