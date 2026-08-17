#pragma once

#include "05_analysis/semantic/CompileTimeEvaluator.hpp"
#include "05_analysis/semantic/StrongTypeSystem.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 컴파일 타임 계산 결과를 캐싱하는 시스템
 *
 * 컴파일 타임 계산의 성능을 크게 향상시키기 위해:
 * - constexpr 함수 호출 결과 캐싱
 * - 타입 추론 결과 캐싱
 * - 메타프로그래밍 계산 결과 캐싱
 * - LRU 정책으로 메모리 관리
 */
class CompileTimeCache {
public:
  struct CacheEntry {
    CompileTimeValue value;
    std::chrono::steady_clock::time_point timestamp;
    size_t access_count = 0;
    bool is_immutable = true; // 변경되지 않는 계산 결과

    CacheEntry(CompileTimeValue val)
        : value(std::move(val)), timestamp(std::chrono::steady_clock::now()) {}
  };

  explicit CompileTimeCache(size_t max_size = 1000);

  /**
   * @brief 컴파일 타임 계산 결과 캐싱
   */
  bool cache_result(const std::string &key, const CompileTimeValue &value);

  /**
   * @brief 캐시된 결과 조회
   */
  std::optional<CompileTimeValue> get_cached_result(const std::string &key);

  /**
   * @brief constexpr 함수 호출 결과 캐싱
   */
  bool cache_function_result(const std::string &function_name,
                             const std::vector<CompileTimeValue> &args,
                             const CompileTimeValue &result);

  std::optional<CompileTimeValue>
  get_cached_function_result(const std::string &function_name,
                             const std::vector<CompileTimeValue> &args);

  /**
   * @brief 타입 계산 결과 캐싱
   */
  bool cache_type_calculation(const std::string &expression_hash,
                              std::unique_ptr<StrongType> type);

  std::unique_ptr<StrongType>
  get_cached_type(const std::string &expression_hash);

  /**
   * @brief 캐시 통계 및 관리
   */
  struct CacheStatistics {
    size_t total_entries = 0;
    size_t hits = 0;
    size_t misses = 0;
    size_t evictions = 0;
    double hit_rate = 0.0;
    size_t memory_usage_bytes = 0;
  };

  CacheStatistics get_statistics() const;
  void clear_cache();
  void evict_expired_entries(std::chrono::seconds max_age);

  // 성능 튜닝
  void set_max_size(size_t new_max_size);
  void enable_aggressive_caching(bool enabled);

private:
  size_t m_max_size;
  bool m_aggressive_caching = false;

  // 다양한 타입의 캐시들
  std::unordered_map<std::string, CacheEntry> m_compile_time_cache;
  std::unordered_map<std::string, std::unique_ptr<StrongType>> m_type_cache;
  std::unordered_map<std::string, CompileTimeValue> m_function_cache;

  // 캐시 통계
  mutable CacheStatistics m_stats;

  // LRU 관리를 위한 액세스 순서
  std::list<std::string> m_access_order;
  std::unordered_map<std::string, std::list<std::string>::iterator>
      m_access_map;

  // 내부 헬퍼 메서드들
  std::string
  generate_function_key(const std::string &function_name,
                        const std::vector<CompileTimeValue> &args) const;

  void update_access_order(const std::string &key);
  void evict_lru_entry();
  bool should_cache_value(const CompileTimeValue &value) const;
  size_t estimate_memory_usage(const CompileTimeValue &value) const;

  void update_statistics() const;
};

/**
 * @brief 컴파일 타임 성능 최적화를 위한 힌트 시스템
 */
class CompileTimePerformanceOptimizer {
public:
  struct OptimizationHint {
    enum class Type {
      CACHE_THIS_CALCULATION,    // 이 계산을 캐시하라
      AVOID_REPEATED_EVALUATION, // 반복 계산 피하라
      USE_MEMOIZATION,           // 메모이제이션 사용
      PRECOMPUTE_AT_STARTUP,     // 시작 시 미리 계산
      LAZY_EVALUATION            // 지연 계산 사용
    };

    Type type;
    std::string description;
    const ast::ASTNode *target_node;
    double estimated_speedup;
  };

  static std::vector<OptimizationHint>
  analyze_performance_opportunities(const ast::ASTNode &root,
                                    const CompileTimeCache &cache);

  /**
   * @brief 자주 사용되는 계산 패턴 감지
   */
  static std::vector<std::string>
  detect_frequent_patterns(const std::vector<std::string> &calculation_history);

  /**
   * @brief 컴파일 타임 비용이 높은 연산 식별
   */
  static std::vector<const ast::ASTNode *>
  identify_expensive_operations(const ast::ASTNode &root);

private:
  static bool is_expensive_operation(const ast::ASTNode &node);
  static size_t estimate_computation_cost(const ast::ASTNode &node);
};

/**
 * @brief 컴파일 타임 계산의 메모이제이션 지원
 */
template <typename ResultType> class CompileTimeMemoizer {
public:
  using ComputeFunction =
      std::function<ResultType(const std::vector<CompileTimeValue> &)>;

  explicit CompileTimeMemoizer(ComputeFunction compute_func,
                               size_t max_cache_size = 100)
      : m_compute_func(std::move(compute_func)),
        m_max_cache_size(max_cache_size) {}

  ResultType compute(const std::vector<CompileTimeValue> &inputs) {
    std::string key = generate_key(inputs);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
      // 캐시 히트
      it->second.access_count++;
      return it->second.result;
    }

    // 캐시 미스 - 계산 수행
    ResultType result = m_compute_func(inputs);

    // 결과 캐싱
    if (m_cache.size() >= m_max_cache_size) {
      evict_least_used();
    }

    m_cache[key] = {result, 1, std::chrono::steady_clock::now()};
    return result;
  }

  void clear() { m_cache.clear(); }
  size_t cache_size() const { return m_cache.size(); }

private:
  struct CachedResult {
    ResultType result;
    size_t access_count;
    std::chrono::steady_clock::time_point last_access;
  };

  ComputeFunction m_compute_func;
  size_t m_max_cache_size;
  std::unordered_map<std::string, CachedResult> m_cache;

  std::string generate_key(const std::vector<CompileTimeValue> &inputs) const {
    // 입력값들을 해시해서 키 생성
    std::string key;
    for (const auto &input : inputs) {
      key += input.to_string() + "|";
    }
    return key;
  }

  void evict_least_used() {
    if (m_cache.empty())
      return;

    auto min_it = std::min_element(
        m_cache.begin(), m_cache.end(), [](const auto &a, const auto &b) {
          return a.second.access_count < b.second.access_count;
        });

    m_cache.erase(min_it);
  }
};

} // namespace nugdev::compiler::analysis