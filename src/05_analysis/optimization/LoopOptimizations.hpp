#pragma once

#include "04_parsing/ast/control_flow/ControlFlow.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/optimization/OptimizationPass.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::optimization {

using BasicBlock = analysis::BasicBlock;

/**
 * @brief 루프 정보 분석 및 관리
 */
class LoopInfo {
public:
  struct Loop {
    BasicBlock *header;                          // 루프 헤더
    std::unordered_set<BasicBlock *> body;       // 루프 바디
    std::unordered_set<BasicBlock *> exits;      // 루프 출구
    std::vector<BasicBlock *> backedges;         // 백엣지들
    Loop *parent = nullptr;                      // 부모 루프 (중첩 루프)
    std::vector<std::unique_ptr<Loop>> children; // 자식 루프들
    size_t depth = 0;                            // 중첩 깊이

    // 루프 특성
    bool is_natural_loop() const;
    bool is_reducible() const;
    size_t get_trip_count() const; // 반복 횟수 (분석 가능한 경우)
  };

  explicit LoopInfo(const analysis::ControlFlowGraph &cfg);

  /**
   * @brief 루프 분석 수행
   */
  void analyze();

  /**
   * @brief 모든 루프 조회
   */
  const std::vector<std::unique_ptr<Loop>> &get_all_loops() const {
    return m_loops;
  }

  /**
   * @brief 블록이 속한 루프 조회
   */
  Loop *get_loop_for_block(BasicBlock *block) const;

  /**
   * @brief 최상위 루프들 조회
   */
  const std::vector<Loop *> &get_top_level_loops() const {
    return m_top_level_loops;
  }

private:
  const analysis::ControlFlowGraph &m_cfg;
  std::vector<std::unique_ptr<Loop>> m_loops;
  std::vector<Loop *> m_top_level_loops;
  std::unordered_map<BasicBlock *, Loop *> m_block_to_loop;

  void detect_natural_loops();
  void compute_loop_nesting();
  void analyze_loop_exits();
};

/**
 * @brief 루프 불변 코드 이동 (Loop Invariant Code Motion)
 */
class LoopInvariantCodeMotion : public VisitorOptimizationPass {
public:
  explicit LoopInvariantCodeMotion(const LoopInfo &loop_info);

  std::string get_name() const override { return "LoopInvariantCodeMotion"; }
  std::string get_description() const override {
    return "Moves loop-invariant code outside loops";
  }

  void visit(ast::ForStatement &node) override;

private:
  const LoopInfo &m_loop_info;
  std::unordered_set<std::string> m_loop_modified_variables;

  /**
   * @brief 표현식이 루프 불변인지 확인
   */
  bool is_loop_invariant(const ast::Expression &expr,
                         const LoopInfo::Loop &loop);

  /**
   * @brief 루프에서 수정되는 변수들 수집
   */
  void collect_modified_variables(const LoopInfo::Loop &loop);

  /**
   * @brief 안전한 호이스팅 위치 찾기
   */
  BasicBlock *find_safe_hoist_location(const LoopInfo::Loop &loop);

  /**
   * @brief 불변 코드 이동
   */
  void hoist_invariant_code(const LoopInfo::Loop &loop);
};

/**
 * @brief 루프 펼치기 (Loop Unrolling)
 */
class LoopUnroller : public OptimizationPass {
public:
  struct UnrollOptions {
    size_t max_unroll_factor = 4; // 최대 펼치기 배수
    size_t max_body_size = 100;   // 펼치기할 루프 바디 최대 크기
    bool partial_unroll = true;   // 부분 펼치기 허용
    bool runtime_unroll = false;  // 런타임 펼치기 허용
  };

  explicit LoopUnroller(const LoopInfo &loop_info,
                        const UnrollOptions &options = UnrollOptions{});

  std::string get_name() const override { return "LoopUnroller"; }
  std::string get_description() const override {
    return "Unrolls loops to reduce branch overhead";
  }

  bool run(ast::ASTNode &node) override;

private:
  const LoopInfo &m_loop_info;
  UnrollOptions m_options;

  /**
   * @brief 루프가 펼치기 가능한지 확인
   */
  bool can_unroll(const LoopInfo::Loop &loop);

  /**
   * @brief 최적의 펼치기 배수 계산
   */
  size_t calculate_unroll_factor(const LoopInfo::Loop &loop);

  /**
   * @brief 완전 펼치기 수행
   */
  std::unique_ptr<ast::Statement> full_unroll(const ast::ForStatement &loop,
                                              size_t trip_count);

  /**
   * @brief 부분 펼치기 수행
   */
  std::unique_ptr<ast::Statement> partial_unroll(const ast::ForStatement &loop,
                                                 size_t factor);

  /**
   * @brief 루프 바디 복제
   */
  std::unique_ptr<ast::Statement>
  clone_loop_body(const ast::Statement &body, const std::string &induction_var,
                  size_t iteration);
};

/**
 * @brief 루프 융합 (Loop Fusion)
 */
class LoopFusion : public OptimizationPass {
public:
  explicit LoopFusion(const LoopInfo &loop_info);

  std::string get_name() const override { return "LoopFusion"; }
  std::string get_description() const override {
    return "Fuses adjacent loops with compatible iteration patterns";
  }

  bool run(ast::ASTNode &node) override;

private:
  const LoopInfo &m_loop_info;

  /**
   * @brief 루프들이 융합 가능한지 확인
   */
  bool can_fuse_loops(const ast::ForStatement &loop1,
                      const ast::ForStatement &loop2);

  /**
   * @brief 반복 패턴이 호환되는지 확인
   */
  bool have_compatible_iteration_patterns(const ast::ForStatement &loop1,
                                          const ast::ForStatement &loop2);

  /**
   * @brief 데이터 의존성 검사
   */
  bool check_data_dependencies(const ast::ForStatement &loop1,
                               const ast::ForStatement &loop2);

  /**
   * @brief 루프들을 융합
   */
  std::unique_ptr<ast::ForStatement> fuse_loops(const ast::ForStatement &loop1,
                                                const ast::ForStatement &loop2);
};

/**
 * @brief 루프 교환 (Loop Interchange)
 */
class LoopInterchange : public OptimizationPass {
public:
  explicit LoopInterchange(const LoopInfo &loop_info);

  std::string get_name() const override { return "LoopInterchange"; }
  std::string get_description() const override {
    return "Interchanges nested loops for better cache locality";
  }

  bool run(ast::ASTNode &node) override;

private:
  const LoopInfo &m_loop_info;

  /**
   * @brief 중첩 루프 감지
   */
  std::vector<std::vector<ast::ForStatement *>>
  find_nested_loops(ast::ASTNode &root);

  /**
   * @brief 교환이 유리한지 분석
   */
  bool should_interchange(const ast::ForStatement &outer,
                          const ast::ForStatement &inner);

  /**
   * @brief 메모리 접근 패턴 분석
   */
  struct MemoryAccess {
    std::string variable;
    std::vector<std::string> indices; // 인덱스 변수들
  };

  std::vector<MemoryAccess>
  analyze_memory_accesses(const ast::Statement &loop_body);

  /**
   * @brief 캐시 지역성 비용 계산
   */
  double calculate_cache_cost(const std::vector<MemoryAccess> &accesses,
                              const std::vector<std::string> &loop_order);

  /**
   * @brief 루프 교환 수행
   */
  std::unique_ptr<ast::ForStatement>
  interchange_loops(const ast::ForStatement &outer,
                    const ast::ForStatement &inner);
};

/**
 * @brief 루프 벡터화 (Loop Vectorization)
 */
class LoopVectorizer : public OptimizationPass {
public:
  struct VectorizationInfo {
    size_t vector_width = 4;        // 벡터 폭 (SIMD 레지스터 크기)
    bool allow_unsafe_math = false; // 안전하지 않은 수학 연산 허용
    size_t min_trip_count = 8;      // 벡터화할 최소 반복 횟수
  };

  explicit LoopVectorizer(const LoopInfo &loop_info,
                          const VectorizationInfo &info = VectorizationInfo{});

  std::string get_name() const override { return "LoopVectorizer"; }
  std::string get_description() const override {
    return "Vectorizes loops for SIMD execution";
  }

  bool run(ast::ASTNode &node) override;

private:
  const LoopInfo &m_loop_info;
  VectorizationInfo m_vectorization_info;

  /**
   * @brief 루프가 벡터화 가능한지 확인
   */
  bool can_vectorize(const LoopInfo::Loop &loop);

  /**
   * @brief 벡터화 장애물 분석
   */
  enum class VectorizationBarrier {
    NONE,
    FUNCTION_CALL,     // 함수 호출
    COMPLEX_CONTROL,   // 복잡한 제어 흐름
    DATA_DEPENDENCY,   // 데이터 의존성
    MEMORY_ALIASING,   // 메모리 앨리어싱
    REDUCTION_PATTERN, // 리덕션 패턴
    IRREGULAR_ACCESS   // 불규칙한 메모리 접근
  };

  VectorizationBarrier
  analyze_vectorization_barriers(const LoopInfo::Loop &loop);

  /**
   * @brief 리덕션 패턴 감지
   */
  struct ReductionPattern {
    std::string accumulator;
    ast::BinaryExpression::Operator operation;
    bool is_commutative;
  };

  std::optional<ReductionPattern>
  detect_reduction_pattern(const LoopInfo::Loop &loop);

  /**
   * @brief 벡터화된 루프 생성
   */
  std::unique_ptr<ast::Statement>
  generate_vectorized_loop(const ast::ForStatement &original_loop,
                           size_t vector_width);
};

/**
 * @brief 루프 분할 (Loop Distribution/Fission)
 */
class LoopDistribution : public OptimizationPass {
public:
  explicit LoopDistribution(const LoopInfo &loop_info);

  std::string get_name() const override { return "LoopDistribution"; }
  std::string get_description() const override {
    return "Distributes loop iterations to improve vectorization and cache "
           "usage";
  }

  bool run(ast::ASTNode &node) override;

private:
  const LoopInfo &m_loop_info;

  /**
   * @brief 분할 가능한 문장들 그룹화
   */
  std::vector<std::vector<ast::Statement *>>
  group_distributable_statements(const ast::ForStatement &loop);

  /**
   * @brief 문장 간 의존성 분석
   */
  bool has_dependency(const ast::Statement &stmt1, const ast::Statement &stmt2);

  /**
   * @brief 루프를 여러 루프로 분할
   */
  std::vector<std::unique_ptr<ast::ForStatement>>
  distribute_loop(const ast::ForStatement &original_loop,
                  const std::vector<std::vector<ast::Statement *>> &groups);
};

} // namespace nugdev::compiler::optimization