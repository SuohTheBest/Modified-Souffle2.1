/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SubsetType.h
 *
 * Defines the subset type class
 *
 ***********************************************************************/

#pragma once

#include "ast/QualifiedName.h"
#include "ast/Type.h"
#include "parser/SrcLocation.h"
#include <iosfwd>

namespace souffle::ast {

/**
 * @class SubsetType
 * @brief Defines subset type class
 *
 * Example:
 *    .type A <: B
 */
class SubsetType : public Type {
public:
    SubsetType(QualifiedName name, QualifiedName baseTypeName, SrcLocation loc = {});

    /** Return base type */
    const QualifiedName& getBaseType() const {
        return baseType;
    }

protected:
    void print(std::ostream& os) const override;

private:
    bool equal(const Node& node) const override;

    SubsetType* cloning() const override;

private:
    /** Base type */
    const QualifiedName baseType;
};

}  // namespace souffle::ast
