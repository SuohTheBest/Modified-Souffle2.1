/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file symbol_table_test.cpp
 *
 * Tests souffle's symbol table.
 *
 ***********************************************************************/

#include "tests/test.h"

#include "souffle/SymbolTable.h"
#include "souffle/utility/MiscUtil.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace souffle::test {

TEST(SymbolTable, Basics) {
    SymbolTable table;

    EXPECT_STREQ("Hello", table.decode(table.encode(table.decode(table.encode("Hello")))));

    EXPECT_EQ(table.encode("Hello"), table.encode(table.decode(table.encode("Hello"))));

    EXPECT_STREQ("Hello", table.decode(table.encode(table.decode(table.encode("Hello")))));

    EXPECT_EQ(table.encode("Hello"),
            table.encode(table.decode(table.encode(table.decode(table.encode("Hello"))))));
}

TEST(SymbolTable, Inserts) {
    SymbolTable X({"A", "B", "C", "D"});
    std::vector<std::string> V;
    for (const auto& It : X) {
        V.push_back(It.first);
    }
    EXPECT_EQ(V.size(), 4);
}

}  // namespace souffle::test
