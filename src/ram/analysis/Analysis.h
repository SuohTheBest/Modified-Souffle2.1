/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Analysis.h
 *
 * Defines an interface for RAM analysis
 *
 ***********************************************************************/

#pragma once

#include <ostream>
#include <string>

namespace souffle::ram {

class TranslationUnit;

namespace analysis {

/**
 * @class Analysis
 * @brief Abstract class for a RAM Analysis.
 */
class Analysis {
public:
    Analysis(const char* id) : identifier(id) {}
    virtual ~Analysis() = default;

    /** @brief get name of the analysis */
    virtual const std::string& getName() const {
        return identifier;
    }

    /** @brief Run analysis for a RAM translation unit */
    virtual void run(const TranslationUnit& translationUnit) = 0;

    /** @brief Print the analysis result in HTML format */
    virtual void print(std::ostream& /* os */) const {}

    /** @brief define output stream operator */
    friend std::ostream& operator<<(std::ostream& out, const Analysis& other) {
        other.print(out);
        return out;
    }

protected:
    /** @brief name of analysis instance */
    std::string identifier;
};

}  // namespace analysis
}  // namespace souffle::ram
