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
 * Defines the node mapper class
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/MiscUtil.h"
#include <cassert>
#include <utility>

namespace souffle::ast {
class Node;

/**
 * An abstract class for manipulating AST Nodes by substitution
 */
class NodeMapper {
public:
    virtual ~NodeMapper() = default;

    /**
     * Abstract replacement method for a node.
     *
     * If the given nodes is to be replaced, the handed in node
     * will be destroyed by the mapper and the returned node
     * will become owned by the caller.
     */
    virtual Own<Node> operator()(Own<Node> node) const = 0;

    /**
     * Wrapper for any subclass of the AST node hierarchy performing type casts.
     */
    template <typename T>
    Own<T> operator()(Own<T> node) const {
        Own<Node> resPtr = (*this)(Own<Node>(static_cast<Node*>(node.release())));
        assert(isA<T>(resPtr) && "Invalid target node!");
        return Own<T>(as<T>(resPtr.release()));
    }
};

template <typename R>
void mapAll(R& range, NodeMapper const& mapper) {
    for (auto& cur : range) {
        cur = mapper(std::move(cur));
    }
}

}  // namespace souffle::ast
