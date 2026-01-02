#include "mcts.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace gomoku {

MCTS::MCTS(const MCTSConfig& config) : config_(config), iterations_(0) {
    if (config_.seed == 0) {
        rng_.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    } else {
        rng_.seed(config_.seed);
    }
}

Move MCTS::search(const Board& board) {
    return search(board, config_.max_time_ms);
}

Move MCTS::search(const Board& board, int time_limit_ms) {
    // Check for immediate winning move
    Move winning = heuristic_.find_winning_move(board);
    if (winning.is_valid()) {
        return winning;
    }
    
    // Check for forced blocking move
    Move blocking = heuristic_.find_blocking_move(board);
    if (blocking.is_valid()) {
        return blocking;
    }
    
    // Create root node
    auto root = std::make_unique<MCTSNode>(Move(), nullptr, board.current_player());
    init_untried_moves(root.get(), board);
    
    // If only one legal move, return it
    if (root->untried_moves.size() == 1) {
        return root->untried_moves[0];
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    iterations_ = 0;
    
    while (iterations_ < config_.max_iterations) {
        // Check time limit
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        if (elapsed >= time_limit_ms) break;
        
        // Create a copy of the board for this iteration
        Board sim_board = board;
        
        // Selection
        MCTSNode* node = select(root.get(), sim_board);
        
        // Expansion
        if (!node->untried_moves.empty() && !sim_board.is_terminal()) {
            node = expand(node, sim_board);
        }
        
        // Rollout
        double value = rollout(sim_board);
        
        // Backpropagation
        backpropagate(node, value, board.current_player());
        
        ++iterations_;
    }
    
    return select_best_move(root.get());
}

int MCTS::get_root_visits() const {
    return iterations_;
}

MCTSNode* MCTS::select(MCTSNode* node, Board& board) {
    while (!node->is_leaf() && node->is_fully_expanded()) {
        // Select child with highest UCT value
        MCTSNode* best_child = nullptr;
        double best_uct = -std::numeric_limits<double>::infinity();
        
        for (const auto& child : node->children) {
            double uct = uct_value(child.get(), node->visit_count);
            if (uct > best_uct) {
                best_uct = uct;
                best_child = child.get();
            }
        }
        
        if (best_child == nullptr) break;
        
        node = best_child;
        board.make_move(node->move);
    }
    
    return node;
}

MCTSNode* MCTS::expand(MCTSNode* node, Board& board) {
    if (node->untried_moves.empty()) return node;
    
    // Use heuristic to pick a promising move
    Move move;
    if (node->untried_moves.size() > 3) {
        // Score a few random moves and pick the best
        std::vector<ScoredMove> candidates;
        int sample_size = std::min(5, static_cast<int>(node->untried_moves.size()));
        
        // Shuffle and take sample
        std::shuffle(node->untried_moves.begin(), node->untried_moves.end(), rng_);
        for (int i = 0; i < sample_size; ++i) {
            candidates.push_back(heuristic_.score_move(board, node->untried_moves[i]));
        }
        
        // Pick highest scored
        auto best = std::max_element(candidates.begin(), candidates.end(), 
            [](const ScoredMove& a, const ScoredMove& b) { return a.score < b.score; });
        
        // Find and remove from untried
        auto it = std::find(node->untried_moves.begin(), node->untried_moves.end(), best->move);
        move = *it;
        node->untried_moves.erase(it);
    } else {
        // Just pick randomly from remaining
        std::uniform_int_distribution<size_t> dist(0, node->untried_moves.size() - 1);
        size_t idx = dist(rng_);
        move = node->untried_moves[idx];
        node->untried_moves.erase(node->untried_moves.begin() + idx);
    }
    
    // Create new node
    board.make_move(move);
    auto child = std::make_unique<MCTSNode>(move, node, board.current_player());
    init_untried_moves(child.get(), board);
    
    MCTSNode* child_ptr = child.get();
    node->children.push_back(std::move(child));
    
    return child_ptr;
}

double MCTS::rollout(Board& board) {
    if (board.is_terminal()) {
        int8_t winner = board.get_winner();
        if (winner == EMPTY) return 0.0;
        return (winner == board.current_player()) ? -1.0 : 1.0;
    }
    
    double total = 0.0;
    int count = 0;
    
    if (config_.use_heuristic_rollouts) {
        Board board_copy = board;
        total += heuristic_rollout(board_copy);
        ++count;
    }
    
    if (config_.use_random_rollouts) {
        Board board_copy = board;
        total += random_rollout(board_copy);
        ++count;
    }
    
    return count > 0 ? total / count : 0.0;
}

double MCTS::heuristic_rollout(Board& board) {
    int8_t start_player = board.current_player();
    int max_moves = 50; // Limit rollout length
    
    while (!board.is_terminal() && max_moves-- > 0) {
        auto scored_moves = heuristic_.get_scored_moves(board);
        if (scored_moves.empty()) break;
        
        // Pick from top moves with some randomness
        int top_n = std::min(3, static_cast<int>(scored_moves.size()));
        std::uniform_int_distribution<int> dist(0, top_n - 1);
        int idx = dist(rng_);
        
        board.make_move(scored_moves[idx].move);
    }
    
    int8_t winner = board.get_winner();
    if (winner == EMPTY) return 0.0;
    return (winner == start_player) ? 1.0 : -1.0;
}

double MCTS::random_rollout(Board& board) {
    int8_t start_player = board.current_player();
    int max_moves = 50;
    
    while (!board.is_terminal() && max_moves-- > 0) {
        auto moves = board.get_legal_moves();
        if (moves.empty()) break;
        
        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        board.make_move(moves[dist(rng_)]);
    }
    
    int8_t winner = board.get_winner();
    if (winner == EMPTY) return 0.0;
    return (winner == start_player) ? 1.0 : -1.0;
}

void MCTS::backpropagate(MCTSNode* node, double value, int8_t root_player) {
    while (node != nullptr) {
        ++node->visit_count;
        
        // Value is from perspective of player who just moved
        // We need to flip based on whose turn it is at this node
        double adjusted_value = (node->player_to_move == root_player) ? value : -value;
        node->total_value += adjusted_value;
        
        node = node->parent;
    }
}

double MCTS::uct_value(const MCTSNode* node, int parent_visits) const {
    if (node->visit_count == 0) {
        return std::numeric_limits<double>::infinity();
    }
    
    double exploitation = node->q_value();
    double exploration = config_.exploration_constant * 
                         std::sqrt(std::log(static_cast<double>(parent_visits)) / node->visit_count);
    
    // Negate because we want from parent's perspective (opponent's score)
    return -exploitation + exploration;
}

Move MCTS::select_best_move(MCTSNode* root) const {
    if (root->children.empty()) {
        // Fallback to untried moves
        if (!root->untried_moves.empty()) {
            return root->untried_moves[0];
        }
        return Move();
    }
    
    // Select most visited child
    MCTSNode* best = nullptr;
    int best_visits = -1;
    
    for (const auto& child : root->children) {
        if (child->visit_count > best_visits) {
            best_visits = child->visit_count;
            best = child.get();
        }
    }
    
    return best ? best->move : Move();
}

void MCTS::init_untried_moves(MCTSNode* node, const Board& board) {
    node->untried_moves = board.get_legal_moves();
}

} // namespace gomoku
