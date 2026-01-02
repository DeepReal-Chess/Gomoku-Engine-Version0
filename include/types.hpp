#pragma once

#include <cstdint>
#include <array>
#include <bitset>

namespace gomoku {

// Board constants
constexpr int BOARD_SIZE = 15;
constexpr int BOARD_CELLS = BOARD_SIZE * BOARD_SIZE; // 225

// Player representation
constexpr int8_t EMPTY = 0;
constexpr int8_t BLACK = 1;
constexpr int8_t WHITE = -1;

// Chebyshev radius for legal moves
constexpr int LEGAL_RADIUS = 2;

// Direction vectors for win detection (dx, dy pairs)
constexpr std::array<std::pair<int, int>, 4> DIRECTIONS = {{
    {1, 0},   // Horizontal
    {0, 1},   // Vertical
    {1, 1},   // Diagonal
    {1, -1}   // Anti-diagonal
}};

// Utility functions
inline constexpr int to_index(int x, int y) {
    return y * BOARD_SIZE + x;
}

inline constexpr int to_x(int idx) {
    return idx % BOARD_SIZE;
}

inline constexpr int to_y(int idx) {
    return idx / BOARD_SIZE;
}

inline constexpr bool in_bounds(int x, int y) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

// Move structure
struct Move {
    int8_t x;
    int8_t y;
    
    Move() : x(-1), y(-1) {}
    Move(int x_, int y_) : x(static_cast<int8_t>(x_)), y(static_cast<int8_t>(y_)) {}
    
    bool is_valid() const { return x >= 0 && y >= 0; }
    int to_index() const { return gomoku::to_index(x, y); }
    
    bool operator==(const Move& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Move& other) const {
        return !(*this == other);
    }
};

// Bitset for 225 cells
using BitBoard = std::bitset<BOARD_CELLS>;

// Game result
enum class GameResult {
    ONGOING,
    BLACK_WIN,
    WHITE_WIN,
    DRAW
};

} // namespace gomoku
