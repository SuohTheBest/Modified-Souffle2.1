/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2021, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file RemoveEmptyRelations.cpp
 *
 ***********************************************************************/

#include "ast/transform/RemoveEmptyRelations.h"
#include "ast/Aggregator.h"
#include "ast/Atom.h"
#include "ast/Clause.h"
#include "ast/Literal.h"
#include "ast/Negation.h"
#include "ast/Program.h"
#include "ast/Relation.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/IOType.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace souffle::ast::transform {

bool RemoveEmptyRelationsTransformer::removeEmptyRelations(TranslationUnit& translationUnit) {
    Program& program = translationUnit.getProgram();
    std::set<QualifiedName> emptyRelations;
    bool changed = false;
    for (auto rel : program.getRelations()) {
        auto* ioTypes = translationUnit.getAnalysis<analysis::IOTypeAnalysis>();
        if (!getClauses(program, *rel).empty() || ioTypes->isInput(rel)) {
            continue;
        }
        emptyRelations.insert(rel->getQualifiedName());

        bool usedInAggregate = false;
        visit(program, [&](const Aggregator& agg) {
            for (const auto lit : agg.getBodyLiterals()) {
                visit(*lit, [&](const Atom& atom) {
                    if (getAtomRelation(&atom, &program) == rel) {
                        usedInAggregate = true;
                    }
                });
            }
        });

        if (!usedInAggregate && !ioTypes->isOutput(rel)) {
            removeRelation(translationUnit, rel->getQualifiedName());
            changed = true;
        }
    }

    for (const auto& relName : emptyRelations) {
        changed |= removeEmptyRelationUses(translationUnit, relName);
    }

    return changed;
}

bool RemoveEmptyRelationsTransformer::removeEmptyRelationUses(
        TranslationUnit& translationUnit, const QualifiedName& emptyRelationName) {
    Program& program = translationUnit.getProgram();
    bool changed = false;

    //
    // (1) drop rules from the program that have empty relations in their bodies.
    // (2) drop negations of empty relations
    //
    // get all clauses
    std::vector<const Clause*> clauses;
    visit(program, [&](const Clause& cur) { clauses.push_back(&cur); });

    // clean all clauses
    for (const Clause* cl : clauses) {
        // check for an atom whose relation is the empty relation

        bool removed = false;
        for (Literal* lit : cl->getBodyLiterals()) {
            if (auto* arg = as<Atom>(lit)) {
                if (arg->getQualifiedName() == emptyRelationName) {
                    program.removeClause(cl);
                    removed = true;
                    changed = true;
                    break;
                }
            }
        }

        if (!removed) {
            // check whether a negation with empty relations exists

            bool rewrite = false;
            for (Literal* lit : cl->getBodyLiterals()) {
                if (auto* neg = as<Negation>(lit)) {
                    if (neg->getAtom()->getQualifiedName() == emptyRelationName) {
                        rewrite = true;
                        break;
                    }
                }
            }

            if (rewrite) {
                // clone clause without negation for empty relations

                auto res = cloneHead(*cl);

                for (Literal* lit : cl->getBodyLiterals()) {
                    if (auto* neg = as<Negation>(lit)) {
                        if (neg->getAtom()->getQualifiedName() != emptyRelationName) {
                            res->addToBody(clone(lit));
                        }
                    } else {
                        res->addToBody(clone(lit));
                    }
                }

                program.removeClause(cl);
                program.addClause(std::move(res));
                changed = true;
            }
        }
    }

    return changed;
}

}  // namespace souffle::ast::transform
