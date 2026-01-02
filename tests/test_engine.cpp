#include "board.hpp"
#include "heuristic.hpp"
#include "mcts.hpp"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace gomoku;

// Test utilities
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    test_##name(); \
    std::cout << "PASSED" << std::endl; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        std::cerr << "\nAssertion failed: " << #cond << " at line " << __LINE__ << std::endl; \
        assert(false); \
    } \
} while(0)

// ============================================================================
// Board Logic Tests
// ============================================================================

TEST(legal_radius) {
    Board board;
    
    // Place stone at center (7,7)
    board.make_move(7, 7);
    
    // (5,5) should be legal (Chebyshev distance = 2)
    ASSERT(board.is_legal(5, 5));
    
    // (9,9) should be legal (Chebyshev distance = 2)
    ASSERT(board.is_legal(9, 9));
    
    // (10,7) should be illegal (Chebyshev distance = 3)
    ASSERT(!board.is_legal(10, 7));
    
    // (4,7) should be illegal (Chebyshev distance = 3)
    ASSERT(!board.is_legal(4, 7));
    
    // (6,6) should be legal (Chebyshev distance = 1)
    ASSERT(board.is_legal(6, 6));
}

TEST(incremental_win) {
    Board board;
    
    // Place BLACK at (3,7), (4,7), (5,7), (6,7)
    // Interleave with WHITE moves
    board.make_move(3, 7);  // BLACK
    board.make_move(3, 8);  // WHITE
    board.make_move(4, 7);  // BLACK
    board.make_move(4, 8);  // WHITE
    board.make_move(5, 7);  // BLACK
    board.make_move(5, 8);  // WHITE
    board.make_move(6, 7);  // BLACK
    board.make_move(6, 8);  // WHITE
    
    ASSERT(!board.is_terminal());
    
    // Winning move for BLACK
    board.make_move(7, 7);  // BLACK completes 5 in a row
    
    ASSERT(board.is_terminal());
    ASSERT(board.get_winner() == BLACK);
    ASSERT(board.get_result() == GameResult::BLACK_WIN);
}

TEST(vertical_win) {
    Board board;
    
    // Build vertical line for BLACK
    board.make_move(7, 3);  // BLACK
    board.make_move(8, 3);  // WHITE
    board.make_move(7, 4);  // BLACK
    board.make_move(8, 4);  // WHITE
    board.make_move(7, 5);  // BLACK
    board.make_move(8, 5);  // WHITE
    board.make_move(7, 6);  // BLACK
    board.make_move(8, 6);  // WHITE
    board.make_move(7, 7);  // BLACK wins
    
    ASSERT(board.is_terminal());
    ASSERT(board.get_winner() == BLACK);
}

TEST(diagonal_win) {
    Board board;
    
    // Build diagonal line for BLACK
    board.make_move(3, 3);  // BLACK
    board.make_move(3, 4);  // WHITE
    board.make_move(4, 4);  // BLACK
    board.make_move(4, 5);  // WHITE
    board.make_move(5, 5);  // BLACK
    board.make_move(5, 6);  // WHITE
    board.make_move(6, 6);  // BLACK
    board.make_move(6, 7);  // WHITE
    board.make_move(7, 7);  // BLACK wins
    
    ASSERT(board.is_terminal());
    ASSERT(board.get_winner() == BLACK);
}

TEST(anti_diagonal_win) {
    Board board;
    
    // Build anti-diagonal line for BLACK
    board.make_move(7, 3);  // BLACK
    board.make_move(8, 3);  // WHITE
    board.make_move(6, 4);  // BLACK
    board.make_move(8, 4);  // WHITE
    board.make_move(5, 5);  // BLACK
    board.make_move(8, 5);  // WHITE
    board.make_move(4, 6);  // BLACK
    board.make_move(8, 6);  // WHITE
    board.make_move(3, 7);  // BLACK wins
    
    ASSERT(board.is_terminal());
    ASSERT(board.get_winner() == BLACK);
}

TEST(unmake_move) {
    Board board;
    
    board.make_move(7, 7);
    board.make_move(8, 7);
    board.make_move(7, 8);
    
    ASSERT(board.get(7, 7) == BLACK);
    ASSERT(board.get(8, 7) == WHITE);
    ASSERT(board.get(7, 8) == BLACK);
    ASSERT(board.current_player() == WHITE);
    
    board.unmake_move(Move(7, 8));
    
    ASSERT(board.get(7, 8) == EMPTY);
    ASSERT(board.current_player() == BLACK);
    ASSERT(board.move_count() == 2);
}

// ============================================================================
// Heuristic Tests
// ============================================================================

TEST(forced_block) {
    Board board;
    Heuristic heuristic;
    
    // WHITE has four in a row with open end
    board.make_move(7, 7);   // BLACK
    board.make_move(3, 7);   // WHITE
    board.make_move(8, 8);   // BLACK
    board.make_move(4, 7);   // WHITE
    board.make_move(9, 9);   // BLACK
    board.make_move(5, 7);   // WHITE
    board.make_move(10, 10); // BLACK
    board.make_move(6, 7);   // WHITE - now has 4 in a row
    
    // BLACK must block at (2,7) or (7,7) is occupied, so (2,7)
    auto scored_moves = heuristic.get_scored_moves(board);
    
    // The blocking move should be ranked highest
    Move blocking = heuristic.find_blocking_move(board);
    ASSERT(blocking.is_valid());
    ASSERT(blocking.x == 2 || blocking.x == 7);
    ASSERT(blocking.y == 7);
}

TEST(opportunity_preference) {
    Board board;
    Heuristic heuristic;
    
    // Setup: one move is boxed, one has open extensions
    board.make_move(7, 7);  // BLACK center
    board.make_move(6, 7);  // WHITE blocks left
    board.make_move(8, 7);  // BLACK extends right
    
    // Compare (5,7) vs (9,7) - (9,7) should be better as it extends the line
    // and (5,7) is blocked by white
    auto score_9_7 = heuristic.score_move(board, Move(9, 7));
    auto score_5_7 = heuristic.score_move(board, Move(5, 7));
    
    // (9,7) should score higher as it extends BLACK's line
    ASSERT(score_9_7.score > score_5_7.score);
}

TEST(winning_move_detection) {
    Board board;
    Heuristic heuristic;
    
    // Set up BLACK with 4 in a row
    board.make_move(5, 7);  // BLACK
    board.make_move(5, 8);  // WHITE
    board.make_move(6, 7);  // BLACK
    board.make_move(6, 8);  // WHITE
    board.make_move(7, 7);  // BLACK
    board.make_move(7, 8);  // WHITE
    board.make_move(8, 7);  // BLACK
    board.make_move(8, 8);  // WHITE
    
    // BLACK can win at (4,7) or (9,7)
    Move winning = heuristic.find_winning_move(board);
    ASSERT(winning.is_valid());
    ASSERT((winning.x == 4 && winning.y == 7) || (winning.x == 9 && winning.y == 7));
}

// ============================================================================
// MCTS Tests
// ============================================================================

TEST(mcts_winning_in_one) {
    Board board;
    MCTSConfig config;
    config.max_iterations = 100;
    config.max_time_ms = 500;
    config.seed = 42;
    MCTS mcts(config);
    
    // Set up BLACK with 4 in a row
    board.make_move(5, 7);  // BLACK
    board.make_move(5, 8);  // WHITE
    board.make_move(6, 7);  // BLACK
    board.make_move(6, 8);  // WHITE
    board.make_move(7, 7);  // BLACK
    board.make_move(7, 8);  // WHITE
    board.make_move(8, 7);  // BLACK
    board.make_move(8, 8);  // WHITE
    
    // MCTS should find the winning move
    Move best = mcts.search(board);
    
    ASSERT(best.is_valid());
    ASSERT((best.x == 4 && best.y == 7) || (best.x == 9 && best.y == 7));
}

TEST(mcts_defensive_necessity) {
    Board board;
    MCTSConfig config;
    config.max_iterations = 100;
    config.max_time_ms = 500;
    config.seed = 42;
    MCTS mcts(config);
    
    // WHITE threatens to win next move
    board.make_move(7, 7);  // BLACK
    board.make_move(3, 7);  // WHITE
    board.make_move(7, 8);  // BLACK
    board.make_move(4, 7);  // WHITE
    board.make_move(7, 9);  // BLACK
    board.make_move(5, 7);  // WHITE
    board.make_move(10, 10); // BLACK
    board.make_move(6, 7);  // WHITE - 4 in a row!
    
    // BLACK must block at (2,7) or (7,7) but (7,7) is occupied
    Move best = mcts.search(board);
    
    ASSERT(best.is_valid());
    // Must block at x=2 or x=7 (but 7,7 is occupied so x=2)
    ASSERT(best.y == 7);
    ASSERT(best.x == 2 || best.x == 7);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(move_performance) {
    Board board;
    const int iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        board.reset();
        board.make_move(7, 7);
        board.make_move(8, 7);
        board.make_move(7, 8);
        board.unmake_move(Move(7, 8));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double per_op = static_cast<double>(duration) / (iterations * 4);
    
    std::cout << "[" << per_op << " ns/op] ";
    
    // Target: ~0.1 μs = 100 ns
    // Allow some slack for test environment
    ASSERT(per_op < 10000); // 10 μs max
}

TEST(heuristic_performance) {
    Board board;
    Heuristic heuristic;
    
    // Setup a mid-game position
    board.make_move(7, 7);
    board.make_move(8, 7);
    board.make_move(7, 8);
    board.make_move(8, 8);
    board.make_move(6, 6);
    board.make_move(9, 9);
    
    const int iterations = 10000;
    Move test_move(6, 7);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        heuristic.evaluate_move(board, test_move);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double per_op = static_cast<double>(duration) / iterations;
    
    std::cout << "[" << per_op << " ns/op] ";
    
    // Target: ~0.1 μs = 100 ns
    ASSERT(per_op < 100000); // 100 μs max
}

TEST(mcts_iteration_performance) {
    Board board;
    MCTSConfig config;
    config.max_iterations = 1000;
    config.max_time_ms = 10000; // Long timeout
    config.seed = 42;
    MCTS mcts(config);
    
    board.make_move(7, 7);
    board.make_move(8, 7);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    mcts.search(board);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double per_iter = static_cast<double>(duration) / mcts.get_iterations();
    
    std::cout << "[" << per_iter << " μs/iter, " << mcts.get_iterations() << " iters] ";
    
    // Target: < 10 μs per iteration
    ASSERT(per_iter < 1000); // 1 ms max per iteration
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Gomoku Engine Test Suite ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "--- Board Logic Tests ---" << std::endl;
    RUN_TEST(legal_radius);
    RUN_TEST(incremental_win);
    RUN_TEST(vertical_win);
    RUN_TEST(diagonal_win);
    RUN_TEST(anti_diagonal_win);
    RUN_TEST(unmake_move);
    
    std::cout << std::endl;
    std::cout << "--- Heuristic Tests ---" << std::endl;
    RUN_TEST(forced_block);
    RUN_TEST(opportunity_preference);
    RUN_TEST(winning_move_detection);
    
    std::cout << std::endl;
    std::cout << "--- MCTS Tests ---" << std::endl;
    RUN_TEST(mcts_winning_in_one);
    RUN_TEST(mcts_defensive_necessity);
    
    std::cout << std::endl;
    std::cout << "--- Performance Tests ---" << std::endl;
    RUN_TEST(move_performance);
    RUN_TEST(heuristic_performance);
    RUN_TEST(mcts_iteration_performance);
    
    std::cout << std::endl;
    std::cout << "=== All tests passed! ===" << std::endl;
    
    return 0;
}
