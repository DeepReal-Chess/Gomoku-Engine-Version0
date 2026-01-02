#include "board.hpp"
#include <sstream>
#include <algorithm>

namespace gomoku {

Board::Board() {
    reset();
}

void Board::reset() {
    cells_.fill(EMPTY);
    occupied_mask_.reset();
    black_mask_.reset();
    white_mask_.reset();
    legal_mask_.reset();
    current_player_ = BLACK;
    is_terminal_ = false;
    result_ = GameResult::ONGOING;
    history_.clear();
}

void Board::make_move(int x, int y) {
    make_move(Move(x, y));
}

void Board::make_move(const Move& move) {
    int idx = move.to_index();
    
    // Place stone
    cells_[idx] = current_player_;
    occupied_mask_.set(idx);
    
    if (current_player_ == BLACK) {
        black_mask_.set(idx);
    } else {
        white_mask_.set(idx);
    }
    
    // Update legal mask
    update_legal_mask(move);
    
    // Remove this square from legal moves
    legal_mask_.reset(idx);
    
    // Record history
    history_.push_back(move);
    
    // Check for win
    if (check_win(move)) {
        is_terminal_ = true;
        result_ = (current_player_ == BLACK) ? GameResult::BLACK_WIN : GameResult::WHITE_WIN;
    } else if (legal_mask_.none()) {
        // Check for draw (no legal moves)
        is_terminal_ = true;
        result_ = GameResult::DRAW;
    }
    
    // Switch player
    current_player_ = -current_player_;
}

void Board::unmake_move(const Move& move) {
    if (history_.empty()) return;
    
    // Switch player back
    current_player_ = -current_player_;
    
    int idx = move.to_index();
    
    // Remove stone
    cells_[idx] = EMPTY;
    occupied_mask_.reset(idx);
    black_mask_.reset(idx);
    white_mask_.reset(idx);
    
    // Restore game state
    is_terminal_ = false;
    result_ = GameResult::ONGOING;
    
    // Remove from history
    history_.pop_back();
    
    // Rebuild legal mask (simpler than tracking changes)
    legal_mask_.reset();
    if (history_.empty()) {
        // If board is empty after unmake, no legal moves yet
        // First move can be anywhere - for simplicity, center area
        legal_mask_.set(to_index(7, 7));
    } else {
        // Rebuild from all existing stones
        for (const auto& m : history_) {
            int mx = m.x;
            int my = m.y;
            for (int dy = -LEGAL_RADIUS; dy <= LEGAL_RADIUS; ++dy) {
                for (int dx = -LEGAL_RADIUS; dx <= LEGAL_RADIUS; ++dx) {
                    int nx = mx + dx;
                    int ny = my + dy;
                    if (in_bounds(nx, ny) && !occupied_mask_[to_index(nx, ny)]) {
                        legal_mask_.set(to_index(nx, ny));
                    }
                }
            }
        }
    }
}

void Board::update_legal_mask(const Move& move) {
    // If first move, initialize legal mask with center
    if (history_.empty() && legal_mask_.none()) {
        legal_mask_.set(move.to_index());
    }
    
    // Add all empty cells within Chebyshev radius 2
    int mx = move.x;
    int my = move.y;
    for (int dy = -LEGAL_RADIUS; dy <= LEGAL_RADIUS; ++dy) {
        for (int dx = -LEGAL_RADIUS; dx <= LEGAL_RADIUS; ++dx) {
            int nx = mx + dx;
            int ny = my + dy;
            if (in_bounds(nx, ny) && !occupied_mask_[to_index(nx, ny)]) {
                legal_mask_.set(to_index(nx, ny));
            }
        }
    }
}

int8_t Board::get(int x, int y) const {
    return cells_[to_index(x, y)];
}

int8_t Board::get(int idx) const {
    return cells_[idx];
}

bool Board::is_empty(int x, int y) const {
    return cells_[to_index(x, y)] == EMPTY;
}

bool Board::is_legal(int x, int y) const {
    if (!in_bounds(x, y)) return false;
    int idx = to_index(x, y);
    
    // First move special case - allow center
    if (history_.empty()) {
        return is_empty(x, y);
    }
    
    return legal_mask_[idx] && !occupied_mask_[idx];
}

bool Board::is_legal(const Move& move) const {
    return is_legal(move.x, move.y);
}

std::vector<Move> Board::get_legal_moves() const {
    std::vector<Move> moves;
    
    // First move - just return center
    if (history_.empty()) {
        moves.emplace_back(7, 7);
        return moves;
    }
    
    moves.reserve(legal_mask_.count());
    for (int idx = 0; idx < BOARD_CELLS; ++idx) {
        if (legal_mask_[idx]) {
            moves.emplace_back(to_x(idx), to_y(idx));
        }
    }
    return moves;
}

int Board::count_legal_moves() const {
    if (history_.empty()) return 1;
    return static_cast<int>(legal_mask_.count());
}

int8_t Board::get_winner() const {
    switch (result_) {
        case GameResult::BLACK_WIN: return BLACK;
        case GameResult::WHITE_WIN: return WHITE;
        default: return EMPTY;
    }
}

bool Board::check_win(const Move& move) const {
    int x = move.x;
    int y = move.y;
    int8_t player = cells_[move.to_index()];
    
    for (const auto& [dx, dy] : DIRECTIONS) {
        int count = 1; // Include the placed stone
        count += count_direction(x, y, dx, dy, player);
        count += count_direction(x, y, -dx, -dy, player);
        
        if (count >= 5) {
            return true;
        }
    }
    return false;
}

int Board::count_direction(int x, int y, int dx, int dy, int8_t player) const {
    int count = 0;
    int nx = x + dx;
    int ny = y + dy;
    
    while (in_bounds(nx, ny) && cells_[to_index(nx, ny)] == player) {
        ++count;
        nx += dx;
        ny += dy;
    }
    return count;
}

std::string Board::to_string() const {
    std::ostringstream oss;
    
    // Column headers
    oss << "   ";
    for (int x = 0; x < BOARD_SIZE; ++x) {
        oss << static_cast<char>('A' + x) << ' ';
    }
    oss << "\n";
    
    for (int y = 0; y < BOARD_SIZE; ++y) {
        // Row number
        oss << (y < 9 ? " " : "") << (y + 1) << " ";
        
        for (int x = 0; x < BOARD_SIZE; ++x) {
            int8_t cell = cells_[to_index(x, y)];
            if (cell == BLACK) {
                oss << "X ";
            } else if (cell == WHITE) {
                oss << "O ";
            } else {
                oss << ". ";
            }
        }
        oss << "\n";
    }
    return oss.str();
}

} // namespace gomoku
