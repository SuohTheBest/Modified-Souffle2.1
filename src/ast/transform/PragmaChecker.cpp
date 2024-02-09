/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file PragmaChecker.cpp
 *
 * Defines a transformer that applies pragmas found in parsed input.
 *
 ***********************************************************************/

#include "ast/transform/PragmaChecker.h"
#include "Global.h"
#include "ast/Pragma.h"
#include "ast/Program.h"
#include "ast/TranslationUnit.h"
#include "ast/utility/Visitor.h"
#include <utility>
#include <vector>

namespace souffle::ast::transform {
bool PragmaChecker::transform(TranslationUnit& translationUnit) {
    bool changed = false;
    Program& program = translationUnit.getProgram();

    // Take in pragma options from the datalog file
    visit(program, [&](const Pragma& pragma) {
        std::pair<std::string, std::string> kvp = pragma.getkvp();

        // Command line options take precedence
        if (!Global::config().has(kvp.first)) {
            changed = true;
            Global::config().set(kvp.first, kvp.second);
        }
    });

    return changed;
}
}  // namespace souffle::ast::transform
