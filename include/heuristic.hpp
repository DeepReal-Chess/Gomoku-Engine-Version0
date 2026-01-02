#pragma once

#include "board.hpp"
#include <array>

namespace gomoku {

// Pattern scores for move evaluation
constexpr int SCORE_WIN = 1000000;
constexpr int SCORE_FOUR_OPEN = 100000;
constexpr int SCORE_FOUR_CLOSED = 10000;
constexpr int SCORE_THREE_OPEN = 5000;
constexpr int SCORE_THREE_CLOSED = 500;
constexpr int SCORE_TWO_OPEN = 200;
constexpr int SCORE_TWO_CLOSED = 20;
constexpr int SCORE_SPACE = 10;      // Per empty square around move
constexpr int SCORE_CLUSTER = 10;    // Per nearby stone

// Move with score for sorting
struct ScoredMove {
    Move move;
    int score;
    bool is_winning;
    bool is_blocking;
    
    ScoredMove() : score(0), is_winning(false), is_blocking(false) {}
    ScoredMove(const Move& m, int s) : move(m), score(s), is_winning(false), is_blocking(false) {}
    
    bool operator>(const ScoredMove& other) const {
        // Priority: winning > blocking > score
        if (is_winning != other.is_winning) return is_winning;
        if (is_blocking != other.is_blocking) return is_blocking;
        return score > other.score;
    }
};

class Heuristic {
public:
    Heuristic();
    
    // Evaluate a single move
    int evaluate_move(const Board& board, const Move& move) const;
    
    // Get scored move with full analysis
    ScoredMove score_move(const Board& board, const Move& move) const;
    
    // Get all moves sorted by score
    std::vector<ScoredMove> get_scored_moves(const Board& board) const;
    
    // Quick check for immediate wins/threats
    Move find_winning_move(const Board& board) const;
    Move find_blocking_move(const Board& board) const;
    
private:
    // Pattern evaluation
    int evaluate_line(const Board& board, int x, int y, int dx, int dy, int8_t player) const;
    int count_consecutive(const Board& board, int x, int y, int dx, int dy, int8_t player) const;
    int count_space(const Board& board, int x, int y, int dx, int dy) const;
    
    // Clustering bonus
    int cluster_bonus(const Board& board, const Move& move) const;
    
    // Precomputed pattern scores (indexed by pattern hash)
    std::array<int, 6561> pattern_table_; // 3^8 patterns
    
    void init_pattern_table();
};

} // namespace gomoku
