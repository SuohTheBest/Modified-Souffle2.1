/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file DebugReporter.cpp
 *
 * Defines class for adapting other transformers to produce debug output
 *
 ***********************************************************************/

#include "ast/transform/DebugReporter.h"
#include "ast/Program.h"
#include "ast/TranslationUnit.h"
#include "ast/utility/Utils.h"
#include "reports/DebugReport.h"
#include <chrono>

namespace souffle::ast::transform {

bool DebugReporter::transform(TranslationUnit& translationUnit) {
    translationUnit.getDebugReport().startSection();
    auto datalogSpecOriginal = pprint(translationUnit.getProgram());
    auto start = std::chrono::high_resolution_clock::now();
    bool changed = applySubtransformer(translationUnit, wrappedTransformer.get());
    auto end = std::chrono::high_resolution_clock::now();

    if (changed) {
        generateDebugReport(translationUnit, datalogSpecOriginal);
    }

    auto elapsed = std::to_string(std::chrono::duration<double>(end - start).count());
    translationUnit.getDebugReport().endSection(wrappedTransformer->getName(),
            wrappedTransformer->getName() + " (" + elapsed + "s)" + (changed ? "" : " (unchanged)"));
    return changed;
}

void DebugReporter::generateDebugReport(TranslationUnit& tu, const std::string& preTransformDatalog) {
    tu.getDebugReport().addCodeSection(
            "dl", "Datalog", "souffle", preTransformDatalog, pprint(tu.getProgram()));
}

}  // namespace souffle::ast::transform
