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
 * Declaration of RAM node and mappers for RAM nodes
 *
 ***********************************************************************/

#pragma once

#include "ram/utility/NodeMapper.h"
#include "souffle/utility/ContainerUtil.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <utility>
#include <vector>

namespace souffle::ram {

class Node;

/**
 * @class LambdaNodeMapper
 * @brief A special NodeMapper wrapping a lambda conducting node transformations.
 */
template <typename Lambda>
class LambdaNodeMapper : public NodeMapper {
    const Lambda& lambda;

public:
    /**
     * @brief Constructor for LambdaNodeMapper
     */
    LambdaNodeMapper(const Lambda& lambda) : lambda(lambda) {}

    /**
     * @brief Applies lambda
     */
    Own<Node> operator()(Own<Node> node) const override {
        Own<Node> result = lambda(std::move(node));
        assert(result != nullptr && "null-pointer in lambda ram-node mapper");
        return result;
    }
};

/**
 * @brief Creates a node mapper based on a corresponding lambda expression.
 */
template <typename Lambda>
LambdaNodeMapper<Lambda> makeLambdaRamMapper(const Lambda& lambda) {
    return LambdaNodeMapper<Lambda>(lambda);
}

}  // namespace souffle::ram
