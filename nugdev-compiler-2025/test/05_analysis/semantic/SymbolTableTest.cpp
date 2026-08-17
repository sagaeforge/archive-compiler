#include "05_analysis/semantic/SymbolTable.hpp"
#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/literals/BooleanLiteral.hpp"
#include "04_parsing/ast/literals/NumberLiteral.hpp"
#include "04_parsing/ast/literals/StringLiteral.hpp"
#include "04_parsing/ast/types/TypeLiteral.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>

using namespace nugdev::compiler::analysis;
namespace ast = nugdev::ast;

class SymbolTableTest : public ::testing::Test {
protected:
  void SetUp() override {
    symbol_table = std::make_unique<SymbolTable>();

    // 테스트용 타입 리터럴 생성 (더미 구현)
    number_type = std::make_unique<ast::TypeLiteral>("number");
    string_type = std::make_unique<ast::TypeLiteral>("string");
    boolean_type = std::make_unique<ast::TypeLiteral>("boolean");
  }

  std::unique_ptr<SymbolTable> symbol_table;
  std::unique_ptr<ast::TypeLiteral> number_type;
  std::unique_ptr<ast::TypeLiteral> string_type;
  std::unique_ptr<ast::TypeLiteral> boolean_type;
};

// =============================================================================
// Symbol Tests
// =============================================================================

TEST_F(SymbolTableTest, SymbolCreation) {
  auto symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "test_var",
                               std::make_unique<ast::TypeLiteral>("number"),
                               Symbol::Mutability::IMMUTABLE);

  EXPECT_EQ(symbol->get_kind(), Symbol::Kind::VARIABLE);
  EXPECT_EQ(symbol->get_name(), "test_var");
  EXPECT_EQ(symbol->get_mutability(), Symbol::Mutability::IMMUTABLE);
  EXPECT_FALSE(symbol->is_used());
  EXPECT_FALSE(symbol->is_initialized());
}

TEST_F(SymbolTableTest, SymbolUsageTracking) {
  auto symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "test_var",
                               std::make_unique<ast::TypeLiteral>("number"));

  // 초기 상태
  EXPECT_FALSE(symbol->is_used());
  EXPECT_FALSE(symbol->is_initialized());

  // 사용 표시
  symbol->mark_used();
  EXPECT_TRUE(symbol->is_used());

  // 초기화 표시
  symbol->mark_initialized();
  EXPECT_TRUE(symbol->is_initialized());
}

TEST_F(SymbolTableTest, SymbolMutability) {
  auto immutable_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "let_var",
                               std::make_unique<ast::TypeLiteral>("number"),
                               Symbol::Mutability::IMMUTABLE);

  auto mutable_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "mut_var",
                               std::make_unique<ast::TypeLiteral>("number"),
                               Symbol::Mutability::MUTABLE);

  EXPECT_EQ(immutable_symbol->get_mutability(), Symbol::Mutability::IMMUTABLE);
  EXPECT_EQ(mutable_symbol->get_mutability(), Symbol::Mutability::MUTABLE);
}

TEST_F(SymbolTableTest, SymbolLineTracking) {
  auto symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "test_var",
                               std::make_unique<ast::TypeLiteral>("number"));

  EXPECT_EQ(symbol->get_declaration_line(), 0); // 초기값

  symbol->set_declaration_line(42);
  EXPECT_EQ(symbol->get_declaration_line(), 42);
}

// =============================================================================
// Scope Tests
// =============================================================================

TEST_F(SymbolTableTest, ScopeCreation) {
  auto global_scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);
  EXPECT_EQ(global_scope->get_type(), Scope::ScopeType::GLOBAL);
  EXPECT_EQ(global_scope->get_parent(), nullptr);
}

TEST_F(SymbolTableTest, ScopeHierarchy) {
  auto global_scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);
  auto function_scope =
      global_scope->create_child_scope(Scope::ScopeType::FUNCTION);

  EXPECT_EQ(function_scope->get_type(), Scope::ScopeType::FUNCTION);
  EXPECT_EQ(function_scope->get_parent(), global_scope.get());
}

TEST_F(SymbolTableTest, ScopeSymbolDefinition) {
  auto scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);

  auto symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "test_var",
                               std::make_unique<ast::TypeLiteral>("number"));

  std::string symbol_name = symbol->get_name();

  EXPECT_TRUE(scope->define_symbol(std::move(symbol)));
  EXPECT_TRUE(scope->has_symbol(symbol_name));

  // 중복 정의 시도
  auto duplicate_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, symbol_name,
                               std::make_unique<ast::TypeLiteral>("string"));

  EXPECT_FALSE(scope->define_symbol(std::move(duplicate_symbol)));
}

TEST_F(SymbolTableTest, ScopeSymbolLookup) {
  auto global_scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);
  auto function_scope =
      global_scope->create_child_scope(Scope::ScopeType::FUNCTION);

  // 전역 스코프에 심볼 정의
  auto global_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "global_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  global_scope->define_symbol(std::move(global_symbol));

  // 함수 스코프에 심볼 정의
  auto local_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "local_var",
                               std::make_unique<ast::TypeLiteral>("string"));
  function_scope->define_symbol(std::move(local_symbol));

  // 조회 테스트
  EXPECT_TRUE(global_scope->has_symbol("global_var"));
  EXPECT_FALSE(global_scope->has_symbol("local_var"));

  EXPECT_TRUE(function_scope->has_symbol("local_var"));
  // 부모 스코프 조회는 lookup 메서드를 통해 이루어짐 (별도 구현 필요)
}

TEST_F(SymbolTableTest, ScopeUnusedSymbols) {
  auto scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);

  auto used_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "used_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  used_symbol->mark_used();

  auto unused_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "unused_var",
                               std::make_unique<ast::TypeLiteral>("string"));

  scope->define_symbol(std::move(used_symbol));
  scope->define_symbol(std::move(unused_symbol));

  auto unused_symbols = scope->get_unused_symbols();
  EXPECT_EQ(unused_symbols.size(), 1);
  EXPECT_EQ(unused_symbols[0]->get_name(), "unused_var");
}

TEST_F(SymbolTableTest, ScopeUninitializedSymbols) {
  auto scope = std::make_unique<Scope>(Scope::ScopeType::GLOBAL);

  auto initialized_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "initialized_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  initialized_symbol->mark_initialized();

  auto uninitialized_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "uninitialized_var",
                               std::make_unique<ast::TypeLiteral>("string"));

  scope->define_symbol(std::move(initialized_symbol));
  scope->define_symbol(std::move(uninitialized_symbol));

  auto uninitialized_symbols = scope->get_uninitialized_symbols();
  EXPECT_EQ(uninitialized_symbols.size(), 1);
  EXPECT_EQ(uninitialized_symbols[0]->get_name(), "uninitialized_var");
}

// =============================================================================
// SymbolTable Tests
// =============================================================================

TEST_F(SymbolTableTest, SymbolTableInitialization) {
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::GLOBAL);
}

TEST_F(SymbolTableTest, ScopeEnterExit) {
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::GLOBAL);

  symbol_table->enter_scope(Scope::ScopeType::FUNCTION);
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::FUNCTION);

  symbol_table->enter_scope(Scope::ScopeType::BLOCK);
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::BLOCK);

  symbol_table->exit_scope();
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::FUNCTION);

  symbol_table->exit_scope();
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::GLOBAL);
}

TEST_F(SymbolTableTest, SymbolDefinitionAndLookup) {
  auto symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "test_var",
                               std::make_unique<ast::TypeLiteral>("number"));

  EXPECT_TRUE(symbol_table->define_symbol(std::move(symbol)));
  EXPECT_TRUE(symbol_table->is_symbol_accessible("test_var"));

  auto found_symbol = symbol_table->lookup_symbol("test_var");
  EXPECT_NE(found_symbol, nullptr);
  EXPECT_EQ(found_symbol->get_name(), "test_var");
}

TEST_F(SymbolTableTest, SymbolShadowing) {
  // 전역 스코프에 심볼 정의
  auto global_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "var",
                               std::make_unique<ast::TypeLiteral>("number"));
  symbol_table->define_symbol(std::move(global_symbol));

  // 함수 스코프 진입 후 같은 이름의 심볼 정의
  symbol_table->enter_scope(Scope::ScopeType::FUNCTION);
  auto local_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "var",
                               std::make_unique<ast::TypeLiteral>("string"));
  symbol_table->define_symbol(std::move(local_symbol));

  // 현재 스코프에서는 로컬 심볼이 조회되어야 함
  auto found_symbol = symbol_table->lookup_symbol("var");
  EXPECT_NE(found_symbol, nullptr);
  EXPECT_EQ(found_symbol->get_type()->get_name(), "string");

  // 스코프 종료 후 전역 심볼이 조회되어야 함
  symbol_table->exit_scope();
  found_symbol = symbol_table->lookup_symbol("var");
  EXPECT_NE(found_symbol, nullptr);
  EXPECT_EQ(found_symbol->get_type()->get_name(), "number");
}

TEST_F(SymbolTableTest, SymbolRedefinitionInSameScope) {
  auto symbol1 =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "var",
                               std::make_unique<ast::TypeLiteral>("number"));

  auto symbol2 =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "var",
                               std::make_unique<ast::TypeLiteral>("string"));

  EXPECT_TRUE(symbol_table->define_symbol(std::move(symbol1)));
  EXPECT_FALSE(
      symbol_table->define_symbol(std::move(symbol2))); // 중복 정의 실패
}

TEST_F(SymbolTableTest, FindUnusedSymbols) {
  // 사용된 심볼
  auto used_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "used_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  used_symbol->mark_used();
  symbol_table->define_symbol(std::move(used_symbol));

  // 사용되지 않은 심볼
  auto unused_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "unused_var",
                               std::make_unique<ast::TypeLiteral>("string"));
  symbol_table->define_symbol(std::move(unused_symbol));

  auto unused_symbols = symbol_table->find_unused_symbols();
  EXPECT_EQ(unused_symbols.size(), 1);
  EXPECT_EQ(unused_symbols[0]->get_name(), "unused_var");
}

TEST_F(SymbolTableTest, FindUninitializedVariables) {
  // 초기화된 변수
  auto initialized_var =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "initialized_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  initialized_var->mark_initialized();
  symbol_table->define_symbol(std::move(initialized_var));

  // 초기화되지 않은 변수
  auto uninitialized_var =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "uninitialized_var",
                               std::make_unique<ast::TypeLiteral>("string"));
  symbol_table->define_symbol(std::move(uninitialized_var));

  // 함수는 초기화 체크 대상이 아님
  auto function_symbol =
      std::make_unique<Symbol>(Symbol::Kind::FUNCTION, "test_function",
                               std::make_unique<ast::TypeLiteral>("function"));
  symbol_table->define_symbol(std::move(function_symbol));

  auto uninitialized_vars = symbol_table->find_uninitialized_variables();
  EXPECT_EQ(uninitialized_vars.size(), 1);
  EXPECT_EQ(uninitialized_vars[0]->get_name(), "uninitialized_var");
}

// =============================================================================
// Type Compatibility Tests
// =============================================================================

TEST_F(SymbolTableTest, TypeCompatibilityBasic) {
  ast::TypeLiteral number_type1("number");
  ast::TypeLiteral number_type2("number");
  ast::TypeLiteral string_type("string");

  EXPECT_TRUE(symbol_table->are_types_compatible(number_type1, number_type2));
  EXPECT_FALSE(symbol_table->are_types_compatible(number_type1, string_type));
}

TEST_F(SymbolTableTest, TypeInference) {
  // 정수 리터럴 타입 추론
  ast::NumberLiteral int_literal(42);
  auto inferred_type = symbol_table->infer_type(int_literal);
  EXPECT_NE(inferred_type, nullptr);
  EXPECT_EQ(inferred_type->get_name(), "number");

  // 문자열 리터럴 타입 추론
  ast::StringLiteral string_literal("hello");
  inferred_type = symbol_table->infer_type(string_literal);
  EXPECT_NE(inferred_type, nullptr);
  EXPECT_EQ(inferred_type->get_name(), "string");

  // 불리언 리터럴 타입 추론
  ast::BooleanLiteral bool_literal(true);
  inferred_type = symbol_table->infer_type(bool_literal);
  EXPECT_NE(inferred_type, nullptr);
  EXPECT_EQ(inferred_type->get_name(), "boolean");
}

// =============================================================================
// Complex Scenarios Tests
// =============================================================================

TEST_F(SymbolTableTest, NestedScopes) {
  // 전역 -> 함수 -> 블록 -> 루프 스코프
  symbol_table->enter_scope(Scope::ScopeType::FUNCTION);
  symbol_table->enter_scope(Scope::ScopeType::BLOCK);
  symbol_table->enter_scope(Scope::ScopeType::LOOP);

  // 각 스코프에서 심볼 정의
  auto loop_symbol =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "loop_var",
                               std::make_unique<ast::TypeLiteral>("number"));
  symbol_table->define_symbol(std::move(loop_symbol));

  EXPECT_TRUE(symbol_table->is_symbol_accessible("loop_var"));
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::LOOP);

  // 스코프 순차적 종료
  symbol_table->exit_scope(); // LOOP -> BLOCK
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::BLOCK);
  EXPECT_FALSE(symbol_table->is_symbol_accessible("loop_var"));

  symbol_table->exit_scope(); // BLOCK -> FUNCTION
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::FUNCTION);

  symbol_table->exit_scope(); // FUNCTION -> GLOBAL
  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::GLOBAL);
}

TEST_F(SymbolTableTest, MultipleSymbolTypes) {
  // 변수
  auto variable =
      std::make_unique<Symbol>(Symbol::Kind::VARIABLE, "my_var",
                               std::make_unique<ast::TypeLiteral>("number"));

  // 함수
  auto function =
      std::make_unique<Symbol>(Symbol::Kind::FUNCTION, "my_function",
                               std::make_unique<ast::TypeLiteral>("function"));

  // 구조체
  auto struct_symbol =
      std::make_unique<Symbol>(Symbol::Kind::STRUCT, "MyStruct",
                               std::make_unique<ast::TypeLiteral>("struct"));

  EXPECT_TRUE(symbol_table->define_symbol(std::move(variable)));
  EXPECT_TRUE(symbol_table->define_symbol(std::move(function)));
  EXPECT_TRUE(symbol_table->define_symbol(std::move(struct_symbol)));

  EXPECT_TRUE(symbol_table->is_symbol_accessible("my_var"));
  EXPECT_TRUE(symbol_table->is_symbol_accessible("my_function"));
  EXPECT_TRUE(symbol_table->is_symbol_accessible("MyStruct"));

  auto found_var = symbol_table->lookup_symbol("my_var");
  auto found_func = symbol_table->lookup_symbol("my_function");
  auto found_struct = symbol_table->lookup_symbol("MyStruct");

  EXPECT_EQ(found_var->get_kind(), Symbol::Kind::VARIABLE);
  EXPECT_EQ(found_func->get_kind(), Symbol::Kind::FUNCTION);
  EXPECT_EQ(found_struct->get_kind(), Symbol::Kind::STRUCT);
}

// =============================================================================
// Performance and Stress Tests
// =============================================================================

TEST_F(SymbolTableTest, PerformanceTestManySymbols) {
  auto start = std::chrono::high_resolution_clock::now();

  // 1000개의 심볼 추가
  for (size_t i = 0; i < 1000; ++i) {
    auto symbol = std::make_unique<Symbol>(
        Symbol::Kind::VARIABLE, "var_" + std::to_string(i),
        std::make_unique<ast::TypeLiteral>("number"));
    symbol_table->define_symbol(std::move(symbol));
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 1000개 심볼 추가가 100ms 미만이어야 함
  EXPECT_LT(duration.count(), 100)
      << "Symbol definition too slow: " << duration.count() << "ms";

  // 조회 성능 테스트
  start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < 1000; ++i) {
    std::string symbol_name = "var_" + std::to_string(i);
    EXPECT_TRUE(symbol_table->is_symbol_accessible(symbol_name));
  }

  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 1000번 조회가 50ms 미만이어야 함
  EXPECT_LT(duration.count(), 50)
      << "Symbol lookup too slow: " << duration.count() << "ms";
}

TEST_F(SymbolTableTest, StressTestDeepNesting) {
  // 100개의 중첩 스코프 생성
  for (int i = 0; i < 100; ++i) {
    symbol_table->enter_scope(Scope::ScopeType::BLOCK);

    auto symbol = std::make_unique<Symbol>(
        Symbol::Kind::VARIABLE, "nested_var_" + std::to_string(i),
        std::make_unique<ast::TypeLiteral>("number"));
    symbol_table->define_symbol(std::move(symbol));
  }

  // 가장 깊은 스코프에서 모든 심볼에 접근 가능한지 확인
  for (int i = 0; i < 100; ++i) {
    std::string symbol_name = "nested_var_" + std::to_string(i);
    EXPECT_TRUE(symbol_table->is_symbol_accessible(symbol_name));
  }

  // 스코프 순차적 종료
  for (int i = 0; i < 100; ++i) {
    symbol_table->exit_scope();
  }

  EXPECT_EQ(symbol_table->get_current_scope_type(), Scope::ScopeType::GLOBAL);
}