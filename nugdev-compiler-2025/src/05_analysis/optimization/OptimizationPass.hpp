#pragma once

#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/core/ASTVisitor.h"
#include <memory>
#include <string>
#include <vector>

namespace nugdev::compiler::optimization {

/**
 * @brief 최적화 패스의 기본 인터페이스
 *
 * 모든 최적화 알고리즘은 이 클래스를 상속받아 구현합니다.
 */
class OptimizationPass {
public:
  virtual ~OptimizationPass() = default;

  /**
   * @brief 최적화를 실행합니다
   * @param node 최적화할 AST 노드
   * @return 최적화가 적용되었으면 true, 아니면 false
   */
  virtual bool run(ast::ASTNode &node) = 0;

  /**
   * @brief 최적화 패스의 이름을 반환합니다
   */
  virtual std::string get_name() const = 0;

  /**
   * @brief 최적화 패스의 설명을 반환합니다
   */
  virtual std::string get_description() const = 0;

  /**
   * @brief 이 최적화 패스가 다른 패스와 호환되는지 확인합니다
   */
  virtual bool is_compatible_with(const OptimizationPass &other) const {
    // 기본적으로 모든 패스는 호환됩니다
    return true;
  }

  /**
   * @brief 최적화 결과에 대한 통계 정보
   */
  struct Statistics {
    size_t nodes_processed = 0;
    size_t nodes_optimized = 0;
    size_t bytes_saved = 0;

    void reset() {
      nodes_processed = 0;
      nodes_optimized = 0;
      bytes_saved = 0;
    }
  };

  const Statistics &get_statistics() const { return m_stats; }
  void reset_statistics() { m_stats.reset(); }

protected:
  Statistics m_stats;
};

/**
 * @brief 여러 최적화 패스를 관리하는 매니저 클래스
 */
class OptimizationManager {
public:
  OptimizationManager() = default;

  // 최적화 패스 등록
  void add_pass(std::unique_ptr<OptimizationPass> pass);
  void remove_pass(const std::string &pass_name);

  // 최적화 실행
  bool run_all_passes(ast::ASTNode &root);
  bool run_pass(const std::string &pass_name, ast::ASTNode &root);

  // 최적화 레벨 설정
  enum class OptimizationLevel {
    O0, // 최적화 없음
    O1, // 기본 최적화
    O2, // 고급 최적화
    O3  // 공격적 최적화
  };

  void set_optimization_level(OptimizationLevel level);
  void configure_for_level(OptimizationLevel level);

  // 통계 및 정보
  void print_statistics() const;
  size_t get_pass_count() const { return m_passes.size(); }
  std::vector<std::string> get_pass_names() const;

private:
  std::vector<std::unique_ptr<OptimizationPass>> m_passes;
  OptimizationLevel m_level = OptimizationLevel::O1;

  // 최적화 레벨별 기본 패스 구성
  void setup_o1_passes();
  void setup_o2_passes();
  void setup_o3_passes();
};

/**
 * @brief AST 방문자 기반 최적화 패스의 기본 클래스
 */
class VisitorOptimizationPass : public OptimizationPass,
                                public ast::DefaultASTVisitor {
public:
  bool run(ast::ASTNode &node) override {
    m_optimization_applied = false;
    node.accept(*this);
    return m_optimization_applied;
  }

protected:
  bool m_optimization_applied = false;

  void mark_optimization_applied() {
    m_optimization_applied = true;
    m_stats.nodes_optimized++;
  }
};

} // namespace nugdev::compiler::optimization