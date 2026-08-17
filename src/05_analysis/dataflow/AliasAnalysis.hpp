#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/dataflow/DataFlowAnalysis.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 메모리 위치를 나타내는 추상 표현
 */
class AbstractLocation {
public:
  enum class Kind {
    VARIABLE,      // 일반 변수
    FIELD,         // 구조체 필드
    ARRAY_ELEMENT, // 배열 요소
    UNKNOWN        // 알 수 없는 위치
  };

  explicit AbstractLocation(Kind kind, const std::string &name = "");

  Kind get_kind() const { return m_kind; }
  const std::string &get_name() const { return m_name; }

  // 비교 연산자
  bool operator==(const AbstractLocation &other) const;
  bool operator!=(const AbstractLocation &other) const;

  std::string to_string() const;

private:
  Kind m_kind;
  std::string m_name;
  std::vector<std::string> m_path; // 필드 경로나 인덱스
};

/**
 * @brief 포인터/참조 관계를 나타내는 클래스
 */
class PointsToSet {
public:
  PointsToSet() = default;

  void add_location(const AbstractLocation &location);
  void remove_location(const AbstractLocation &location);
  bool may_point_to(const AbstractLocation &location) const;

  // 집합 연산
  PointsToSet operator|(const PointsToSet &other) const; // 합집합
  PointsToSet operator&(const PointsToSet &other) const; // 교집합

  bool is_empty() const { return m_locations.empty(); }
  size_t size() const { return m_locations.size(); }

  const std::unordered_set<AbstractLocation> &get_locations() const {
    return m_locations;
  }

private:
  std::unordered_set<AbstractLocation> m_locations;
};

/**
 * @brief 기본적인 Andersen 스타일 Points-to 분석
 */
class AndersonPointsToAnalysis {
public:
  explicit AndersonPointsToAnalysis(const ControlFlowGraph &cfg);

  /**
   * @brief Points-to 분석 실행
   */
  void analyze();

  /**
   * @brief 변수가 가리킬 수 있는 위치들 조회
   */
  PointsToSet get_points_to_set(const std::string &variable) const;

  /**
   * @brief 두 포인터가 같은 위치를 가리킬 수 있는지 확인
   */
  bool may_alias(const std::string &ptr1, const std::string &ptr2) const;

  /**
   * @brief 확실히 같은 위치를 가리키는지 확인
   */
  bool must_alias(const std::string &ptr1, const std::string &ptr2) const;

private:
  const ControlFlowGraph &m_cfg;
  std::unordered_map<std::string, PointsToSet> m_points_to_map;

  // 제약 조건들
  struct Constraint {
    enum class Type {
      COPY,   // p = q
      LOAD,   // p = *q
      STORE,  // *p = q
      ADDRESS // p = &x
    };

    Type type;
    std::string lhs;
    std::string rhs;
    std::optional<std::string> field; // 필드 접근시
  };

  std::vector<Constraint> m_constraints;

  /**
   * @brief 제약 조건 생성
   */
  void generate_constraints();

  /**
   * @brief 워크리스트 알고리즘으로 해결
   */
  void solve_constraints();

  /**
   * @brief AST 노드에서 제약 조건 추출
   */
  void extract_constraints_from_statement(const ast::Statement &stmt);
  void extract_constraints_from_expression(const ast::Expression &expr);
};

/**
 * @brief 더 정확한 Steensgaard 스타일 분석
 */
class SteensgaardAnalysis {
public:
  explicit SteensgaardAnalysis(const ControlFlowGraph &cfg);

  void analyze();

  /**
   * @brief 두 표현식이 별명일 가능성
   */
  enum class AliasResult {
    NO_ALIAS,  // 확실히 별명 아님
    MAY_ALIAS, // 별명일 가능성 있음
    MUST_ALIAS // 확실히 별명임
  };

  AliasResult query_alias(const ast::Expression &expr1,
                          const ast::Expression &expr2) const;

private:
  const ControlFlowGraph &m_cfg;

  // Union-Find 자료구조로 동등 클래스 관리
  class UnionFind {
  public:
    explicit UnionFind(size_t size);

    size_t find(size_t x);
    void unite(size_t x, size_t y);
    bool same(size_t x, size_t y);

  private:
    std::vector<size_t> m_parent;
    std::vector<size_t> m_rank;
  };

  std::unique_ptr<UnionFind> m_union_find;
  std::unordered_map<std::string, size_t> m_variable_to_id;
  size_t m_next_id = 0;

  size_t get_or_create_id(const std::string &variable);
  void unify_expressions(const ast::Expression &expr1,
                         const ast::Expression &expr2);
};

/**
 * @brief 메모리 안전성 분석
 */
class MemorySafetyAnalyzer {
public:
  explicit MemorySafetyAnalyzer(const ControlFlowGraph &cfg);

  /**
   * @brief 메모리 안전성 문제들 분석
   */
  struct SafetyViolation {
    enum class Kind {
      NULL_DEREFERENCE, // null 포인터 역참조
      BUFFER_OVERFLOW,  // 버퍼 오버플로우
      USE_AFTER_FREE,   // 해제 후 사용
      DOUBLE_FREE,      // 이중 해제
      MEMORY_LEAK       // 메모리 누수
    };

    Kind kind;
    std::string description;
    const ast::ASTNode *location;
  };

  std::vector<SafetyViolation> analyze_memory_safety();

private:
  const ControlFlowGraph &m_cfg;
  std::unique_ptr<AndersonPointsToAnalysis> m_points_to_analysis;

  /**
   * @brief null 포인터 역참조 검사
   */
  std::vector<SafetyViolation> check_null_dereferences();

  /**
   * @brief 배열 경계 검사
   */
  std::vector<SafetyViolation> check_buffer_overflows();

  /**
   * @brief 메모리 생명주기 추적
   */
  std::vector<SafetyViolation> check_memory_lifecycle();

  /**
   * @brief 표현식이 null일 가능성 확인
   */
  bool may_be_null(const ast::Expression &expr) const;

  /**
   * @brief 배열 인덱스가 유효한 범위인지 확인
   */
  bool is_valid_array_access(const ast::Expression &array,
                             const ast::Expression &index) const;
};

/**
 * @brief 별명 분석 결과를 활용한 최적화 지원
 */
class AliasAwareOptimizer {
public:
  explicit AliasAwareOptimizer(const AndersonPointsToAnalysis &alias_analysis);

  /**
   * @brief 메모리 접근 최적화 기회 탐지
   */
  struct OptimizationOpportunity {
    enum class Kind {
      REDUNDANT_LOAD,     // 중복 로드
      DEAD_STORE,         // 죽은 저장
      LOAD_STORE_FORWARD, // 로드-저장 전달
      MEMORY_COALESCING   // 메모리 접근 병합
    };

    Kind kind;
    std::string description;
    const ast::ASTNode *location;
    double estimated_benefit;
  };

  std::vector<OptimizationOpportunity>
  find_optimization_opportunities(const ControlFlowGraph &cfg);

private:
  const AndersonPointsToAnalysis &m_alias_analysis;

  /**
   * @brief 중복 메모리 로드 감지
   */
  std::vector<OptimizationOpportunity>
  find_redundant_loads(const ControlFlowGraph &cfg);

  /**
   * @brief 무의미한 저장 연산 감지
   */
  std::vector<OptimizationOpportunity>
  find_dead_stores(const ControlFlowGraph &cfg);

  /**
   * @brief 두 메모리 접근이 같은 위치인지 확인
   */
  bool same_memory_location(const ast::Expression &expr1,
                            const ast::Expression &expr2) const;
};

} // namespace nugdev::compiler::analysis