#include <gtest/gtest.h>

#include "01_tokenize/factory/OperatorTokenFactory.h"

TEST(OperatorTokenFactory, canHandle) {
    auto factory = nugdev::compiler::tokenize::OperatorTokenFactory();
    EXPECT_TRUE(factory.canHandle(L'+'));
}

TEST(OperatorTokenFactory, canNotHandle) {
    auto factory = nugdev::compiler::tokenize::OperatorTokenFactory();
    EXPECT_FALSE(factory.canHandle(L'a'));
}
