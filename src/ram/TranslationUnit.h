/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TranslationUnit.h
 *
 * Define a RAM translation unit
 *
 ***********************************************************************/

#pragma once

#include "Global.h"
#include "ram/Program.h"
#include "ram/analysis/Analysis.h"
#include "reports/DebugReport.h"
#include "reports/ErrorReport.h"
#include <cassert>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

namespace souffle {

namespace ram {

/**
 * @class TranslationUnit
 * @brief Translating a RAM program
 *
 * Comprises the program, symbol table, error report, debug report, and analyses
 */
class TranslationUnit {
public:
    TranslationUnit(Own<Program> prog, ErrorReport& e, DebugReport& d)
            : program(std::move(prog)), errorReport(e), debugReport(d) {
        assert(program != nullptr && "program is a null-pointer");
    }

    virtual ~TranslationUnit() = default;

    /** @brief Get an analysis */
    template <class Analysis>
    Analysis* getAnalysis() const {
        static const bool debug = Global::config().has("debug-report");
        std::string name = Analysis::name;
        auto it = analyses.find(name);
        if (it == analyses.end()) {
            // analysis does not exist yet, create instance and run it.
            auto analysis = mk<Analysis>(Analysis::name);
            analysis->run(*this);
            // output analysis in debug report
            if (debug) {
                std::stringstream ramAnalysisStr;
                ramAnalysisStr << *analysis;
                if (!ramAnalysisStr.str().empty()) {
                    debugReport.addSection(
                            analysis->getName(), "RAM Analysis " + analysis->getName(), ramAnalysisStr.str());
                }
            }
            // check it hasn't been created by someone else, and insert if not
            it = analyses.find(name);
            if (it == analyses.end()) {
                analyses[name] = std::move(analysis);
            }
        }
        return as<Analysis>(analyses[name]);
    }

    /** @brief Get all alive analyses */
    std::set<const analysis::Analysis*> getAliveAnalyses() const {
        std::set<const analysis::Analysis*> result;
        for (auto const& a : analyses) {
            result.insert(a.second.get());
        }
        return result;
    }

    /** @brief Invalidate all alive analyses of the translation unit */
    void invalidateAnalyses() {
        analyses.clear();
    }

    /** @brief Get the RAM Program of the translation unit  */
    Program& getProgram() const {
        return *program;
    }

    /** @brief Obtain error report */
    ErrorReport& getErrorReport() {
        return errorReport;
    }

    /** @brief Obtain debug report */
    DebugReport& getDebugReport() {
        return debugReport;
    }

protected:
    /* Cached analyses */
    mutable std::map<std::string, Own<analysis::Analysis>> analyses;

    /* RAM program */
    Own<Program> program;

    /* Error report for raising errors and warnings */
    ErrorReport& errorReport;

    /* Debug report for logging information */
    DebugReport& debugReport;
};

}  // namespace ram
}  // end of namespace souffle
