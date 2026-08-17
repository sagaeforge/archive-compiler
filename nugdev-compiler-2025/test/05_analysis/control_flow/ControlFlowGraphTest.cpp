#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace nugdev::compiler::analysis;

class ControlFlowGraphTest : public ::testing::Test {
protected:
  void SetUp() override { cfg = std::make_unique<ControlFlowGraph>(); }

  std::unique_ptr<ControlFlowGraph> cfg;
};

// =============================================================================
// BasicBlock Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, BasicBlockCreation) {
  auto block = cfg->create_block();

  EXPECT_NE(block, nullptr);
  EXPECT_GE(block->get_id(), 0);
  EXPECT_TRUE(block->is_entry_block());
  EXPECT_TRUE(block->is_exit_block());
  EXPECT_FALSE(block->is_unreachable());
}

TEST_F(ControlFlowGraphTest, BasicBlockSuccessors) {
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto block3 = cfg->create_block();

  // 후속자 추가
  block1->add_successor(block2);
  block1->add_successor(block3);

  auto successors = block1->get_successors();
  EXPECT_EQ(successors.size(), 2);
  EXPECT_NE(std::find(successors.begin(), successors.end(), block2),
            successors.end());
  EXPECT_NE(std::find(successors.begin(), successors.end(), block3),
            successors.end());

  // 더이상 exit block이 아님
  EXPECT_FALSE(block1->is_exit_block());
}

TEST_F(ControlFlowGraphTest, BasicBlockPredecessors) {
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto block3 = cfg->create_block();

  // 후속자 추가 시 자동으로 선행자도 설정됨
  block1->add_successor(block2);
  block3->add_successor(block2);

  auto predecessors = block2->get_predecessors();
  EXPECT_EQ(predecessors.size(), 2);
  EXPECT_NE(std::find(predecessors.begin(), predecessors.end(), block1),
            predecessors.end());
  EXPECT_NE(std::find(predecessors.begin(), predecessors.end(), block3),
            predecessors.end());

  // 더이상 entry block이 아님
  EXPECT_FALSE(block2->is_entry_block());
}

TEST_F(ControlFlowGraphTest, BasicBlockStatements) {
  auto block = cfg->create_block();

  // 더미 AST 노드들 추가 (실제로는 AST 노드 포인터를 사용)
  // 테스트용으로는 nullptr 사용
  block->add_statement(nullptr);
  block->add_statement(nullptr);

  auto statements = block->get_statements();
  EXPECT_EQ(statements.size(), 2);
}

TEST_F(ControlFlowGraphTest, BasicBlockDominance) {
  auto dominator = cfg->create_block();
  auto dominated = cfg->create_block();

  dominated->set_dominator(dominator);
  dominator->add_dominated_block(dominated);

  EXPECT_EQ(dominated->get_dominator(), dominator);

  auto dominated_blocks = dominator->get_dominated_blocks();
  EXPECT_EQ(dominated_blocks.size(), 1);
  EXPECT_EQ(dominated_blocks[0], dominated);
}

TEST_F(ControlFlowGraphTest, BasicBlockLoopProperties) {
  auto block = cfg->create_block();

  EXPECT_FALSE(block->is_loop_header());
  EXPECT_EQ(block->get_loop_depth(), 0);

  block->mark_as_loop_header();
  block->set_loop_depth(2);

  EXPECT_TRUE(block->is_loop_header());
  EXPECT_EQ(block->get_loop_depth(), 2);
}

TEST_F(ControlFlowGraphTest, BasicBlockUnreachable) {
  auto block = cfg->create_block();

  EXPECT_FALSE(block->is_unreachable());

  block->mark_unreachable();

  EXPECT_TRUE(block->is_unreachable());
}

// =============================================================================
// ControlFlowGraph Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, ControlFlowGraphCreation) {
  EXPECT_EQ(cfg->get_block_count(), 0);
  EXPECT_EQ(cfg->get_entry_block(), nullptr);
  EXPECT_EQ(cfg->get_exit_block(), nullptr);
}

TEST_F(ControlFlowGraphTest, BlockCreationAndManagement) {
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto block3 = cfg->create_block();

  EXPECT_EQ(cfg->get_block_count(), 3);

  // 각 블록은 고유한 ID를 가져야 함
  EXPECT_NE(block1->get_id(), block2->get_id());
  EXPECT_NE(block2->get_id(), block3->get_id());
  EXPECT_NE(block1->get_id(), block3->get_id());
}

TEST_F(ControlFlowGraphTest, EntryAndExitBlocks) {
  auto entry = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  EXPECT_EQ(cfg->get_entry_block(), entry);
  EXPECT_EQ(cfg->get_exit_block(), exit);
}

TEST_F(ControlFlowGraphTest, EdgeManagement) {
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto block3 = cfg->create_block();

  // 엣지 추가
  cfg->add_edge(block1, block2);
  cfg->add_edge(block2, block3);
  cfg->add_edge(block1, block3); // 분기

  // block1의 후속자 확인
  auto successors1 = block1->get_successors();
  EXPECT_EQ(successors1.size(), 2);
  EXPECT_NE(std::find(successors1.begin(), successors1.end(), block2),
            successors1.end());
  EXPECT_NE(std::find(successors1.begin(), successors1.end(), block3),
            successors1.end());

  // block3의 선행자 확인
  auto predecessors3 = block3->get_predecessors();
  EXPECT_EQ(predecessors3.size(), 2);
  EXPECT_NE(std::find(predecessors3.begin(), predecessors3.end(), block1),
            predecessors3.end());
  EXPECT_NE(std::find(predecessors3.begin(), predecessors3.end(), block2),
            predecessors3.end());

  // 엣지 제거
  cfg->remove_edge(block1, block3);

  successors1 = block1->get_successors();
  EXPECT_EQ(successors1.size(), 1);
  EXPECT_EQ(successors1[0], block2);

  predecessors3 = block3->get_predecessors();
  EXPECT_EQ(predecessors3.size(), 1);
  EXPECT_EQ(predecessors3[0], block2);
}

// =============================================================================
// Dominator Analysis Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, DominatorComputation) {
  // 간단한 그래프 생성: entry -> block1 -> exit
  auto entry = cfg->create_block();
  auto block1 = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  cfg->add_edge(entry, block1);
  cfg->add_edge(block1, exit);

  cfg->compute_dominators();

  // 지배자 관계 확인
  EXPECT_EQ(entry->get_dominator(), nullptr); // entry는 지배자가 없음
  EXPECT_EQ(block1->get_dominator(), entry);  // entry가 block1을 지배
  EXPECT_EQ(exit->get_dominator(), block1);   // block1이 exit을 지배
}

TEST_F(ControlFlowGraphTest, ComplexDominatorAnalysis) {
  // 더 복잡한 그래프: 다이아몬드 형태
  //     entry
  //    /     \
    // block1  block2
  //    \     /
  //     exit

  auto entry = cfg->create_block();
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  cfg->add_edge(entry, block1);
  cfg->add_edge(entry, block2);
  cfg->add_edge(block1, exit);
  cfg->add_edge(block2, exit);

  cfg->compute_dominators();

  // 지배자 관계 확인
  EXPECT_EQ(entry->get_dominator(), nullptr);
  EXPECT_EQ(block1->get_dominator(), entry);
  EXPECT_EQ(block2->get_dominator(), entry);
  EXPECT_EQ(exit->get_dominator(),
            entry); // entry가 exit의 지배자 (block1, block2 통해서만 도달 가능)
}

// =============================================================================
// Loop Detection Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, SimpleLoopDetection) {
  // 간단한 루프: entry -> loop_header -> loop_body -> loop_header -> exit
  auto entry = cfg->create_block();
  auto loop_header = cfg->create_block();
  auto loop_body = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  cfg->add_edge(entry, loop_header);
  cfg->add_edge(loop_header, loop_body);
  cfg->add_edge(loop_body, loop_header); // 백엣지
  cfg->add_edge(loop_header, exit);

  cfg->detect_loops();

  // 루프 헤더 확인
  EXPECT_TRUE(loop_header->is_loop_header());
  EXPECT_FALSE(loop_body->is_loop_header());
  EXPECT_FALSE(entry->is_loop_header());
  EXPECT_FALSE(exit->is_loop_header());

  // 루프 깊이 확인
  EXPECT_EQ(loop_header->get_loop_depth(), 1);
  EXPECT_EQ(loop_body->get_loop_depth(), 1);
  EXPECT_EQ(entry->get_loop_depth(), 0);
  EXPECT_EQ(exit->get_loop_depth(), 0);
}

TEST_F(ControlFlowGraphTest, NestedLoopDetection) {
  // 중첩 루프 구조
  auto entry = cfg->create_block();
  auto outer_header = cfg->create_block();
  auto inner_header = cfg->create_block();
  auto inner_body = cfg->create_block();
  auto outer_body = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  // 외부 루프
  cfg->add_edge(entry, outer_header);
  cfg->add_edge(outer_header, inner_header);
  cfg->add_edge(outer_body, outer_header); // 외부 루프 백엣지
  cfg->add_edge(outer_header, exit);

  // 내부 루프
  cfg->add_edge(inner_header, inner_body);
  cfg->add_edge(inner_body, inner_header); // 내부 루프 백엣지
  cfg->add_edge(inner_header, outer_body);

  cfg->detect_loops();

  // 루프 헤더 확인
  EXPECT_TRUE(outer_header->is_loop_header());
  EXPECT_TRUE(inner_header->is_loop_header());

  // 중첩 루프 깊이 확인
  EXPECT_EQ(outer_header->get_loop_depth(), 1);
  EXPECT_EQ(inner_header->get_loop_depth(), 2); // 내부 루프는 깊이 2
  EXPECT_EQ(inner_body->get_loop_depth(), 2);
  EXPECT_EQ(outer_body->get_loop_depth(), 1);
}

// =============================================================================
// Unreachable Code Detection Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, UnreachableCodeDetection) {
  auto entry = cfg->create_block();
  auto reachable = cfg->create_block();
  auto unreachable1 = cfg->create_block();
  auto unreachable2 = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  // 도달 가능한 경로
  cfg->add_edge(entry, reachable);
  cfg->add_edge(reachable, exit);

  // 도달 불가능한 블록들 (엣지가 없음)
  cfg->add_edge(unreachable1, unreachable2);

  cfg->mark_unreachable_blocks();

  // 도달 가능성 확인
  EXPECT_FALSE(entry->is_unreachable());
  EXPECT_FALSE(reachable->is_unreachable());
  EXPECT_FALSE(exit->is_unreachable());
  EXPECT_TRUE(unreachable1->is_unreachable());
  EXPECT_TRUE(unreachable2->is_unreachable());

  // 도달 불가능한 블록 목록 확인
  auto unreachable_blocks = cfg->get_unreachable_blocks();
  EXPECT_EQ(unreachable_blocks.size(), 2);
  EXPECT_NE(std::find(unreachable_blocks.begin(), unreachable_blocks.end(),
                      unreachable1),
            unreachable_blocks.end());
  EXPECT_NE(std::find(unreachable_blocks.begin(), unreachable_blocks.end(),
                      unreachable2),
            unreachable_blocks.end());
}

TEST_F(ControlFlowGraphTest, UnreachableCodeRemoval) {
  auto entry = cfg->create_block();
  auto reachable = cfg->create_block();
  auto unreachable = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  cfg->add_edge(entry, reachable);
  cfg->add_edge(reachable, exit);
  // unreachable은 연결되지 않음

  size_t initial_count = cfg->get_block_count();
  EXPECT_EQ(initial_count, 4);

  cfg->mark_unreachable_blocks();
  cfg->remove_unreachable_blocks();

  EXPECT_EQ(cfg->get_block_count(), 3); // unreachable 블록이 제거됨
  EXPECT_FALSE(cfg->has_unreachable_code());
}

// =============================================================================
// Graph Optimization Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, BlockMerging) {
  // 단일 후속자를 가진 블록들이 병합되는지 테스트
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  auto block3 = cfg->create_block();

  // block1 -> block2 -> block3 (각각 단일 후속자)
  cfg->add_edge(block1, block2);
  cfg->add_edge(block2, block3);

  size_t initial_count = cfg->get_block_count();
  cfg->merge_blocks();

  // 병합 후 블록 수가 줄어들어야 함 (구현에 따라 다름)
  EXPECT_LE(cfg->get_block_count(), initial_count);
}

// =============================================================================
// Graph Representation Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, DotFormatGeneration) {
  auto entry = cfg->create_block();
  auto block1 = cfg->create_block();
  auto exit = cfg->create_block();

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  cfg->add_edge(entry, block1);
  cfg->add_edge(block1, exit);

  std::string dot_output = cfg->to_dot_format();

  // DOT 형식의 기본 구조 확인
  EXPECT_NE(dot_output.find("digraph"), std::string::npos);
  EXPECT_NE(dot_output.find("->"), std::string::npos);
  EXPECT_FALSE(dot_output.empty());
}

TEST_F(ControlFlowGraphTest, StatisticsPrinting) {
  auto block1 = cfg->create_block();
  auto block2 = cfg->create_block();
  cfg->add_edge(block1, block2);

  // 통계 출력이 크래시하지 않는지 확인
  EXPECT_NO_THROW(cfg->print_statistics());
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(ControlFlowGraphTest, PerformanceTestLargeGraph) {
  auto start = std::chrono::high_resolution_clock::now();

  // 1000개 블록의 선형 그래프 생성
  std::vector<BasicBlock *> blocks;
  for (size_t i = 0; i < 1000; ++i) {
    blocks.push_back(cfg->create_block());
    if (i > 0) {
      cfg->add_edge(blocks[i - 1], blocks[i]);
    }
  }

  cfg->set_entry_block(blocks[0]);
  cfg->set_exit_block(blocks[999]);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 1000개 블록 생성이 100ms 미만이어야 함
  EXPECT_LT(duration.count(), 100)
      << "Graph construction too slow: " << duration.count() << "ms";

  EXPECT_EQ(cfg->get_block_count(), 1000);
}

TEST_F(ControlFlowGraphTest, PerformanceTestDominatorAnalysis) {
  // 복잡한 그래프에 대한 지배자 분석 성능 테스트
  std::vector<BasicBlock *> blocks;
  for (size_t i = 0; i < 100; ++i) {
    blocks.push_back(cfg->create_block());
  }

  // 복잡한 연결 구조 생성
  for (size_t i = 0; i < 99; ++i) {
    cfg->add_edge(blocks[i], blocks[i + 1]);
    if (i % 10 == 0 && i > 0) {
      cfg->add_edge(blocks[i], blocks[i / 10]); // 백엣지 추가
    }
  }

  cfg->set_entry_block(blocks[0]);
  cfg->set_exit_block(blocks[99]);

  auto start = std::chrono::high_resolution_clock::now();
  cfg->compute_dominators();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 100개 블록의 지배자 분석이 50ms 미만이어야 함
  EXPECT_LT(duration.count(), 50)
      << "Dominator analysis too slow: " << duration.count() << "ms";
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(ControlFlowGraphTest, EmptyGraph) {
  EXPECT_EQ(cfg->get_block_count(), 0);
  EXPECT_FALSE(cfg->has_unreachable_code());

  // 빈 그래프에 대한 연산들이 크래시하지 않아야 함
  EXPECT_NO_THROW(cfg->compute_dominators());
  EXPECT_NO_THROW(cfg->detect_loops());
  EXPECT_NO_THROW(cfg->mark_unreachable_blocks());
}

TEST_F(ControlFlowGraphTest, SingleBlockGraph) {
  auto single_block = cfg->create_block();
  cfg->set_entry_block(single_block);
  cfg->set_exit_block(single_block);

  EXPECT_EQ(cfg->get_block_count(), 1);
  EXPECT_TRUE(single_block->is_entry_block());
  EXPECT_TRUE(single_block->is_exit_block());

  cfg->compute_dominators();
  cfg->detect_loops();
  cfg->mark_unreachable_blocks();

  EXPECT_EQ(single_block->get_dominator(), nullptr);
  EXPECT_FALSE(single_block->is_loop_header());
  EXPECT_FALSE(single_block->is_unreachable());
}

TEST_F(ControlFlowGraphTest, SelfLoopDetection) {
  auto self_loop_block = cfg->create_block();
  cfg->add_edge(self_loop_block, self_loop_block);

  cfg->detect_loops();

  EXPECT_TRUE(self_loop_block->is_loop_header());
  EXPECT_EQ(self_loop_block->get_loop_depth(), 1);
}