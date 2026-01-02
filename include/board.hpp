#pragma once

#include "types.hpp"
#include <vector>
#include <string>

namespace gomoku {

class Board {
public:
    Board();
    
    // Core operations
    void make_move(const Move& move);
    void make_move(int x, int y);
    void unmake_move(const Move& move);
    void reset();
    
    // State queries
    int8_t get(int x, int y) const;
    int8_t get(int idx) const;
    bool is_empty(int x, int y) const;
    bool is_legal(int x, int y) const;
    bool is_legal(const Move& move) const;
    
    // Legal moves
    std::vector<Move> get_legal_moves() const;
    int count_legal_moves() const;
    
    // Game state
    bool is_terminal() const { return is_terminal_; }
    GameResult get_result() const { return result_; }
    int8_t get_winner() const;
    int8_t current_player() const { return current_player_; }
    
    // Move history
    const std::vector<Move>& get_history() const { return history_; }
    int move_count() const { return static_cast<int>(history_.size()); }
    
    // Debug
    std::string to_string() const;
    
private:
    // Board storage - flat array for cache locality
    std::array<int8_t, BOARD_CELLS> cells_;
    
    // Bitboards for fast operations
    BitBoard occupied_mask_;
    BitBoard black_mask_;
    BitBoard white_mask_;
    BitBoard legal_mask_;
    
    // Game state
    int8_t current_player_;
    bool is_terminal_;
    GameResult result_;
    
    // Move history for unmake
    std::vector<Move> history_;
    
    // Internal methods
    void update_legal_mask(const Move& move);
    bool check_win(const Move& move) const;
    int count_direction(int x, int y, int dx, int dy, int8_t player) const;
};

} // namespace gomoku
