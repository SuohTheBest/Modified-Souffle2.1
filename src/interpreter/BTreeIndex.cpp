/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file BTreeIndex.cpp
 *
 * Interpreter index with generic interface.
 *
 ***********************************************************************/

#include "interpreter/Relation.h"
#include "ram/Relation.h"
#include "ram/analysis/Index.h"
#include "souffle/utility/MiscUtil.h"

namespace souffle::interpreter {

#define CREATE_BTREE_REL(Structure, Arity, ...)                        \
    case (Arity): {                                                    \
        return mk<Relation<Arity, interpreter::Btree>>(                \
                id.getAuxiliaryArity(), id.getName(), indexSelection); \
    }

Own<RelationWrapper> createBTreeRelation(
        const ram::Relation& id, const ram::analysis::IndexCluster& indexSelection) {
    switch (id.getArity()) {
        FOR_EACH_BTREE(CREATE_BTREE_REL);

        default: fatal("Requested arity not yet supported. Feel free to add it.");
    }
}

}  // namespace souffle::interpreter
