#include "05_analysis/semantic/SymbolTable.hpp"
#include <algorithm>

namespace nugdev::compiler::analysis {

// Symbol 구현
Symbol::Symbol(Kind kind, const std::string &name,
               std::unique_ptr<ast::TypeLiteral> type, Mutability mutability)
    : m_kind(kind), m_name(name), m_type(std::move(type)),
      m_mutability(mutability) {}

const std::string &Symbol::get_name() const { return m_name; }

const ast::TypeLiteral *Symbol::get_type() const { return m_type.get(); }

// Scope 구현
Scope::Scope(ScopeType type, Scope *parent) : m_type(type), m_parent(parent) {}

bool Scope::define_symbol(std::unique_ptr<Symbol> symbol) {
  const std::string &name = symbol->get_name();

  // 현재 스코프에서 중복 선언 확인
  if (m_symbols.find(name) != m_symbols.end()) {
    return false; // 이미 존재함
  }

  m_symbols[name] = std::move(symbol);
  return true;
}

Symbol *Scope::lookup_symbol(const std::string &name) const {
  auto it = m_symbols.find(name);
  if (it != m_symbols.end()) {
    return it->second.get();
  }

  // 부모 스코프에서 검색
  if (m_parent) {
    return m_parent->lookup_symbol(name);
  }

  return nullptr;
}

Symbol *Scope::lookup_symbol_local(const std::string &name) const {
  auto it = m_symbols.find(name);
  if (it != m_symbols.end()) {
    return it->second.get();
  }
  return nullptr;
}

bool Scope::has_symbol(const std::string &name) const {
  return m_symbols.find(name) != m_symbols.end();
}

Scope *Scope::get_parent() const { return m_parent; }

Scope::ScopeType Scope::get_type() const { return m_type; }

const std::unordered_map<std::string, std::unique_ptr<Symbol>> &
Scope::get_symbols() const {
  return m_symbols;
}

Scope *Scope::create_child_scope(ScopeType type) {
  auto child = std::make_unique<Scope>(type, this);
  Scope *child_ptr = child.get();
  m_children.push_back(std::move(child));
  return child_ptr;
}

std::vector<Symbol *> Scope::get_unused_symbols() const {
  std::vector<Symbol *> unused;

  for (const auto &pair : m_symbols) {
    if (!pair.second->is_used()) {
      unused.push_back(pair.second.get());
    }
  }

  return unused;
}

std::vector<Symbol *> Scope::get_uninitialized_symbols() const {
  std::vector<Symbol *> uninitialized;

  for (const auto &pair : m_symbols) {
    if (pair.second->get_kind() == Symbol::Kind::VARIABLE &&
        !pair.second->is_initialized()) {
      uninitialized.push_back(pair.second.get());
    }
  }

  return uninitialized;
}

// SymbolTable 구현
SymbolTable::SymbolTable()
    : m_global_scope(std::make_unique<Scope>(Scope::ScopeType::GLOBAL)),
      m_current_scope(m_global_scope.get()) {}

void SymbolTable::enter_scope(Scope::ScopeType type) {
  m_current_scope = m_current_scope->create_child_scope(type);
}

void SymbolTable::exit_scope() {
  if (m_current_scope->get_parent()) {
    m_current_scope = m_current_scope->get_parent();
  }
}

Scope *SymbolTable::get_current_scope() const { return m_current_scope; }

Scope *SymbolTable::get_global_scope() const { return m_global_scope.get(); }

bool SymbolTable::define_symbol(std::unique_ptr<Symbol> symbol) {
  return m_current_scope->define_symbol(std::move(symbol));
}

Symbol *SymbolTable::lookup_symbol(const std::string &name) const {
  return m_current_scope->lookup_symbol(name);
}

bool SymbolTable::is_symbol_accessible(const std::string &name) const {
  return lookup_symbol(name) != nullptr;
}

void SymbolTable::mark_symbol_used(const std::string &name) {
  Symbol *symbol = lookup_symbol(name);
  if (symbol) {
    symbol->mark_used();
  }
}

std::vector<Symbol *> SymbolTable::find_unused_symbols() const {
  std::vector<Symbol *> unused;
  collect_unused_symbols_recursive(m_global_scope.get(), unused);
  return unused;
}

std::vector<Symbol *> SymbolTable::find_uninitialized_variables() const {
  std::vector<Symbol *> uninitialized;
  collect_uninitialized_variables_recursive(m_global_scope.get(),
                                            uninitialized);
  return uninitialized;
}

std::vector<std::string> SymbolTable::find_undefined_references() const {
  // 실제 구현에서는 AST를 순회하면서 사용된 식별자들을 수집하고
  // 정의되지 않은 것들을 찾아야 함
  // 여기서는 기본 구현만 제공
  std::vector<std::string> undefined;
  return undefined;
}

bool SymbolTable::are_types_compatible(const ast::TypeLiteral &type1,
                                       const ast::TypeLiteral &type2) const {
  // 간단한 타입 호환성 검사
  // 실제 구현에서는 더 정교한 타입 시스템 필요
  return type1.get_name() == type2.get_name();
}

std::unique_ptr<ast::TypeLiteral>
SymbolTable::infer_type(const ast::Expression &expression) const {
  // 간단한 타입 추론
  // 실제 구현에서는 더 정교한 타입 추론 시스템 필요

  // 기본 구현: 리터럴 타입 추론
  // TODO: 실제 표현식별 타입 추론 구현

  return nullptr; // 추론 실패
}

void SymbolTable::collect_unused_symbols_recursive(
    Scope *scope, std::vector<Symbol *> &unused) const {
  if (!scope)
    return;

  auto scope_unused = scope->get_unused_symbols();
  unused.insert(unused.end(), scope_unused.begin(), scope_unused.end());

  // 자식 스코프들도 재귀적으로 검사
  for (const auto &child : scope->m_children) {
    collect_unused_symbols_recursive(child.get(), unused);
  }
}

void SymbolTable::collect_uninitialized_variables_recursive(
    Scope *scope, std::vector<Symbol *> &uninitialized) const {
  if (!scope)
    return;

  auto scope_uninitialized = scope->get_uninitialized_symbols();
  uninitialized.insert(uninitialized.end(), scope_uninitialized.begin(),
                       scope_uninitialized.end());

  // 자식 스코프들도 재귀적으로 검사
  for (const auto &child : scope->m_children) {
    collect_uninitialized_variables_recursive(child.get(), uninitialized);
  }
}

// 유틸리티 함수들
std::string symbol_kind_to_string(Symbol::Kind kind) {
  switch (kind) {
  case Symbol::Kind::VARIABLE:
    return "variable";
  case Symbol::Kind::FUNCTION:
    return "function";
  case Symbol::Kind::PARAMETER:
    return "parameter";
  case Symbol::Kind::STRUCT:
    return "struct";
  case Symbol::Kind::INTERFACE:
    return "interface";
  case Symbol::Kind::TYPE_ALIAS:
    return "type_alias";
  default:
    return "unknown";
  }
}

std::string scope_type_to_string(Scope::ScopeType type) {
  switch (type) {
  case Scope::ScopeType::GLOBAL:
    return "global";
  case Scope::ScopeType::FUNCTION:
    return "function";
  case Scope::ScopeType::BLOCK:
    return "block";
  case Scope::ScopeType::LOOP:
    return "loop";
  case Scope::ScopeType::CONDITIONAL:
    return "conditional";
  default:
    return "unknown";
  }
}

} // namespace nugdev::compiler::analysis