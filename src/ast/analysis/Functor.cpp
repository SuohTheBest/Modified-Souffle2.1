/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020 The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Functor.cpp
 *
 * Analysis that provides type information for functors
 *
 ***********************************************************************/

#include "ast/analysis/Functor.h"
#include "ast/TranslationUnit.h"
#include "ast/analysis/Type.h"

namespace souffle::ast::analysis {

void FunctorAnalysis::run(const TranslationUnit& translationUnit) {
    typeAnalysis = translationUnit.getAnalysis<TypeAnalysis>();
}

bool FunctorAnalysis::isStateful(const UserDefinedFunctor& udf) const {
    return typeAnalysis->isStatefulFunctor(udf);
}

TypeAttribute FunctorAnalysis::getReturnTypeAttribute(const Functor& functor) const {
    return typeAnalysis->getFunctorReturnTypeAttribute(functor);
}

Type const& FunctorAnalysis::getReturnType(const UserDefinedFunctor& functor) const {
    return typeAnalysis->getFunctorReturnType(functor);
}

/** Return parameter type of functor */
TypeAttribute FunctorAnalysis::getParamTypeAttribute(const Functor& functor, const std::size_t idx) const {
    return typeAnalysis->getFunctorParamTypeAttribute(functor, idx);
}

Type const& FunctorAnalysis::getParamType(const UserDefinedFunctor& functor, const std::size_t idx) const {
    return typeAnalysis->getFunctorParamType(functor, idx);
}

std::vector<TypeAttribute> FunctorAnalysis::getParamTypeAttributes(const UserDefinedFunctor& functor) const {
    return typeAnalysis->getFunctorParamTypeAttributes(functor);
}

bool FunctorAnalysis::isMultiResult(const Functor& functor) {
    return TypeAnalysis::isMultiResultFunctor(functor);
}

}  // namespace souffle::ast::analysis
