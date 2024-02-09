/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

#include "ast/TranslationUnit.h"
#include "Global.h"
#include "ast/Program.h"
#include "ast/analysis/PrecedenceGraph.h"
#include "ast/analysis/SCCGraph.h"
#include "reports/DebugReport.h"
#include <sstream>
#include <string>
#include <utility>

namespace souffle::ast {

TranslationUnit::TranslationUnit(Own<Program> program, ErrorReport& e, DebugReport& d)
        : program(std::move(program)), errorReport(e), debugReport(d) {
    assert(this->program != nullptr);
}

TranslationUnit::~TranslationUnit() = default;

/** get analysis: analysis is generated on the fly if not present */
analysis::Analysis* TranslationUnit::addAnalysis(char const* name, Own<analysis::Analysis> analysis) const {
    static const bool debug = Global::config().has("debug-report");
    auto* anaPtr = analysis.get();
    analyses.insert({name, std::move(analysis)});
    anaPtr->run(*this);
    if (debug) {
        std::ostringstream ss;
        std::string strName = name;
        anaPtr->print(ss);
        if (!isA<analysis::PrecedenceGraphAnalysis>(anaPtr) && !isA<analysis::SCCGraphAnalysis>(anaPtr)) {
            debugReport.addSection(strName, "Ast Analysis [" + strName + "]", ss.str());
        } else {
            debugReport.addSection(
                    DebugReportSection(strName, "Ast Analysis [" + strName + "]", {}, ss.str()));
        }
    }
    return anaPtr;
}

void TranslationUnit::invalidateAnalyses() const {
    analyses.clear();
}

}  // namespace souffle::ast
