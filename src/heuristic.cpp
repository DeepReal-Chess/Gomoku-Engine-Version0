#include "heuristic.hpp"
#include <algorithm>

namespace gomoku {

Heuristic::Heuristic() {
    init_pattern_table();
}

void Heuristic::init_pattern_table() {
    pattern_table_.fill(0);
}

int Heuristic::count_consecutive(const Board& board, int x, int y, int dx, int dy, int8_t player) const {
    int count = 0;
    int nx = x + dx;
    int ny = y + dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
        if (board.get(nx, ny) != player) break;
        count++;
        nx += dx;
        ny += dy;
    }
    return count;
}

int Heuristic::count_space(const Board& board, int x, int y, int dx, int dy) const {
    int space = 0;
    int nx = x + dx;
    int ny = y + dy;
    while (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE && space < 5) {
        if (board.get(nx, ny) != EMPTY) break;
        space++;
        nx += dx;
        ny += dy;
    }
    return space;
}

// Evaluate a line from position (x,y) in direction (dx,dy) for player
// Returns score based on pattern found
int Heuristic::evaluate_line(const Board& board, int x, int y, int dx, int dy, int8_t player) const {
    int8_t opponent = (player == BLACK) ? WHITE : BLACK;
    
    // Count consecutive stones in both directions
    int count_pos = count_consecutive(board, x, y, dx, dy, player);
    int count_neg = count_consecutive(board, x, y, -dx, -dy, player);
    int total = count_pos + count_neg;
    
    if (total >= 4) return SCORE_WIN;
    
    // Check openness (space at both ends)
    // Positive direction end
    int end_pos_x = x + dx * (count_pos + 1);
    int end_pos_y = y + dy * (count_pos + 1);
    bool open_pos = (end_pos_x >= 0 && end_pos_x < BOARD_SIZE && 
                     end_pos_y >= 0 && end_pos_y < BOARD_SIZE &&
                     board.get(end_pos_x, end_pos_y) == EMPTY);
    
    // Negative direction end
    int end_neg_x = x - dx * (count_neg + 1);
    int end_neg_y = y - dy * (count_neg + 1);
    bool open_neg = (end_neg_x >= 0 && end_neg_x < BOARD_SIZE && 
                     end_neg_y >= 0 && end_neg_y < BOARD_SIZE &&
                     board.get(end_neg_x, end_neg_y) == EMPTY);
    
    int openness = (open_pos ? 1 : 0) + (open_neg ? 1 : 0);
    
    // Also check for "gapped" patterns like X_XX or XX_X
    // Check if there's a stone with a gap in positive direction
    int gap_count = 0;
    if (open_pos && count_pos < 4) {
        // Check beyond the empty space
        int gap_x = end_pos_x + dx;
        int gap_y = end_pos_y + dy;
        while (gap_x >= 0 && gap_x < BOARD_SIZE && gap_y >= 0 && gap_y < BOARD_SIZE) {
            if (board.get(gap_x, gap_y) == player) {
                gap_count++;
                gap_x += dx;
                gap_y += dy;
            } else {
                break;
            }
        }
    }
    
    // Check gap in negative direction
    int gap_count_neg = 0;
    if (open_neg && count_neg < 4) {
        int gap_x = end_neg_x - dx;
        int gap_y = end_neg_y - dy;
        while (gap_x >= 0 && gap_x < BOARD_SIZE && gap_y >= 0 && gap_y < BOARD_SIZE) {
            if (board.get(gap_x, gap_y) == player) {
                gap_count_neg++;
                gap_x -= dx;
                gap_y -= dy;
            } else {
                break;
            }
        }
    }
    
    int total_with_gaps = total + gap_count + gap_count_neg;
    
    // Scoring based on pattern
    if (total == 3) {
        if (openness == 2) return SCORE_FOUR_OPEN; // Open four potential (critical!)
        if (openness == 1) return SCORE_FOUR_CLOSED;
    }
    
    if (total == 2) {
        // Check for gapped three (X_XX pattern after this move)
        if (gap_count >= 1 || gap_count_neg >= 1) {
            if (openness >= 1) return SCORE_THREE_OPEN; // Broken three is still a threat
        }
        if (openness == 2) return SCORE_THREE_OPEN;
        if (openness == 1) return SCORE_THREE_CLOSED;
    }
    
    if (total == 1) {
        // Two with gap potential
        if (gap_count >= 2 || gap_count_neg >= 2) {
            return SCORE_THREE_CLOSED; // XX_X or X_XX pattern
        }
        if (gap_count >= 1 || gap_count_neg >= 1) {
            if (openness >= 1) return SCORE_TWO_OPEN;
        }
        if (openness == 2) return SCORE_TWO_OPEN;
        if (openness == 1) return SCORE_TWO_CLOSED;
    }
    
    if (total == 0 && openness >= 1) {
        return 0;  // Space bonus handled separately in cluster_bonus
    }
    
    return 0;
}

int Heuristic::cluster_bonus(const Board& board, const Move& move) const {
    int bonus = 0;
    int empty_count = 0;
    
    // Count nearby stones (any color) for clustering and empty squares for space
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            if (dx == 0 && dy == 0) continue;
            int nx = move.x + dx;
            int ny = move.y + dy;
            if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
                if (board.get(nx, ny) != EMPTY) {
                    // Closer stones give more bonus
                    int dist = std::max(std::abs(dx), std::abs(dy));
                    bonus += SCORE_CLUSTER * (3 - dist);
                } else {
                    // Count empty squares for space bonus
                    empty_count++;
                }
            }
        }
    }
    // Add space bonus (10 per empty square)
    bonus += empty_count * SCORE_SPACE;
    return bonus;
}

int Heuristic::evaluate_move(const Board& board, const Move& move) const {
    int8_t player = board.current_player();
    int8_t opponent = (player == BLACK) ? WHITE : BLACK;
    
    int offensive = 0;
    int defensive = 0;
    
    // Directions: horizontal, vertical, diagonal, anti-diagonal
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int d = 0; d < 4; d++) {
        // Offensive: how good is this move for us
        offensive += evaluate_line(board, move.x, move.y, dx[d], dy[d], player);
        
        // Defensive: how good would this move be for opponent
        defensive += evaluate_line(board, move.x, move.y, dx[d], dy[d], opponent);
    }
    
    // Defensive bonus slightly higher to encourage blocking
    return offensive + static_cast<int>(defensive * 1.1) + cluster_bonus(board, move);
}

ScoredMove Heuristic::score_move(const Board& board, const Move& move) const {
    ScoredMove sm(move, 0);
    
    int8_t player = board.current_player();
    int8_t opponent = (player == BLACK) ? WHITE : BLACK;
    
    int offensive = 0;
    int defensive = 0;
    
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int d = 0; d < 4; d++) {
        int off_score = evaluate_line(board, move.x, move.y, dx[d], dy[d], player);
        int def_score = evaluate_line(board, move.x, move.y, dx[d], dy[d], opponent);
        
        if (off_score >= SCORE_WIN) {
            sm.is_winning = true;
        }
        if (def_score >= SCORE_FOUR_OPEN) {
            sm.is_blocking = true;
        }
        
        offensive += off_score;
        defensive += def_score;
    }
    
    sm.score = offensive + static_cast<int>(defensive * 1.1) + cluster_bonus(board, move);
    return sm;
}

std::vector<ScoredMove> Heuristic::get_scored_moves(const Board& board) const {
    auto moves = board.get_legal_moves();
    std::vector<ScoredMove> scored;
    scored.reserve(moves.size());
    
    for (const auto& move : moves) {
        scored.push_back(score_move(board, move));
    }
    
    std::sort(scored.begin(), scored.end(), std::greater<ScoredMove>());
    return scored;
}

Move Heuristic::find_winning_move(const Board& board) const {
    int8_t player = board.current_player();
    auto moves = board.get_legal_moves();
    
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (const auto& move : moves) {
        for (int d = 0; d < 4; d++) {
            int count_pos = count_consecutive(board, move.x, move.y, dx[d], dy[d], player);
            int count_neg = count_consecutive(board, move.x, move.y, -dx[d], -dy[d], player);
            if (count_pos + count_neg >= 4) {
                return move;
            }
        }
    }
    
    return Move(-1, -1);
}

Move Heuristic::find_blocking_move(const Board& board) const {
    int8_t player = board.current_player();
    int8_t opponent = (player == BLACK) ? WHITE : BLACK;
    auto moves = board.get_legal_moves();
    
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    // First check for opponent winning threats (4 in a row)
    for (const auto& move : moves) {
        for (int d = 0; d < 4; d++) {
            int count_pos = count_consecutive(board, move.x, move.y, dx[d], dy[d], opponent);
            int count_neg = count_consecutive(board, move.x, move.y, -dx[d], -dy[d], opponent);
            if (count_pos + count_neg >= 4) {
                return move;
            }
        }
    }
    
    // Then check for open threes that must be blocked
    Move best_block(-1, -1);
    int best_threat = 0;
    
    for (const auto& move : moves) {
        for (int d = 0; d < 4; d++) {
            int threat = evaluate_line(board, move.x, move.y, dx[d], dy[d], opponent);
            if (threat >= SCORE_FOUR_OPEN && threat > best_threat) {
                best_threat = threat;
                best_block = move;
            }
        }
    }
    
    return best_block;
}

} // namespace gomoku
