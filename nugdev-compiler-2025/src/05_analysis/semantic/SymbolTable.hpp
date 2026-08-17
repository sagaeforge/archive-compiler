#pragma once

#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/types/Types.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 심볼 정보를 저장하는 클래스
 */
class Symbol {
public:
  enum class Kind {
    VARIABLE,
    FUNCTION,
    PARAMETER,
    STRUCT,
    INTERFACE,
    TYPE_ALIAS
  };

  enum class Mutability {
    IMMUTABLE, // let
    MUTABLE    // mut
  };

  Symbol(Kind kind, const std::string &name,
         std::unique_ptr<ast::TypeLiteral> type,
         Mutability mutability = Mutability::IMMUTABLE);

  // Getters
  Kind get_kind() const { return m_kind; }
  const std::string &get_name() const { return m_name; }
  const ast::TypeLiteral *get_type() const { return m_type.get(); }
  Mutability get_mutability() const { return m_mutability; }
  size_t get_declaration_line() const { return m_declaration_line; }
  bool is_used() const { return m_is_used; }
  bool is_initialized() const { return m_is_initialized; }

  // Setters
  void set_declaration_line(size_t line) { m_declaration_line = line; }
  void mark_used() { m_is_used = true; }
  void mark_initialized() { m_is_initialized = true; }

private:
  Kind m_kind;
  std::string m_name;
  std::unique_ptr<ast::TypeLiteral> m_type;
  Mutability m_mutability;
  size_t m_declaration_line = 0;
  bool m_is_used = false;
  bool m_is_initialized = false;
};

/**
 * @brief 스코프 정보를 관리하는 클래스
 */
class Scope {
public:
  enum class ScopeType { GLOBAL, FUNCTION, BLOCK, LOOP, CONDITIONAL };

  explicit Scope(ScopeType type, Scope *parent = nullptr);

  // 심볼 관리
  bool define_symbol(std::unique_ptr<Symbol> symbol);
  Symbol *lookup_symbol(const std::string &name, bool recursive = true);
  bool has_symbol(const std::string &name) const;

  // 스코프 관리
  ScopeType get_type() const { return m_type; }
  Scope *get_parent() const { return m_parent; }
  const std::vector<std::unique_ptr<Scope>> &get_children() const {
    return m_children;
  }

  Scope *create_child_scope(ScopeType type);

  // 분석 결과
  std::vector<Symbol *> get_unused_symbols() const;
  std::vector<Symbol *> get_uninitialized_symbols() const;

private:
  ScopeType m_type;
  Scope *m_parent;
  std::vector<std::unique_ptr<Scope>> m_children;
  std::unordered_map<std::string, std::unique_ptr<Symbol>> m_symbols;
};

/**
 * @brief 전체 심볼 테이블을 관리하는 클래스
 */
class SymbolTable {
public:
  SymbolTable();

  // 스코프 관리
  void enter_scope(Scope::ScopeType type);
  void exit_scope();
  Scope *get_current_scope() const { return m_current_scope; }
  Scope *get_global_scope() const { return m_global_scope.get(); }

  // 심볼 관리
  bool define_symbol(std::unique_ptr<Symbol> symbol);
  Symbol *lookup_symbol(const std::string &name);
  bool is_symbol_accessible(const std::string &name) const;

  // 분석 기능
  std::vector<Symbol *> find_unused_symbols() const;
  std::vector<Symbol *> find_uninitialized_variables() const;
  std::vector<std::string> find_undefined_references() const;

  // 타입 검사 지원
  bool are_types_compatible(const ast::TypeLiteral &type1,
                            const ast::TypeLiteral &type2) const;
  std::unique_ptr<ast::TypeLiteral>
  infer_type(const ast::Expression &expr) const;

private:
  std::unique_ptr<Scope> m_global_scope;
  Scope *m_current_scope;

  void collect_unused_symbols_recursive(Scope *scope,
                                        std::vector<Symbol *> &unused) const;
};

} // namespace nugdev::compiler::analysis