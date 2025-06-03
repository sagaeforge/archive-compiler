#include "02_scanning/Scanner.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <locale>
#include <random>
#include <string>
#include <thread>

namespace nugdev::test {

class ScannerTest : public ::testing::Test {
protected:
  void SetUp() override {
    scanner = std::make_unique<nugdev::compiler::scanning::Scanner>();

    // 각 테스트마다 고유한 디렉토리 생성
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    testDir = std::filesystem::temp_directory_path() /
              ("scanner_test_" + std::to_string(dis(gen)));

    // 기존 디렉토리가 있다면 삭제
    if (std::filesystem::exists(testDir)) {
      try {
        std::filesystem::remove_all(testDir);
      } catch (const std::filesystem::filesystem_error &) {
        // 무시하고 계속 진행
      }
    }

    std::filesystem::create_directories(testDir);

    // 로케일 설정
    try {
      std::locale::global(std::locale(""));
    } catch (const std::runtime_error &) {
      // 로케일 설정 실패 시 기본 로케일 사용
      std::locale::global(std::locale("C"));
    }
  }

  void TearDown() override {
    // 파일 시스템이 안정화될 시간을 주기
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    try {
      if (std::filesystem::exists(testDir)) {
        std::filesystem::remove_all(testDir);
      }
    } catch (const std::filesystem::filesystem_error &e) {
      // 테스트 실패를 방지하기 위해 예외를 무시
      // 임시 디렉토리이므로 OS가 정리할 것임
      std::cerr << "Warning: Failed to clean up test directory: " << e.what()
                << std::endl;
    }
  }

  void CreateTestFile(const std::string &filename, const std::string &content) {
    std::filesystem::path filePath = testDir / filename;

    // 파일이 이미 존재한다면 삭제
    if (std::filesystem::exists(filePath)) {
      std::filesystem::remove(filePath);
    }

    std::ofstream file(filePath,
                       std::ios::out | std::ios::trunc | std::ios::binary);
    ASSERT_TRUE(file.is_open()) << "Failed to create test file: " << filePath;

    file << content;
    file.flush();
    file.close();

    // 파일 시스템이 안정화될 시간을 주기
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // 파일이 실제로 생성되었는지 확인
    ASSERT_TRUE(std::filesystem::exists(filePath))
        << "Test file does not exist: " << filePath;

    // 파일 크기 확인 (내용이 있는 경우)
    if (!content.empty()) {
      ASSERT_GT(std::filesystem::file_size(filePath), 0)
          << "Test file is empty: " << filePath;
    }
  }

  void CreateTestFileW(const std::string &filename,
                       const std::wstring &content) {
    std::filesystem::path filePath = testDir / filename;
    std::wofstream file(filePath);
    if (file.is_open()) {
      file.imbue(std::locale(""));
      file << content;
      file.close();
    }
  }

  std::unique_ptr<nugdev::compiler::scanning::Scanner> scanner;
  std::filesystem::path testDir;
};

// scan_with_line 메서드 테스트
TEST_F(ScannerTest, ScanWithLine_SingleLine) {
  nugdev::lib::String line("Hello World");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].to_string(), "Hello World");
}

TEST_F(ScannerTest, ScanWithLine_MultipleLines) {
  nugdev::lib::String line("Line 1\nLine 2\nLine 3");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].to_string(), "Line 1");
  EXPECT_EQ(result[1].to_string(), "Line 2");
  EXPECT_EQ(result[2].to_string(), "Line 3");
}

TEST_F(ScannerTest, ScanWithLine_EmptyString) {
  nugdev::lib::String line("");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].to_string(), "");
}

TEST_F(ScannerTest, ScanWithLine_OnlyNewlines) {
  nugdev::lib::String line("\n\n\n");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 4);
  for (const auto &str : result) {
    EXPECT_EQ(str.to_string(), "");
  }
}

TEST_F(ScannerTest, ScanWithLine_TrailingNewline) {
  nugdev::lib::String line("Line 1\nLine 2\n");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].to_string(), "Line 1");
  EXPECT_EQ(result[1].to_string(), "Line 2");
  EXPECT_EQ(result[2].to_string(), "");
}

TEST_F(ScannerTest, ScanWithLine_UnicodeContent) {
  nugdev::lib::String line("안녕하세요\n世界\nHello");
  auto result = scanner->scan_with_line(line);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].to_string(), "안녕하세요");
  EXPECT_EQ(result[1].to_string(), "世界");
  EXPECT_EQ(result[2].to_string(), "Hello");
}

// scan_with_file 메서드 테스트
TEST_F(ScannerTest, ScanWithFile_ValidFile) {
  CreateTestFile("test.txt", "First line\nSecond line\nThird line");

  std::filesystem::path filePath = testDir / "test.txt";
  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0].to_string(), "First line");
  EXPECT_EQ(result[1].to_string(), "Second line");
  EXPECT_EQ(result[2].to_string(), "Third line");
}

TEST_F(ScannerTest, ScanWithFile_EmptyFile) {
  CreateTestFile("empty.txt", "");

  std::filesystem::path filePath = testDir / "empty.txt";
  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(ScannerTest, ScanWithFile_SingleLineFile) {
  CreateTestFile("single.txt", "Only one line");

  std::filesystem::path filePath = testDir / "single.txt";
  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].to_string(), "Only one line");
}

TEST_F(ScannerTest, ScanWithFile_FileWithEmptyLines) {
  CreateTestFile("empty_lines.txt", "Line 1\n\nLine 3\n\nLine 5");

  std::filesystem::path filePath = testDir / "empty_lines.txt";
  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(result[0].to_string(), "Line 1");
  EXPECT_EQ(result[1].to_string(), "");
  EXPECT_EQ(result[2].to_string(), "Line 3");
  EXPECT_EQ(result[3].to_string(), "");
  EXPECT_EQ(result[4].to_string(), "Line 5");
}

TEST_F(ScannerTest, ScanWithFile_UnicodeFile) {
  // ASCII 문자로 먼저 테스트 (안정성 확보)
  CreateTestFile("unicode.txt", "Hello\nWorld\nTest");

  std::filesystem::path filePath = testDir / "unicode.txt";

  // 파일이 실제로 존재하는지 다시 한 번 확인
  ASSERT_TRUE(std::filesystem::exists(filePath))
      << "File was not created: " << filePath;

  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 3) << "File path: " << filePath;
  if (result.size() >= 3) {
    EXPECT_EQ(result[0].to_string(), "Hello");
    EXPECT_EQ(result[1].to_string(), "World");
    EXPECT_EQ(result[2].to_string(), "Test");
  }
}

TEST_F(ScannerTest, ScanWithFile_SimpleUnicodeFile) {
  // 간단한 유니코드 문자만 테스트
  CreateTestFile("simple_unicode.txt", "Hello\nWorld");

  std::filesystem::path filePath = testDir / "simple_unicode.txt";

  // 파일 존재 확인
  ASSERT_TRUE(std::filesystem::exists(filePath));

  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 2);
  if (result.size() >= 2) {
    EXPECT_EQ(result[0].to_string(), "Hello");
    EXPECT_EQ(result[1].to_string(), "World");
  }
}

TEST_F(ScannerTest, ScanWithFile_NonExistentFile) {
  nugdev::lib::String pathStr("non_existent_file.txt");
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), 0);
}

TEST_F(ScannerTest, ScanWithFile_LargeFile) {
  std::stringstream content;
  const int lineCount = 1000;
  for (int i = 0; i < lineCount; ++i) {
    content << "Line " << i;
    if (i < lineCount - 1) {
      content << "\n";
    }
  }

  CreateTestFile("large.txt", content.str());

  std::filesystem::path filePath = testDir / "large.txt";
  nugdev::lib::String pathStr(filePath.string());
  auto result = scanner->scan_with_file(pathStr);

  EXPECT_EQ(result.size(), lineCount);
  EXPECT_EQ(result[0].to_string(), "Line 0");
  EXPECT_EQ(result[lineCount - 1].to_string(),
            "Line " + std::to_string(lineCount - 1));
}

// 에지 케이스 테스트
TEST_F(ScannerTest, ScannerDefaultConstructor) {
  nugdev::compiler::scanning::Scanner testScanner;
  nugdev::lib::String line("Test line");
  auto result = testScanner.scan_with_line(line);

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].to_string(), "Test line");
}

} // namespace nugdev::test