/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file TupleOperation.h
 *
 ***********************************************************************/

#pragma once

#include "ram/NestedOperation.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "souffle/utility/MiscUtil.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace souffle::ram {

/**
 * @class TupleOperation
 * @brief Abstract class for relation searches and lookups
 */
class TupleOperation : public NestedOperation {
public:
    TupleOperation(int ident, Own<Operation> nested, std::string profileText = "")
            : NestedOperation(std::move(nested), std::move(profileText)), identifier(ident) {}

    TupleOperation* cloning() const override = 0;

    /** @brief Get identifier */
    int getTupleId() const {
        return identifier;
    }

    /** @brief Set identifier */
    void setTupleId(int id) {
        identifier = id;
    }

    std::vector<const Node*> getChildNodes() const override {
        return NestedOperation::getChildNodes();
    }

protected:
    bool equal(const Node& node) const override {
        const auto& other = asAssert<TupleOperation>(node);
        return NestedOperation::equal(other) && identifier == other.identifier;
    }

    /**
     * Identifier for the tuple, corresponding to
     * its position in the loop nest
     */
    int identifier;
};

}  // namespace souffle::ram
