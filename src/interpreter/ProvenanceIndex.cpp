/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ProvenanceIndex.cpp
 *
 * Interpreter index with generic interface.
 *
 ***********************************************************************/

#include "interpreter/Relation.h"

namespace souffle::interpreter {

#define CREATE_PROVENANCE_REL(Structure, Arity, ...)                   \
    case (Arity): {                                                    \
        return mk<Relation<Arity, interpreter::Provenance>>(           \
                id.getAuxiliaryArity(), id.getName(), indexSelection); \
    }

Own<RelationWrapper> createProvenanceRelation(
        const ram::Relation& id, const ram::analysis::IndexCluster& indexSelection) {
    switch (id.getArity()) {
        FOR_EACH_PROVENANCE(CREATE_PROVENANCE_REL);

        default: fatal("Requested arity not yet supported. Feel free to add it.");
    }
}

}  // namespace souffle::interpreter
