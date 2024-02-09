/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file LambdaNodeMapper.h
 *
 * Defines lambda node mappers class
 *
 ***********************************************************************/

#pragma once

#include "ast/utility/NodeMapper.h"
#include <memory>
#include <utility>

namespace souffle::ast {

namespace detail {

/**
 * A special NodeMapper wrapping a lambda conducting node transformations.
 */
template <typename Lambda>
class LambdaNodeMapper : public NodeMapper {
    const Lambda& lambda;

public:
    LambdaNodeMapper(const Lambda& lambda) : lambda(lambda) {}

    Own<Node> operator()(Own<Node> node) const override {
        return lambda(std::move(node));
    }
};
}  // namespace detail

/**
 * Creates a node mapper based on a corresponding lambda expression.
 */
template <typename Lambda>
detail::LambdaNodeMapper<Lambda> makeLambdaAstMapper(const Lambda& lambda) {
    return detail::LambdaNodeMapper<Lambda>(lambda);
}

}  // namespace souffle::ast
