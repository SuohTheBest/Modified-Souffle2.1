/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file NodeMapper.h
 *
 * Declaration of RAM node and mappers for RAM nodes
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include <cassert>
#include <memory>

namespace souffle::ram {

class Node;

/**
 * @class NodeMapper
 * @brief An abstract class for manipulating RAM Nodes by substitution
 */
class NodeMapper {
public:
    virtual ~NodeMapper() = default;

    /**
     * @brief Abstract replacement method for a node.
     *
     * If the given nodes is to be replaced, the handed in node
     * will be destroyed by the mapper and the returned node
     * will become owned by the caller.
     */
    virtual Own<Node> operator()(Own<Node> node) const = 0;

    /**
     * @brief Wrapper for any subclass of the RAM node hierarchy performing type casts.
     */
    template <typename T>
    Own<T> operator()(Own<T> node) const {
        Own<Node> resPtr = (*this)(Own<Node>(static_cast<Node*>(node.release())));
        assert(isA<T>(resPtr) && "Invalid target node!");
        return Own<T>(as<T>(resPtr.release()));
    }
};

}  // namespace souffle::ram
