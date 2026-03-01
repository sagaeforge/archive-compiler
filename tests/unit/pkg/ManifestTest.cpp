#include "kern/pkg/Manifest.h"
#include <gtest/gtest.h>
#include <fstream>

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

// ============================================================================
// DependencyResolver tests
// ============================================================================

class DepResolverTest : public ::testing::Test {
protected:
    std::string tmp_dir_;

    void SetUp() override {
        tmp_dir_ = "/tmp/kern_dep_test_" + std::to_string(getpid());
        system(("rm -rf " + tmp_dir_).c_str());
        system(("mkdir -p " + tmp_dir_).c_str());
    }

    void TearDown() override {
        system(("rm -rf " + tmp_dir_).c_str());
    }

    void writeManifest(const std::string& dir, const std::string& content) {
        system(("mkdir -p " + dir).c_str());
        std::string path = dir + "/kern.toml";
        std::ofstream ofs(path);
        ofs << content;
    }
};

TEST_F(DepResolverTest, NoDependencies) {
    Manifest root;
    root.name = "app";
    root.entry = "main.kern";
    auto result = DependencyResolver::resolve(root, tmp_dir_);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.resolved.empty());
}

TEST_F(DepResolverTest, SingleDependency) {
    // Create dep manifest
    writeManifest(tmp_dir_ + "/mylib",
        "[package]\nname = \"mylib\"\nversion = \"1.0.0\"\nentry = \"lib.kern\"\n");

    Manifest root;
    root.name = "app";
    root.entry = "main.kern";
    root.dependencies.push_back({"mylib", "", "mylib"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_EQ(result.resolved.size(), 1u);
    EXPECT_EQ(result.resolved[0].name, "mylib");
}

TEST_F(DepResolverTest, TransitiveDependency) {
    // app → libA → libB
    writeManifest(tmp_dir_ + "/libB",
        "[package]\nname = \"libB\"\nentry = \"b.kern\"\n");
    writeManifest(tmp_dir_ + "/libA",
        "[package]\nname = \"libA\"\nentry = \"a.kern\"\n"
        "[dependencies]\nlibB = \"../libB\"\n");

    Manifest root;
    root.name = "app";
    root.dependencies.push_back({"libA", "", "libA"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_EQ(result.resolved.size(), 2u);
    // libB should come before libA (topological order)
    EXPECT_EQ(result.resolved[0].name, "libB");
    EXPECT_EQ(result.resolved[1].name, "libA");
}

TEST_F(DepResolverTest, CircularDependency) {
    // app → libA → libB → libA (cycle!)
    writeManifest(tmp_dir_ + "/libA",
        "[package]\nname = \"libA\"\n[dependencies]\nlibB = \"../libB\"\n");
    writeManifest(tmp_dir_ + "/libB",
        "[package]\nname = \"libB\"\n[dependencies]\nlibA = \"../libA\"\n");

    Manifest root;
    root.name = "app";
    root.dependencies.push_back({"libA", "", "libA"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("circular"), std::string::npos);
}

TEST_F(DepResolverTest, DiamondDependency) {
    // app → libA → libC, app → libB → libC (diamond, not cycle)
    writeManifest(tmp_dir_ + "/libC",
        "[package]\nname = \"libC\"\nentry = \"c.kern\"\n");
    writeManifest(tmp_dir_ + "/libA",
        "[package]\nname = \"libA\"\n[dependencies]\nlibC = \"../libC\"\n");
    writeManifest(tmp_dir_ + "/libB",
        "[package]\nname = \"libB\"\n[dependencies]\nlibC = \"../libC\"\n");

    Manifest root;
    root.name = "app";
    root.dependencies.push_back({"libA", "", "libA"});
    root.dependencies.push_back({"libB", "", "libB"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_EQ(result.resolved.size(), 3u);
    // libC should be first (deepest dep)
    EXPECT_EQ(result.resolved[0].name, "libC");
}

TEST_F(DepResolverTest, MissingDependency) {
    Manifest root;
    root.name = "app";
    root.dependencies.push_back({"nonexistent", "", "nonexistent"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error.find("nonexistent"), std::string::npos);
}

TEST_F(DepResolverTest, DeepChain) {
    // app → A → B → C → D
    writeManifest(tmp_dir_ + "/D",
        "[package]\nname = \"D\"\nentry = \"d.kern\"\n");
    writeManifest(tmp_dir_ + "/C",
        "[package]\nname = \"C\"\n[dependencies]\nD = \"../D\"\n");
    writeManifest(tmp_dir_ + "/B",
        "[package]\nname = \"B\"\n[dependencies]\nC = \"../C\"\n");
    writeManifest(tmp_dir_ + "/A",
        "[package]\nname = \"A\"\n[dependencies]\nB = \"../B\"\n");

    Manifest root;
    root.name = "app";
    root.dependencies.push_back({"A", "", "A"});

    auto result = DependencyResolver::resolve(root, tmp_dir_);
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_EQ(result.resolved.size(), 4u);
    EXPECT_EQ(result.resolved[0].name, "D");
    EXPECT_EQ(result.resolved[1].name, "C");
    EXPECT_EQ(result.resolved[2].name, "B");
    EXPECT_EQ(result.resolved[3].name, "A");
}

} // namespace kern
