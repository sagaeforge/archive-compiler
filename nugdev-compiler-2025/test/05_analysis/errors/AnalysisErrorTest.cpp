#include "05_analysis/errors/AnalysisError.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace nugdev::compiler::analysis;

class AnalysisErrorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 테스트용 기본 에러 생성
    type_error =
        AnalysisError(AnalysisErrorType::TYPE_MISMATCH,
                      "Type mismatch: expected 'number', got 'string'", 42, 15);

    undefined_error =
        AnalysisError(AnalysisErrorType::UNDEFINED_SYMBOL,
                      "Variable 'undefined_var' is not declared", 10, 5);
  }

  AnalysisError type_error;
  AnalysisError undefined_error;
};

// =============================================================================
// Basic Constructor and Getter Tests
// =============================================================================

TEST_F(AnalysisErrorTest, ConstructorAndBasicGetters) {
  EXPECT_EQ(type_error.get_type(), AnalysisErrorType::TYPE_MISMATCH);
  EXPECT_EQ(type_error.get_message(),
            "Type mismatch: expected 'number', got 'string'");
  EXPECT_EQ(type_error.get_line(), 42);
  EXPECT_EQ(type_error.get_column(), 15);
}

TEST_F(AnalysisErrorTest, DefaultConstructorValues) {
  AnalysisError default_error(AnalysisErrorType::SCOPE_ERROR, "Test error");

  EXPECT_EQ(default_error.get_type(), AnalysisErrorType::SCOPE_ERROR);
  EXPECT_EQ(default_error.get_message(), "Test error");
  EXPECT_EQ(default_error.get_line(), 0);
  EXPECT_EQ(default_error.get_column(), 0);
}

// =============================================================================
// Severity Tests
// =============================================================================

TEST_F(AnalysisErrorTest, DefaultSeverity) {
  // 기본 심각도는 ERROR이어야 함
  EXPECT_EQ(type_error.get_severity(), AnalysisError::Severity::ERROR);
}

TEST_F(AnalysisErrorTest, SetAndGetSeverity) {
  type_error.set_severity(AnalysisError::Severity::WARNING);
  EXPECT_EQ(type_error.get_severity(), AnalysisError::Severity::WARNING);

  type_error.set_severity(AnalysisError::Severity::FATAL);
  EXPECT_EQ(type_error.get_severity(), AnalysisError::Severity::FATAL);
}

// =============================================================================
// String Representation Tests
// =============================================================================

TEST_F(AnalysisErrorTest, ToStringBasic) {
  std::string error_str = type_error.to_string();

  // 에러 문자열에 기본 정보가 포함되어 있는지 확인
  EXPECT_NE(error_str.find("TYPE_MISMATCH"), std::string::npos);
  EXPECT_NE(error_str.find("Type mismatch: expected 'number', got 'string'"),
            std::string::npos);
  EXPECT_NE(error_str.find("42"), std::string::npos);
  EXPECT_NE(error_str.find("15"), std::string::npos);
}

TEST_F(AnalysisErrorTest, ToStringWithDifferentSeverities) {
  type_error.set_severity(AnalysisError::Severity::WARNING);
  std::string warning_str = type_error.to_string();
  EXPECT_NE(warning_str.find("WARNING"), std::string::npos);

  type_error.set_severity(AnalysisError::Severity::FATAL);
  std::string fatal_str = type_error.to_string();
  EXPECT_NE(fatal_str.find("FATAL"), std::string::npos);
}

// =============================================================================
// Error Type String Tests
// =============================================================================

TEST_F(AnalysisErrorTest, GetTypeStringForAllTypes) {
  struct TypeTestCase {
    AnalysisErrorType type;
    std::string expected_name;
  };

  std::vector<TypeTestCase> test_cases = {
      {AnalysisErrorType::TYPE_MISMATCH, "TYPE_MISMATCH"},
      {AnalysisErrorType::UNDEFINED_SYMBOL, "UNDEFINED_SYMBOL"},
      {AnalysisErrorType::REDEFINED_SYMBOL, "REDEFINED_SYMBOL"},
      {AnalysisErrorType::INCOMPATIBLE_ASSIGNMENT, "INCOMPATIBLE_ASSIGNMENT"},
      {AnalysisErrorType::INVALID_OPERATION, "INVALID_OPERATION"},
      {AnalysisErrorType::UNREACHABLE_CODE, "UNREACHABLE_CODE"},
      {AnalysisErrorType::UNUSED_VARIABLE, "UNUSED_VARIABLE"},
      {AnalysisErrorType::UNINITIALIZED_VARIABLE, "UNINITIALIZED_VARIABLE"},
      {AnalysisErrorType::INVALID_RETURN_TYPE, "INVALID_RETURN_TYPE"},
      {AnalysisErrorType::INVALID_FUNCTION_CALL, "INVALID_FUNCTION_CALL"},
      {AnalysisErrorType::SCOPE_ERROR, "SCOPE_ERROR"},
      {AnalysisErrorType::CONTROL_FLOW_ERROR, "CONTROL_FLOW_ERROR"}};

  for (const auto &test_case : test_cases) {
    AnalysisError error(test_case.type, "Test message");
    EXPECT_EQ(error.get_type_string(), test_case.expected_name);
  }
}

// =============================================================================
// Edge Cases and Special Scenarios
// =============================================================================

TEST_F(AnalysisErrorTest, EmptyMessage) {
  AnalysisError empty_msg_error(AnalysisErrorType::SCOPE_ERROR, "");

  EXPECT_EQ(empty_msg_error.get_message(), "");
  std::string error_str = empty_msg_error.to_string();
  EXPECT_FALSE(error_str.empty());
}

TEST_F(AnalysisErrorTest, VeryLongMessage) {
  std::string long_message(1000, 'A');
  AnalysisError long_error(AnalysisErrorType::TYPE_MISMATCH, long_message, 1,
                           1);

  EXPECT_EQ(long_error.get_message(), long_message);
  std::string error_str = long_error.to_string();
  EXPECT_NE(error_str.find(long_message), std::string::npos);
}

TEST_F(AnalysisErrorTest, SpecialCharactersInMessage) {
  std::string special_msg =
      "Error with special chars: \n\t\"quoted\" and 'single' quotes";
  AnalysisError special_error(AnalysisErrorType::INVALID_OPERATION, special_msg,
                              5, 10);

  EXPECT_EQ(special_error.get_message(), special_msg);
  std::string error_str = special_error.to_string();
  EXPECT_NE(error_str.find(special_msg), std::string::npos);
}

TEST_F(AnalysisErrorTest, ZeroLineAndColumn) {
  AnalysisError zero_pos_error(AnalysisErrorType::SCOPE_ERROR,
                               "Zero position error", 0, 0);

  EXPECT_EQ(zero_pos_error.get_line(), 0);
  EXPECT_EQ(zero_pos_error.get_column(), 0);

  std::string error_str = zero_pos_error.to_string();
  EXPECT_FALSE(error_str.empty());
}

TEST_F(AnalysisErrorTest, MaxLineAndColumn) {
  size_t max_val = std::numeric_limits<size_t>::max();
  AnalysisError max_pos_error(AnalysisErrorType::TYPE_MISMATCH,
                              "Max position error", max_val, max_val);

  EXPECT_EQ(max_pos_error.get_line(), max_val);
  EXPECT_EQ(max_pos_error.get_column(), max_val);
}

// =============================================================================
// Comparison and Equality Tests
// =============================================================================

TEST_F(AnalysisErrorTest, ErrorComparison) {
  AnalysisError error1(AnalysisErrorType::TYPE_MISMATCH, "Test message", 10, 5);

  AnalysisError error2(AnalysisErrorType::TYPE_MISMATCH, "Test message", 10, 5);

  AnalysisError error3(AnalysisErrorType::UNDEFINED_SYMBOL, "Different message",
                       10, 5);

  // 같은 내용의 에러들은 동일해야 함 (추가 구현 필요시)
  EXPECT_EQ(error1.get_type(), error2.get_type());
  EXPECT_EQ(error1.get_message(), error2.get_message());
  EXPECT_EQ(error1.get_line(), error2.get_line());
  EXPECT_EQ(error1.get_column(), error2.get_column());

  // 다른 에러는 다르게 처리
  EXPECT_NE(error1.get_type(), error3.get_type());
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(AnalysisErrorTest, PerformanceTestStringGeneration) {
  const size_t num_iterations = 10000;
  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_iterations; ++i) {
    AnalysisError perf_error(AnalysisErrorType::TYPE_MISMATCH,
                             "Performance test error " + std::to_string(i), i,
                             i % 100);

    std::string error_str = perf_error.to_string();
    EXPECT_FALSE(error_str.empty());
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 성능 테스트: 10000개 에러 생성 및 문자열 변환이 1초 미만이어야 함
  EXPECT_LT(duration.count(), 1000)
      << "Error string generation too slow: " << duration.count() << "ms";
}

// =============================================================================
// Thread Safety Tests (기본적인 테스트)
// =============================================================================

TEST_F(AnalysisErrorTest, BasicThreadSafety) {
  // 여러 스레드에서 동시에 에러 객체를 읽는 테스트
  AnalysisError shared_error(AnalysisErrorType::TYPE_MISMATCH,
                             "Thread safety test", 100, 50);

  std::vector<std::thread> threads;
  std::atomic<bool> test_passed{true};

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&shared_error, &test_passed]() {
      for (int j = 0; j < 100; ++j) {
        try {
          auto type = shared_error.get_type();
          auto message = shared_error.get_message();
          auto line = shared_error.get_line();
          auto column = shared_error.get_column();
          auto str = shared_error.to_string();

          if (type != AnalysisErrorType::TYPE_MISMATCH ||
              message != "Thread safety test" || line != 100 || column != 50 ||
              str.empty()) {
            test_passed = false;
            break;
          }
        } catch (...) {
          test_passed = false;
          break;
        }
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(test_passed.load()) << "Thread safety test failed";
}

// =============================================================================
// Memory and Resource Tests
// =============================================================================

TEST_F(AnalysisErrorTest, MemoryUsageTest) {
  // 대량의 에러 객체 생성 및 소멸 테스트
  std::vector<std::unique_ptr<AnalysisError>> errors;

  for (size_t i = 0; i < 10000; ++i) {
    errors.push_back(std::make_unique<AnalysisError>(
        static_cast<AnalysisErrorType>(i % 12), // 12개의 에러 타입
        "Memory test error " + std::to_string(i), i, i % 1000));
  }

  // 모든 에러가 올바르게 생성되었는지 확인
  EXPECT_EQ(errors.size(), 10000);

  // 랜덤 샘플링으로 무결성 확인
  for (size_t i = 0; i < 100; ++i) {
    size_t idx = i * 100; // 100간격으로 샘플링
    EXPECT_NE(errors[idx], nullptr);
    EXPECT_EQ(errors[idx]->get_line(), idx);
    EXPECT_EQ(errors[idx]->get_column(), idx % 1000);
  }

  // 명시적으로 메모리 해제
  errors.clear();
}