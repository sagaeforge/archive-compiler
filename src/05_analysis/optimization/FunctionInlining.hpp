#pragma once

#include "04_parsing/ast/statements/Statements.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/optimization/OptimizationPass.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::optimization {

/**
 * @brief 호출 그래프 (Call Graph) 구축 및 관리
 */
class CallGraph {
public:
  struct CallSite {
    ast::FunctionDeclaration *caller;
    ast::FunctionDeclaration *callee;
    const ast::PostfixExpression *call_expression;
    size_t call_count; // 정적 분석 기반 추정 호출 횟수
    bool is_virtual_call;

    CallSite(ast::FunctionDeclaration *caller_func,
             ast::FunctionDeclaration *callee_func,
             const ast::PostfixExpression *call_expr)
        : caller(caller_func), callee(callee_func), call_expression(call_expr),
          call_count(1), is_virtual_call(false) {}
  };

  struct FunctionNode {
    ast::FunctionDeclaration *function;
    std::vector<CallSite *> outgoing_calls; // 이 함수가 호출하는 함수들
    std::vector<CallSite *> incoming_calls; // 이 함수를 호출하는 곳들
    size_t estimated_size;                  // 함수 크기 추정
    bool is_recursive;
    bool is_leaf_function;

    explicit FunctionNode(ast::FunctionDeclaration *func)
        : function(func), estimated_size(0), is_recursive(false),
          is_leaf_function(true) {}
  };

  explicit CallGraph();

  /**
   * @brief 프로그램에서 호출 그래프 구축
   */
  void build_call_graph(ast::Program &program);

  /**
   * @brief 함수 노드 조회
   */
  FunctionNode *get_function_node(const std::string &function_name);
  const FunctionNode *get_function_node(const std::string &function_name) const;

  /**
   * @brief 모든 함수들 조회
   */
  const std::unordered_map<std::string, std::unique_ptr<FunctionNode>> &
  get_functions() const {
    return m_functions;
  }

  /**
   * @brief 재귀 함수 감지
   */
  std::vector<std::vector<FunctionNode *>> find_strongly_connected_components();

  /**
   * @brief 호출 빈도 분석 (핫 함수 찾기)
   */
  std::vector<FunctionNode *> get_hot_functions(size_t top_n = 10);

private:
  std::unordered_map<std::string, std::unique_ptr<FunctionNode>> m_functions;
  std::vector<std::unique_ptr<CallSite>> m_call_sites;

  void collect_function_declarations(ast::ASTNode &root);
  void collect_function_calls(ast::ASTNode &root);
  void analyze_recursion();
  void estimate_function_sizes();
};

/**
 * @brief 인라이닝 결정 분석기
 */
class InliningAnalyzer {
public:
  struct InliningCandidate {
    CallGraph::CallSite *call_site;
    double benefit_score; // 인라이닝 이익 점수
    double cost_score;    // 인라이닝 비용 점수
    std::string reason;   // 인라이닝 결정 이유
    bool should_inline;

    InliningCandidate(CallGraph::CallSite *site)
        : call_site(site), benefit_score(0.0), cost_score(0.0),
          should_inline(false) {}
  };

  struct InliningOptions {
    size_t max_function_size;     // 인라인할 최대 함수 크기
    size_t max_inline_depth;      // 최대 인라인 깊이
    double size_growth_threshold; // 허용 가능한 코드 크기 증가율
    bool inline_small_functions;  // 작은 함수 자동 인라인
    bool inline_hot_functions;    // 자주 호출되는 함수 인라인
    bool avoid_code_bloat;        // 코드 비대화 방지

    InliningOptions()
        : max_function_size(100), max_inline_depth(5),
          size_growth_threshold(0.2), inline_small_functions(true),
          inline_hot_functions(true), avoid_code_bloat(true) {}
  };

  explicit InliningAnalyzer(const CallGraph &call_graph,
                            const InliningOptions &options = InliningOptions{});

  /**
   * @brief 인라이닝 후보들 분석
   */
  std::vector<InliningCandidate> analyze_inlining_candidates();

  /**
   * @brief 특정 호출 사이트에 대한 인라이닝 결정
   */
  bool should_inline(const CallGraph::CallSite &call_site);

private:
  const CallGraph &m_call_graph;
  InliningOptions m_options;

  /**
   * @brief 인라이닝 이익 계산
   */
  double calculate_inlining_benefit(const CallGraph::CallSite &call_site);

  /**
   * @brief 인라이닝 비용 계산
   */
  double calculate_inlining_cost(const CallGraph::CallSite &call_site);

  /**
   * @brief 함수 호출 오버헤드 추정
   */
  double estimate_call_overhead(const CallGraph::CallSite &call_site);

  /**
   * @brief 코드 크기 증가 추정
   */
  size_t estimate_code_size_increase(const CallGraph::CallSite &call_site);

  /**
   * @brief 인라인 체인 분석 (A -> B -> C)
   */
  void analyze_inlining_chains(std::vector<InliningCandidate> &candidates);

  /**
   * @brief 재귀 함수 인라이닝 분석
   */
  bool can_inline_recursive_function(const CallGraph::FunctionNode &function);
};

/**
 * @brief 함수 인라이닝 변환기
 */
class FunctionInliner : public OptimizationPass {
public:
  explicit FunctionInliner(const CallGraph &call_graph,
                           const InliningAnalyzer &analyzer);

  std::string get_name() const override { return "FunctionInliner"; }
  std::string get_description() const override {
    return "Inlines function calls to reduce call overhead";
  }

  bool run(ast::ASTNode &node) override;

private:
  const CallGraph &m_call_graph;
  const InliningAnalyzer &m_analyzer;
  std::unordered_map<std::string, size_t>
      m_inline_counts; // 각 함수별 인라인 횟수

  /**
   * @brief 함수 호출을 인라인으로 대체
   */
  std::unique_ptr<ast::Statement>
  inline_function_call(const ast::PostfixExpression &call_expr,
                       const ast::FunctionDeclaration &target_function);

  /**
   * @brief 매개변수를 인수로 대체
   */
  void substitute_parameters(
      ast::ASTNode &inlined_body,
      const std::vector<std::unique_ptr<ast::Parameter>> &parameters,
      const std::vector<std::unique_ptr<ast::Expression>> &arguments);

  /**
   * @brief 변수 이름 충돌 방지를 위한 이름 변경
   */
  void rename_local_variables(ast::ASTNode &inlined_body,
                              const std::string &suffix);

  /**
   * @brief return 문을 적절히 변환
   */
  void transform_return_statements(ast::ASTNode &inlined_body,
                                   const std::string &result_variable);

  /**
   * @brief 인라인 깊이 제한 검사
   */
  bool exceeds_inline_depth_limit(const std::string &function_name);
};

} // namespace nugdev::compiler::optimization