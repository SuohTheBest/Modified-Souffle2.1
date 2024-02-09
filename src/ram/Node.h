/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Node.h
 *
 * Declaration of RAM node
 *
 ***********************************************************************/

#pragma once

#include "ram/utility/LambdaNodeMapper.h"
#include "souffle/utility/MiscUtil.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <utility>
#include <vector>

namespace souffle::ram {

class NodeMapper;

/**
 *  @class Node
 *  @brief Node is a superclass for all RAM IR classes.
 */
class Node {
public:
    Node() = default;
    Node(Node const&) = delete;
    virtual ~Node() = default;
    Node& operator=(Node const&) = delete;

    /**
     * @brief Equivalence check for two RAM nodes
     */
    bool operator==(const Node& other) const {
        return this == &other || (typeid(*this) == typeid(other) && equal(other));
    }

    /**
     * @brief Inequality check for two RAM nodes
     */
    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

    /** @brief Create a clone (i.e. deep copy) of this node as a smart-pointer */
    Own<Node> cloneImpl() const {
        return Own<Node>(cloning());
    }

    /**
     * @brief Apply the mapper to all child nodes
     */
    virtual void apply(const NodeMapper&) {}

    /**
     * @brief Rewrite a child node
     */
    virtual void rewrite(const Node* oldNode, Own<Node> newNode);

    /**
     * @brief Obtain list of all embedded child nodes
     */
    virtual std::vector<const Node*> getChildNodes() const {
        return {};
    }

    /**
     * Print RAM on a stream
     */
    friend std::ostream& operator<<(std::ostream& out, const Node& node) {
        node.print(out);
        return out;
    }

protected:
    /**
     * @brief Print RAM node
     */
    virtual void print(std::ostream& out = std::cout) const = 0;

    /**
     * @brief Equality check for two RAM nodes.
     * Default action is that nothing needs to be checked.
     */
    virtual bool equal(const Node&) const {
        return true;
    }

    /**
     * @brief Create a cloning (i.e. deep copy) of this node
     */
    virtual Node* cloning() const = 0;
};

}  // namespace souffle::ram
