#include "kern/support/ModuleResolver.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace kern {
namespace {

namespace fs = std::filesystem;

class ModuleResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "kern_mod_test";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    void writeFile(const std::string& rel_path, const std::string& content) {
        auto path = tmp_dir_ / rel_path;
        fs::create_directories(path.parent_path());
        std::ofstream ofs(path);
        ofs << content;
    }

    DiagnosticEngine diag_;
    fs::path tmp_dir_;
};

TEST_F(ModuleResolverTest, ResolveSimpleModule) {
    writeFile("math.kern", "module math\npub fn add(a: i64, b: i64) -> i64 { a + b }\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    std::string file;
    ASSERT_TRUE(resolver.resolve("math", file));
    EXPECT_TRUE(file.find("math.kern") != std::string::npos);
}

TEST_F(ModuleResolverTest, ResolveNestedModule) {
    writeFile("kern/memory/page.kern", "module kern.memory.page\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    std::string file;
    ASSERT_TRUE(resolver.resolve("kern.memory.page", file));
    EXPECT_TRUE(file.find("kern/memory/page.kern") != std::string::npos);
}

TEST_F(ModuleResolverTest, ResolveNotFound) {
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    std::string file;
    EXPECT_FALSE(resolver.resolve("nonexistent", file));
}

TEST_F(ModuleResolverTest, BuildSimpleDag) {
    writeFile("main.kern", "import math (add)\nfn main() -> i64 { add(1, 2) }\n");
    writeFile("math.kern", "module math\npub fn add(a: i64, b: i64) -> i64 { a + b }\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    ASSERT_TRUE(resolver.buildDependencyGraph({(tmp_dir_ / "main.kern").string()}));
    EXPECT_EQ(resolver.modules().size(), 2u);
}

TEST_F(ModuleResolverTest, TopologicalOrder) {
    writeFile("a.kern", "import b (foo)\nfn main() -> i64 { foo(1) }\n");
    writeFile("b.kern", "module b\nimport c (bar)\npub fn foo(x: i64) -> i64 { bar(x) }\n");
    writeFile("c.kern", "module c\npub fn bar(x: i64) -> i64 { x }\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    ASSERT_TRUE(resolver.buildDependencyGraph({(tmp_dir_ / "a.kern").string()}));
    std::vector<std::string> order;
    ASSERT_TRUE(resolver.topologicalOrder(order));
    ASSERT_EQ(order.size(), 3u);
    // Leaves first: c, then b, then a
    EXPECT_EQ(order[0], "c");
    EXPECT_EQ(order[1], "b");
}

TEST_F(ModuleResolverTest, DetectCycle) {
    writeFile("a.kern", "module a\nimport b (bar)\npub fn foo(x: i64) -> i64 { bar(x) }\n");
    writeFile("b.kern", "module b\nimport a (foo)\npub fn bar(x: i64) -> i64 { foo(x) }\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    ASSERT_TRUE(resolver.buildDependencyGraph({(tmp_dir_ / "a.kern").string()}));
    std::vector<std::string> order;
    EXPECT_FALSE(resolver.topologicalOrder(order));
}

TEST_F(ModuleResolverTest, DiamondDependency) {
    writeFile("a.kern", "import b (fb)\nimport c (fc)\nfn main() -> i64 { fb(fc(1)) }\n");
    writeFile("b.kern", "module b\nimport d (fd)\npub fn fb(x: i64) -> i64 { fd(x) }\n");
    writeFile("c.kern", "module c\nimport d (fd)\npub fn fc(x: i64) -> i64 { fd(x) }\n");
    writeFile("d.kern", "module d\npub fn fd(x: i64) -> i64 { x }\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    ASSERT_TRUE(resolver.buildDependencyGraph({(tmp_dir_ / "a.kern").string()}));
    std::vector<std::string> order;
    ASSERT_TRUE(resolver.topologicalOrder(order));
    ASSERT_EQ(order.size(), 4u);
    // d must come before b and c
    auto d_pos = std::find(order.begin(), order.end(), "d") - order.begin();
    auto b_pos = std::find(order.begin(), order.end(), "b") - order.begin();
    auto c_pos = std::find(order.begin(), order.end(), "c") - order.begin();
    EXPECT_LT(d_pos, b_pos);
    EXPECT_LT(d_pos, c_pos);
}

TEST_F(ModuleResolverTest, ModulePathToFilePath) {
    // Just test the static helper implicitly via resolve
    writeFile("kern/types.kern", "module kern.types\n");
    ModuleResolver resolver(diag_);
    resolver.addSearchPath(tmp_dir_.string());
    std::string file;
    ASSERT_TRUE(resolver.resolve("kern.types", file));
    EXPECT_TRUE(file.find("kern/types.kern") != std::string::npos);
}

} // namespace
} // namespace kern
