/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file SemanticChecker.cpp
 *
 * Implementation of the semantic checker pass.
 *
 ***********************************************************************/

#include "ast/transform/SemanticChecker.h"
#include "AggregateOp.h"
#include "FunctorOps.h"
#include "Global.h"
#include "GraphUtils.h"
#include "RelationTag.h"
#include "ast/Aggregator.h"
#include "ast/AlgebraicDataType.h"
#include "ast/Argument.h"
#include "ast/Atom.h"
#include "ast/Attribute.h"
#include "ast/BinaryConstraint.h"
#include "ast/BranchDeclaration.h"
#include "ast/BranchInit.h"
#include "ast/Clause.h"
#include "ast/Constant.h"
#include "ast/Counter.h"
#include "ast/Directive.h"
#include "ast/ExecutionOrder.h"
#include "ast/ExecutionPlan.h"
#include "ast/Functor.h"
#include "ast/IntrinsicFunctor.h"
#include "ast/Literal.h"
#include "ast/Negation.h"
#include "ast/NilConstant.h"
#include "ast/Node.h"
#include "ast/NumericConstant.h"
#include "ast/Program.h"
#include "ast/QualifiedName.h"
#include "ast/RecordInit.h"
#include "ast/RecordType.h"
#include "ast/Relation.h"
#include "ast/StringConstant.h"
#include "ast/SubsetType.h"
#include "ast/Term.h"
#include "ast/TranslationUnit.h"
#include "ast/Type.h"
#include "ast/TypeCast.h"
#include "ast/UnionType.h"
#include "ast/UnnamedVariable.h"
#include "ast/UserDefinedFunctor.h"
#include "ast/Variable.h"
#include "ast/analysis/Aggregate.h"
#include "ast/analysis/Functor.h"
#include "ast/analysis/Ground.h"
#include "ast/analysis/IOType.h"
#include "ast/analysis/PrecedenceGraph.h"
#include "ast/analysis/RecursiveClauses.h"
#include "ast/analysis/SCCGraph.h"
#include "ast/analysis/Type.h"
#include "ast/analysis/TypeEnvironment.h"
#include "ast/analysis/TypeSystem.h"
#include "ast/transform/GroundedTermsChecker.h"
#include "ast/transform/TypeChecker.h"
#include "ast/utility/NodeMapper.h"
#include "ast/utility/Utils.h"
#include "ast/utility/Visitor.h"
#include "parser/SrcLocation.h"
#include "reports/ErrorReport.h"
#include "souffle/BinaryConstraintOps.h"
#include "souffle/TypeAttribute.h"
#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/FunctionalUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/StringUtil.h"
#include "souffle/utility/tinyformat.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <typeinfo>
#include <utility>
#include <vector>

namespace souffle::ast::transform {

using namespace analysis;

struct SemanticCheckerImpl {
    TranslationUnit& tu;
    SemanticCheckerImpl(TranslationUnit& tu);

private:
    const IOTypeAnalysis& ioTypes = *tu.getAnalysis<IOTypeAnalysis>();
    const PrecedenceGraphAnalysis& precedenceGraph = *tu.getAnalysis<PrecedenceGraphAnalysis>();
    const RecursiveClausesAnalysis& recursiveClauses = *tu.getAnalysis<RecursiveClausesAnalysis>();
    const SCCGraphAnalysis& sccGraph = *tu.getAnalysis<SCCGraphAnalysis>();

    const TypeEnvironment& typeEnv = tu.getAnalysis<TypeEnvironmentAnalysis>()->getTypeEnvironment();
    const Program& program = tu.getProgram();
    ErrorReport& report = tu.getErrorReport();

    void checkAtom(const Atom& atom);
    void checkLiteral(const Literal& literal);
    void checkAggregator(const Aggregator& aggregator);
    bool isDependent(const Clause& agg1, const Clause& agg2);
    void checkArgument(const Argument& arg);
    void checkConstant(const Argument& argument);
    void checkFact(const Clause& fact);
    void checkClause(const Clause& clause);
    void checkComplexRule(std::set<const Clause*> multiRule);
    void checkRelationDeclaration(const Relation& relation);
    void checkRelationFunctionalDependencies(const Relation& relation);
    void checkRelation(const Relation& relation);
    void checkType(ast::Attribute const& attr, std::string const& name = {});
    void checkFunctorDeclaration(const FunctorDeclaration& decl);

    void checkNamespaces();
    void checkIO();
    void checkWitnessProblem();
    void checkInlining();
};

bool SemanticChecker::transform(TranslationUnit& translationUnit) {
    SemanticCheckerImpl{translationUnit};
    return false;
}

SemanticCheckerImpl::SemanticCheckerImpl(TranslationUnit& tu) : tu(tu) {
    // suppress warnings for given relations
    if (Global::config().has("suppress-warnings")) {
        std::vector<std::string> suppressedRelations =
                splitString(Global::config().get("suppress-warnings"), ',');

        if (std::find(suppressedRelations.begin(), suppressedRelations.end(), "*") !=
                suppressedRelations.end()) {
            // mute all relations
            for (Relation* rel : program.getRelations()) {
                rel->addQualifier(RelationQualifier::SUPPRESSED);
            }
        } else {
            // mute only the given relations (if they exist)
            for (auto& relname : suppressedRelations) {
                const std::vector<std::string> comps = splitString(relname, '.');
                if (!comps.empty()) {
                    // generate the relation identifier
                    QualifiedName relid(comps[0]);
                    for (std::size_t i = 1; i < comps.size(); i++) {
                        relid.append(comps[i]);
                    }

                    // update suppressed qualifier if the relation is found
                    if (Relation* rel = getRelation(program, relid)) {
                        rel->addQualifier(RelationQualifier::SUPPRESSED);
                    }
                }
            }
        }
    }

    // check rules
    for (auto* rel : program.getRelations()) {
        checkRelation(*rel);
    }
    for (auto* clause : program.getClauses()) {
        checkClause(*clause);
    }

    for (auto* decl : program.getFunctorDeclarations()) {
        checkFunctorDeclaration(*decl);
    }

    // Group clauses that stem from a single complex rule
    // with multiple headers/disjunction etc. The grouping
    // is performed via their source-location.
    std::map<SrcLocation, std::set<const Clause*>> multiRuleMap;
    for (auto* clause : program.getClauses()) {
        // collect clauses of a multi rule, i.e., they have the same source locator
        multiRuleMap[clause->getSrcLoc()].insert(clause);
    }

    // check complex rule
    for (const auto& multiRule : multiRuleMap) {
        checkComplexRule(multiRule.second);
    }

    checkNamespaces();
    checkIO();
    checkWitnessProblem();
    checkInlining();

    // Run grounded terms checker
    GroundedTermsChecker().verify(tu);

    // Check types
    TypeChecker().verify(tu);

    // - stratification --
    // check for cyclic dependencies
    for (Relation* cur : program.getRelations()) {
        std::size_t scc = sccGraph.getSCC(cur);
        if (sccGraph.isRecursive(scc)) {
            for (const Relation* cyclicRelation : sccGraph.getInternalRelations(scc)) {
                // Negations and aggregations need to be stratified
                const Literal* foundLiteral = nullptr;
                bool hasNegation = hasClauseWithNegatedRelation(cyclicRelation, cur, &program, foundLiteral);
                if (hasNegation ||
                        hasClauseWithAggregatedRelation(cyclicRelation, cur, &program, foundLiteral)) {
                    auto const& relSet = sccGraph.getInternalRelations(scc);
                    std::set<const Relation*, NameComparison> sortedRelSet(relSet.begin(), relSet.end());
                    // Negations and aggregations need to be stratified
                    std::string relationsListStr = toString(join(sortedRelSet, ",",
                            [](std::ostream& out, const Relation* r) { out << r->getQualifiedName(); }));
                    std::vector<DiagnosticMessage> messages;
                    messages.push_back(DiagnosticMessage(
                            "Relation " + toString(cur->getQualifiedName()), cur->getSrcLoc()));
                    std::string negOrAgg = hasNegation ? "negation" : "aggregation";
                    messages.push_back(
                            DiagnosticMessage("has cyclic " + negOrAgg, foundLiteral->getSrcLoc()));
                    report.addDiagnostic(Diagnostic(Diagnostic::Type::ERROR,
                            DiagnosticMessage("Unable to stratify relation(s) {" + relationsListStr + "}"),
                            messages));
                    break;
                }
            }
        }
    }
}

void SemanticCheckerImpl::checkAtom(const Atom& atom) {
    // check existence of relation
    auto* r = getRelation(program, atom.getQualifiedName());
    if (r == nullptr) {
        report.addError("Undefined relation " + toString(atom.getQualifiedName()), atom.getSrcLoc());
        return;
    }

    if (r->getArity() != atom.getArity()) {
        report.addError("Mismatching arity of relation " + toString(atom.getQualifiedName()) + " (expected " +
                                toString(r->getArity()) + ", got " + toString(atom.getArity()) + ")",
                atom.getSrcLoc());
    }

    for (const Argument* arg : atom.getArguments()) {
        checkArgument(*arg);
    }
}

namespace {

/**
 * Get unnamed variables except those that appear inside aggregates.
 */
std::set<const UnnamedVariable*> getUnnamedVariables(const Node& node) {
    std::set<const UnnamedVariable*> unnamedInAggregates;
    visit(node, [&](const Aggregator& agg) {
        visit(agg, [&](const UnnamedVariable& var) { unnamedInAggregates.insert(&var); });
    });

    std::set<const UnnamedVariable*> unnamed;
    visit(node, [&](const UnnamedVariable& var) {
        if (!contains(unnamedInAggregates, &var)) {
            unnamed.insert(&var);
        }
    });

    return unnamed;
}

}  // namespace

void SemanticCheckerImpl::checkLiteral(const Literal& literal) {
    // check potential nested atom
    if (const auto* atom = as<Atom>(literal)) {
        checkAtom(*atom);
    }

    if (const auto* neg = as<Negation>(literal)) {
        checkAtom(*neg->getAtom());
    }

    if (const auto* constraint = as<BinaryConstraint>(literal)) {
        checkArgument(*constraint->getLHS());
        checkArgument(*constraint->getRHS());

        std::set<const UnnamedVariable*> unnamedInRecord;
        visit(*constraint, [&](const RecordInit& record) {
            for (auto* arg : record.getArguments()) {
                if (auto* unnamed = as<UnnamedVariable>(arg)) {
                    unnamedInRecord.insert(unnamed);
                }
            }
        });

        // Don't worry about underscores if either side is an aggregate (because of witness exporting)
        if (isA<Aggregator>(*constraint->getLHS()) || isA<Aggregator>(*constraint->getRHS())) {
            return;
        }
        // Check if constraint contains unnamed variables.
        for (auto* unnamed : getUnnamedVariables(*constraint)) {
            if (!contains(unnamedInRecord, unnamed)) {
                report.addError("Underscore in binary relation", unnamed->getSrcLoc());
            }
        }
    }
}

/**
 * agg1, agg2 are clauses which contain no head, and consist of a single literal
 * that contains an aggregate.
 * agg1 is dependent on agg2 if agg1 contains a variable which is grounded by agg2, and not by agg1.
 */
bool SemanticCheckerImpl::isDependent(const Clause& agg1, const Clause& agg2) {
    auto groundedInAgg1 = getGroundedTerms(tu, agg1);
    auto groundedInAgg2 = getGroundedTerms(tu, agg2);
    bool dependent = false;
    // For each variable X in the first aggregate
    visit(agg1, [&](const ast::Variable& searchVar) {
        // Try to find the corresponding variable X in the second aggregate
        // by string comparison
        const ast::Variable* matchingVarPtr = nullptr;
        visit(agg2, [&](const ast::Variable& var) {
            if (var == searchVar) {
                matchingVarPtr = &var;
                return;
            }
        });
        // If the variable occurs in both clauses (a match was found)
        if (matchingVarPtr != nullptr) {
            if (!groundedInAgg1[&searchVar] && groundedInAgg2[matchingVarPtr]) {
                dependent = true;
            }
        }
    });
    return dependent;
}

void SemanticCheckerImpl::checkAggregator(const Aggregator& aggregator) {
    auto& report = tu.getErrorReport();
    const Program& program = tu.getProgram();
    Clause dummyClauseAggregator("dummy");

    visit(program, [&](const Literal& parentLiteral) {
        visit(parentLiteral, [&](const Aggregator& candidateAggregate) {
            if (candidateAggregate != aggregator) {
                return;
            }
            // Get the literal containing the aggregator and put it into a dummy clause
            // so we can get information about groundedness
            dummyClauseAggregator.addToBody(clone(parentLiteral));
        });
    });

    visit(program, [&](const Literal& parentLiteral) {
        visit(parentLiteral, [&](const Aggregator& /* otherAggregate */) {
            // Create the other aggregate's dummy clause
            Clause dummyClauseOther("dummy");
            dummyClauseOther.addToBody(clone(parentLiteral));
            // Check dependency between the aggregator and this one
            if (isDependent(dummyClauseAggregator, dummyClauseOther) &&
                    isDependent(dummyClauseOther, dummyClauseAggregator)) {
                report.addError("Mutually dependent aggregate", aggregator.getSrcLoc());
            }
        });
    });

    for (Literal* literal : aggregator.getBodyLiterals()) {
        checkLiteral(*literal);
    }
}

void SemanticCheckerImpl::checkArgument(const Argument& arg) {
    if (const auto* agg = as<Aggregator>(arg)) {
        checkAggregator(*agg);
    } else if (const auto* func = as<Functor>(arg)) {
        for (auto arg : func->getArguments()) {
            checkArgument(*arg);
        }

        if (auto const* udFunc = as<UserDefinedFunctor const>(func)) {
            auto const& name = udFunc->getName();
            auto const* udfd = getFunctorDeclaration(program, name);

            if (udfd == nullptr) {
                report.addError("Undefined user-defined functor " + name, udFunc->getSrcLoc());
            }
        }
    }
}

namespace {

/**
 * Check if the argument can be statically evaluated
 * and thus in particular, if it should be allowed to appear as argument in facts.
 **/
bool isConstantArgument(const Argument* arg) {
    assert(arg != nullptr);

    if (isA<ast::Variable>(arg) || isA<UnnamedVariable>(arg)) {
        return false;
    } else if (isA<UserDefinedFunctor>(arg)) {
        return false;
    } else if (isA<Counter>(arg)) {
        return false;
    } else if (auto* typeCast = as<ast::TypeCast>(arg)) {
        return isConstantArgument(typeCast->getValue());
    } else if (auto* term = as<Term>(arg)) {
        // Term covers intrinsic functor, records and adts. User-functors are handled earlier.
        return all_of(term->getArguments(), isConstantArgument);
    } else if (isA<Constant>(arg)) {
        return true;
    } else {
        fatal("unsupported argument type: %s", typeid(arg).name());
    }
}

}  // namespace

/* Check if facts contain only constants */
void SemanticCheckerImpl::checkFact(const Clause& fact) {
    assert(isFact(fact));

    Atom* head = fact.getHead();
    if (head == nullptr) {
        return;  // checked by clause
    }

    Relation* rel = getRelation(program, head->getQualifiedName());
    if (rel == nullptr) {
        return;  // checked by clause
    }

    // facts must only contain constants
    for (auto* arg : head->getArguments()) {
        if (!isConstantArgument(arg)) {
            report.addError("Argument in fact is not constant", arg->getSrcLoc());
        }
    }
}

void SemanticCheckerImpl::checkClause(const Clause& clause) {
    // check head atom
    checkAtom(*clause.getHead());

    // Check for absence of underscores in head
    for (auto* unnamed : getUnnamedVariables(*clause.getHead())) {
        report.addError("Underscore in head of rule", unnamed->getSrcLoc());
    }

    // check body literals
    for (Literal* lit : clause.getBodyLiterals()) {
        checkLiteral(*lit);
    }

    // check facts
    if (isFact(clause)) {
        checkFact(clause);
    }

    // check whether named unnamed variables of the form _<ident>
    // are only used once in a clause; if not, warnings will be
    // issued.
    std::map<std::string, int> var_count;
    std::map<std::string, const ast::Variable*> var_pos;
    visit(clause, [&](const ast::Variable& var) {
        var_count[var.getName()]++;
        var_pos[var.getName()] = &var;
    });
    for (const auto& cur : var_count) {
        int numAppearances = cur.second;
        const auto& varName = cur.first;
        const auto& varLocation = var_pos[varName]->getSrcLoc();
        if (varName[0] == '_') {
            assert(varName.size() > 1 && "named variable should not be a single underscore");
            if (numAppearances > 1) {
                report.addWarning("Variable " + varName + " marked as singleton but occurs more than once",
                        varLocation);
            }
        }
    }

    // check auto-increment
    if (recursiveClauses.recursive(&clause)) {
        visit(clause, [&](const Counter& ctr) {
            report.addError("Auto-increment functor in a recursive rule", ctr.getSrcLoc());
        });
    }
}

void SemanticCheckerImpl::checkComplexRule(std::set<const Clause*> multiRule) {
    std::map<std::string, int> var_count;
    std::map<std::string, const ast::Variable*> var_pos;

    // Count the variable occurrence for the body of a
    // complex rule only once.
    // TODO (b-scholz): for negation / disjunction this is not quite
    // right; we would need more semantic information here.
    for (auto literal : (*multiRule.begin())->getBodyLiterals()) {
        visit(*literal, [&](const ast::Variable& var) {
            var_count[var.getName()]++;
            var_pos[var.getName()] = &var;
        });
    }

    // Count variable occurrence for each head separately
    for (auto clause : multiRule) {
        visit(*(clause->getHead()), [&](const ast::Variable& var) {
            var_count[var.getName()]++;
            var_pos[var.getName()] = &var;
        });
    }

    // Check that a variables occurs more than once
    for (const auto& cur : var_count) {
        int numAppearances = cur.second;
        const auto& varName = cur.first;
        const auto& varLocation = var_pos[varName]->getSrcLoc();
        if (varName[0] != '_' && numAppearances == 1) {
            report.addWarning("Variable " + varName + " only occurs once", varLocation);
        }
    }
}

void SemanticCheckerImpl::checkType(ast::Attribute const& attr, std::string const& name) {
    auto&& typeName = attr.getTypeName();
    auto* existingType = getIf(
            program.getTypes(), [&](const ast::Type* type) { return type->getQualifiedName() == typeName; });

    /* check whether type exists */
    if (!typeEnv.isPrimitiveType(typeName) && nullptr == existingType) {
        std::ostringstream out;

        if (name.empty()) {
            if (attr.getName().empty()) {
                report.addError(
                        tfm::format("Undefined type %s in attribute", attr.getTypeName()), attr.getSrcLoc());
            } else {
                report.addError(tfm::format("Undefined type in attribute %s", attr), attr.getSrcLoc());
            }
        } else {
            report.addError(
                    tfm::format("Undefined type %s in %s", attr.getTypeName(), name), attr.getSrcLoc());
        }
    }
}

void SemanticCheckerImpl::checkFunctorDeclaration(const FunctorDeclaration& decl) {
    checkType(decl.getReturnType(), "return type");

    for (auto const& param : decl.getParams()) {
        checkType(*param);
    }
}

void SemanticCheckerImpl::checkRelationDeclaration(const Relation& relation) {
    const auto& attributes = relation.getAttributes();
    assert(attributes.size() == relation.getArity() && "mismatching attribute size and arity");

    for (std::size_t i = 0; i < relation.getArity(); i++) {
        Attribute* attr = attributes[i];
        checkType(*attr);

        /* check whether name occurs more than once */
        for (std::size_t j = 0; j < i; j++) {
            if (attr->getName() == attributes[j]->getName()) {
                report.addError(tfm::format("Doubly defined attribute name %s", *attr), attr->getSrcLoc());
            }
        }
    }
}

/* check that each functional dependency (keys) actually appears in the relation */
void SemanticCheckerImpl::checkRelationFunctionalDependencies(const Relation& relation) {
    const auto attributes = relation.getAttributes();
    for (const auto& fd : relation.getFunctionalDependencies()) {
        // Check that keys appear in relation arguments
        const auto keys = fd->getKeys();
        for (const auto& key : keys) {
            auto found = std::find_if(
                    attributes.begin(), attributes.end(), [&key](const ast::Attribute* attribute) {
                        return key->getName() == attribute->getName();
                    });
            if (found == attributes.end()) {
                report.addError("Attribute " + key->getName() + " not found in relation definition.",
                        fd->getSrcLoc());
            }
        }
    }
}

void SemanticCheckerImpl::checkRelation(const Relation& relation) {
    if (relation.getRepresentation() == RelationRepresentation::EQREL) {
        if (relation.getArity() == 2) {
            const auto& attributes = relation.getAttributes();
            assert(attributes.size() == 2 && "mismatching attribute size and arity");
            if (attributes[0]->getTypeName() != attributes[1]->getTypeName()) {
                report.addError("Domains of equivalence relation " + toString(relation.getQualifiedName()) +
                                        " are different",
                        relation.getSrcLoc());
            }
        } else {
            report.addError(
                    "Equivalence relation " + toString(relation.getQualifiedName()) + " is not binary",
                    relation.getSrcLoc());
        }
    }

    // start with declaration
    checkRelationDeclaration(relation);

    // check dependencies of relation are valid (i.e. attribute names occur in relation)
    checkRelationFunctionalDependencies(relation);

    // check whether this relation is empty
    if (getClauses(program, relation).empty() && !ioTypes.isInput(&relation) &&
            !relation.hasQualifier(RelationQualifier::SUPPRESSED)) {
        report.addWarning("No rules/facts defined for relation " + toString(relation.getQualifiedName()),
                relation.getSrcLoc());
    }
}

void SemanticCheckerImpl::checkIO() {
    auto checkIO = [&](const Directive* directive) {
        auto* r = getRelation(program, directive->getQualifiedName());
        if (r == nullptr) {
            report.addError(
                    "Undefined relation " + toString(directive->getQualifiedName()), directive->getSrcLoc());
        }
    };
    for (const auto* directive : program.getDirectives()) {
        checkIO(directive);
    }
}

/**
 *  A witness is considered "invalid" if it is trying to export a witness
 *  out of a count, sum, or mean aggregate.
 *
 *  However we need to be careful: Sometimes a witness variables occurs within the body
 *  of a count, sum, or mean aggregate, but this is valid, because the witness
 *  actually belongs to an inner min or max aggregate.
 *
 *  We just need to check that that witness only occurs on this level.
 *
 **/
static const std::vector<SrcLocation> usesInvalidWitness(
        TranslationUnit& tu, const Clause& clause, const Aggregator& aggregate) {
    std::vector<SrcLocation> invalidWitnessLocations;

    if (aggregate.getBaseOperator() == AggregateOp::MIN || aggregate.getBaseOperator() == AggregateOp::MAX) {
        return invalidWitnessLocations;  // ie empty result
    }

    auto aggregateSubclause = mk<Clause>("*");
    aggregateSubclause->setBodyLiterals(clone(aggregate.getBodyLiterals()));

    struct InnerAggregateMasker : public NodeMapper {
        mutable int numReplaced = 0;
        Own<Node> operator()(Own<Node> node) const override {
            if (isA<Aggregator>(node)) {
                std::string newVariableName = "+aggr_var_" + toString(numReplaced++);
                return mk<Variable>(newVariableName);
            }
            node->apply(*this);
            return node;
        }
    };
    InnerAggregateMasker update;
    aggregateSubclause->apply(update);

    // Find the witnesses of the original aggregate.
    // If we can find occurrences of the witness in
    // this masked version of the aggregate subclause,
    // AND the aggregate is a sum / count / mean (we know this because
    // of the early exit for a min/max aggregate)
    // then we have an invalid witness and we'll add the source location
    // of the variable to the invalidWitnessLocations vector.
    auto witnesses = analysis::getWitnessVariables(tu, clause, aggregate);
    for (const auto& witness : witnesses) {
        visit(*aggregateSubclause, [&](const Variable& var) {
            if (var.getName() == witness) {
                invalidWitnessLocations.push_back(var.getSrcLoc());
            }
        });
    }
    return invalidWitnessLocations;
}

void SemanticCheckerImpl::checkWitnessProblem() {
    // Check whether there is the use of a witness in
    // an aggregate where it doesn't make sense to use it, i.e.
    // count, sum, mean
    visit(program, [&](const Clause& clause) {
        visit(clause, [&](const Aggregator& agg) {
            for (auto&& invalidArgument : usesInvalidWitness(tu, clause, agg)) {
                report.addError(
                        "Witness problem: argument grounded by an aggregator's inner scope is used "
                        "ungrounded in "
                        "outer scope in a count/sum/mean aggregate",
                        invalidArgument);
            }
        });
    });
}

/**
 * Find a cycle consisting entirely of inlined relations.
 * If no cycle exists, then an empty vector is returned.
 */
std::vector<QualifiedName> findInlineCycle(const PrecedenceGraphAnalysis& precedenceGraph,
        std::map<const Relation*, const Relation*>& origins, const Relation* current, RelationSet& unvisited,
        RelationSet& visiting, RelationSet& visited) {
    std::vector<QualifiedName> result;

    if (current == nullptr) {
        // Not looking at any nodes at the moment, so choose any node from the unvisited list

        if (unvisited.empty()) {
            // Nothing left to visit - so no cycles exist!
            return result;
        }

        // Choose any element from the unvisited set
        current = *unvisited.begin();
        origins[current] = nullptr;

        // Move it to "currently visiting"
        unvisited.erase(current);
        visiting.insert(current);

        // Check if we can find a cycle beginning from this node
        std::vector<QualifiedName> subresult =
                findInlineCycle(precedenceGraph, origins, current, unvisited, visiting, visited);

        if (subresult.empty()) {
            // No cycle found, try again from another node
            return findInlineCycle(precedenceGraph, origins, nullptr, unvisited, visiting, visited);
        } else {
            // Cycle found! Return it
            return subresult;
        }
    }

    // Check neighbours
    const RelationSet& successors = precedenceGraph.graph().successors(current);
    for (const Relation* successor : successors) {
        // Only care about inlined neighbours in the graph
        if (successor->hasQualifier(RelationQualifier::INLINE)) {
            if (visited.find(successor) != visited.end()) {
                // The neighbour has already been visited, so move on
                continue;
            }

            if (visiting.find(successor) != visiting.end()) {
                // Found a cycle!!
                // Construct the cycle in reverse
                while (current != nullptr) {
                    result.push_back(current->getQualifiedName());
                    current = origins[current];
                }
                return result;
            }

            // Node has not been visited yet
            origins[successor] = current;

            // Move from unvisited to visiting
            unvisited.erase(successor);
            visiting.insert(successor);

            // Visit recursively and check if a cycle is formed
            std::vector<QualifiedName> subgraphCycle =
                    findInlineCycle(precedenceGraph, origins, successor, unvisited, visiting, visited);

            if (!subgraphCycle.empty()) {
                // Found a cycle!
                return subgraphCycle;
            }
        }
    }

    // Visited all neighbours with no cycle found, so done visiting this node.
    visiting.erase(current);
    visited.insert(current);
    return result;
}

void SemanticCheckerImpl::checkInlining() {
    auto isInline = [&](const Relation* rel) { return rel->hasQualifier(RelationQualifier::INLINE); };

    // Find all inlined relations
    RelationSet inlinedRelations;
    for (const auto& relation : program.getRelations()) {
        if (isInline(relation)) {
            inlinedRelations.insert(relation);
            if (ioTypes.isIO(relation)) {
                report.addError(
                        "IO relation " + toString(relation->getQualifiedName()) + " cannot be inlined",
                        relation->getSrcLoc());
            }
        }
    }

    // Check 1:
    // Let G' be the subgraph of the precedence graph G containing only those nodes
    // which are marked with the inline directive.
    // If G' contains a cycle, then inlining cannot be performed.

    RelationSet unvisited;  // nodes that have not been visited yet
    RelationSet visiting;   // nodes that we are currently visiting
    RelationSet visited;    // nodes that have been completely explored

    // All nodes are initially unvisited
    for (const Relation* rel : inlinedRelations) {
        unvisited.insert(rel);
    }

    // Remember the parent node of each visited node to construct the found cycle
    std::map<const Relation*, const Relation*> origins;

    std::vector<QualifiedName> result =
            findInlineCycle(precedenceGraph, origins, nullptr, unvisited, visiting, visited);

    // If the result contains anything, then a cycle was found
    if (!result.empty()) {
        Relation* cycleOrigin = getRelation(program, result[result.size() - 1]);

        // Construct the string representation of the cycle
        std::stringstream cycle;
        cycle << "{" << cycleOrigin->getQualifiedName();

        // Print it backwards to preserve the initial cycle order
        for (int i = result.size() - 2; i >= 0; i--) {
            cycle << ", " << result[i];
        }

        cycle << "}";

        report.addError(
                "Cannot inline cyclically dependent relations " + cycle.str(), cycleOrigin->getSrcLoc());
    }

    // Check 2:
    // Cannot use the counter argument ('$') in inlined relations

    // Check if an inlined literal ever takes in a $
    visit(program, [&](const Atom& atom) {
        Relation* associatedRelation = getRelation(program, atom.getQualifiedName());
        if (associatedRelation != nullptr && isInline(associatedRelation)) {
            visit(atom, [&](const Argument& arg) {
                if (isA<Counter>(&arg)) {
                    report.addError(
                            "Cannot inline literal containing a counter argument '$'", arg.getSrcLoc());
                }
            });
        }
    });

    // Check if an inlined clause ever contains a $
    for (const Relation* rel : inlinedRelations) {
        for (Clause* clause : getClauses(program, *rel)) {
            visit(*clause, [&](const Argument& arg) {
                if (isA<Counter>(&arg)) {
                    report.addError(
                            "Cannot inline clause containing a counter argument '$'", arg.getSrcLoc());
                }
            });
        }
    }

    // Check 3:
    // Suppose the relation b is marked with the inline directive, but appears negated
    // in a clause. Then, if b introduces a new variable in its body, we cannot inline
    // the relation b.

    // Find all relations with the inline declarative that introduce new variables in their bodies
    RelationSet nonNegatableRelations;
    for (const Relation* rel : inlinedRelations) {
        bool foundNonNegatable = false;
        for (const Clause* clause : getClauses(program, *rel)) {
            // Get the variables in the head
            std::set<std::string> headVariables;
            visit(*clause->getHead(), [&](const ast::Variable& var) { headVariables.insert(var.getName()); });

            // Get the variables in the body
            std::set<std::string> bodyVariables;
            visit(clause->getBodyLiterals(),
                    [&](const ast::Variable& var) { bodyVariables.insert(var.getName()); });

            // Check if all body variables are in the head
            // Do this separately to the above so only one error is printed per variable
            for (const std::string& var : bodyVariables) {
                if (headVariables.find(var) == headVariables.end()) {
                    nonNegatableRelations.insert(rel);
                    foundNonNegatable = true;
                    break;
                }
            }

            if (foundNonNegatable) {
                break;
            }
        }
    }

    // Check that these relations never appear negated
    visit(program, [&](const Negation& neg) {
        Relation* associatedRelation = getRelation(program, neg.getAtom()->getQualifiedName());
        if (associatedRelation != nullptr &&
                nonNegatableRelations.find(associatedRelation) != nonNegatableRelations.end()) {
            report.addError(
                    "Cannot inline negated relation which may introduce new variables", neg.getSrcLoc());
        }
    });

    // Check 4:
    // Don't support inlining atoms within aggregators at this point.

    // Reasoning: Suppose we have an aggregator like `max X: a(X)`, where `a` is inlined to `a1` and `a2`.
    // Then, `max X: a(X)` will become `max( max X: a1(X),  max X: a2(X) )`. Suppose further that a(X) has
    // values X where it is true, while a2(X) does not. Then, the produced argument
    // `max( max X: a1(X),  max X: a2(X) )` will not return anything (as one of its arguments fails), while
    // `max X: a(X)` will.
    // Can work around this with emptiness checks (e.g. `!a1(_), ... ; !a2(_), ... ; ...`)

    // This corner case prevents generalising aggregator inlining with the current set up.

    visit(program, [&](const Aggregator& aggr) {
        visit(aggr, [&](const Atom& subatom) {
            const Relation* rel = getRelation(program, subatom.getQualifiedName());
            if (rel != nullptr && isInline(rel)) {
                report.addError("Cannot inline relations that appear in aggregator", subatom.getSrcLoc());
            }
        });
    });

    // Check 5:
    // Suppose a relation `a` is inlined, appears negated in a clause, and contains a
    // (possibly nested) unnamed variable in its arguments. Then, the atom can't be
    // inlined, as unnamed variables are named during inlining (since they may appear
    // multiple times in an inlined-clause's body) => ungroundedness!

    // Exception: It's fine if the unnamed variable appears in a nested aggregator, as
    // the entire aggregator will automatically be grounded.

    // TODO (azreika): special case where all rules defined for `a` use the
    // underscored-argument exactly once: can workaround by remapping the variable
    // back to an underscore - involves changes to the actual inlining algo, though

    // Returns the pair (isValid, lastSrcLoc) where:
    //  - isValid is true if and only if the node contains an invalid underscore, and
    //  - lastSrcLoc is the source location of the last visited node
    std::function<std::pair<bool, SrcLocation>(const Node&)> checkInvalidUnderscore = [&](const Node& node) {
        if (isA<UnnamedVariable>(node)) {
            // Found an invalid underscore
            return std::make_pair(true, node.getSrcLoc());
        } else if (isA<Aggregator>(node)) {
            // Don't care about underscores within aggregators
            return std::make_pair(false, node.getSrcLoc());
        }

        // Check if any children nodes use invalid underscores
        for (const Node& child : node.getChildNodes()) {
            std::pair<bool, SrcLocation> childStatus = checkInvalidUnderscore(child);
            if (childStatus.first) {
                // Found an invalid underscore
                return childStatus;
            }
        }

        return std::make_pair(false, node.getSrcLoc());
    };

    // Perform the check
    visit(program, [&](const Negation& negation) {
        const Atom* associatedAtom = negation.getAtom();
        const Relation* associatedRelation = getRelation(program, associatedAtom->getQualifiedName());
        if (associatedRelation != nullptr && isInline(associatedRelation)) {
            std::pair<bool, SrcLocation> atomStatus = checkInvalidUnderscore(*associatedAtom);
            if (atomStatus.first) {
                report.addError(
                        "Cannot inline negated atom containing an unnamed variable unless the variable is "
                        "within an aggregator",
                        atomStatus.second);
            }
        }
    });
}

// Check that type and relation names are disjoint sets.
void SemanticCheckerImpl::checkNamespaces() {
    std::map<std::string, SrcLocation> names;

    // Find all names and report redeclarations as we go.
    for (const auto& type : program.getTypes()) {
        const std::string name = toString(type->getQualifiedName());
        if (names.count(name) != 0u) {
            report.addError("Name clash on type " + name, type->getSrcLoc());
        } else {
            names[name] = type->getSrcLoc();
        }
    }

    for (const auto& rel : program.getRelations()) {
        const std::string name = toString(rel->getQualifiedName());
        if (names.count(name) != 0u) {
            report.addError("Name clash on relation " + name, rel->getSrcLoc());
        } else {
            names[name] = rel->getSrcLoc();
        }
    }
}

}  // namespace souffle::ast::transform
