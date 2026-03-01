#include "kern/pkg/Manifest.h"
#include <gtest/gtest.h>

namespace kern {

TEST(ManifestTest, ParseBasic) {
    std::string toml = R"(
[package]
name = "hello"
version = "0.1.0"
entry = "src/main.kern"
)";
    auto m = Manifest::parse(toml);
    EXPECT_EQ(m.name, "hello");
    EXPECT_EQ(m.version, "0.1.0");
    EXPECT_EQ(m.entry, "src/main.kern");
}

TEST(ManifestTest, ParseSources) {
    std::string toml = R"(
[package]
name = "multi"
version = "1.0.0"
entry = "src/main.kern"
sources = ["src/main.kern", "src/lib.kern"]
)";
    auto m = Manifest::parse(toml);
    ASSERT_EQ(m.sources.size(), 2u);
    EXPECT_EQ(m.sources[0], "src/main.kern");
    EXPECT_EQ(m.sources[1], "src/lib.kern");
}

TEST(ManifestTest, ParseDependencies) {
    std::string toml = R"(
[package]
name = "app"

[dependencies]
mylib = "../mylib"
)";
    auto m = Manifest::parse(toml);
    ASSERT_EQ(m.dependencies.size(), 1u);
    EXPECT_EQ(m.dependencies[0].name, "mylib");
    EXPECT_EQ(m.dependencies[0].path, "../mylib");
}

TEST(ManifestTest, ParseComments) {
    std::string toml = R"(
# This is a comment
[package]
name = "test"
# Another comment
version = "0.0.1"
)";
    auto m = Manifest::parse(toml);
    EXPECT_EQ(m.name, "test");
    EXPECT_EQ(m.version, "0.0.1");
}

TEST(ManifestTest, ParseEmpty) {
    auto m = Manifest::parse("");
    EXPECT_TRUE(m.name.empty());
    EXPECT_TRUE(m.version.empty());
    EXPECT_TRUE(m.entry.empty());
}

TEST(ManifestTest, BuildPlanSingleFile) {
    Manifest m;
    m.name = "hello";
    m.entry = "main.kern";
    auto plan = BuildPlan::create(m);
    ASSERT_EQ(plan.compile_order.size(), 1u);
    EXPECT_EQ(plan.compile_order[0], "main.kern");
    EXPECT_EQ(plan.output, "hello");
}

TEST(ManifestTest, BuildPlanMultiFile) {
    Manifest m;
    m.name = "app";
    m.entry = "main.kern";
    m.sources = {"main.kern", "lib.kern", "utils.kern"};
    auto plan = BuildPlan::create(m);
    ASSERT_EQ(plan.compile_order.size(), 3u);
    EXPECT_EQ(plan.compile_order[0], "main.kern");
}

TEST(ManifestTest, BuildPlanNoName) {
    Manifest m;
    m.entry = "src/main.kern";
    auto plan = BuildPlan::create(m);
    EXPECT_EQ(plan.output, "a.out");
}

} // namespace kern
