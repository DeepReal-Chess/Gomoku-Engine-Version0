#pragma once

#include "board.hpp"
#include "heuristic.hpp"
#include <vector>
#include <memory>
#include <random>
#include <chrono>

namespace gomoku {

// MCTS configuration
struct MCTSConfig {
    double exploration_constant = 1.2;  // c in UCT formula
    int max_iterations = 10000;
    int max_time_ms = 1000;
    uint64_t seed = 0;  // 0 = use time-based seed
    bool use_heuristic_rollouts = true;
    bool use_random_rollouts = true;
};

// MCTS tree node
struct MCTSNode {
    Move move;                                    // Move that led to this node
    MCTSNode* parent;                            // Parent node
    std::vector<std::unique_ptr<MCTSNode>> children;  // Child nodes
    std::vector<Move> untried_moves;             // Moves not yet expanded
    
    int visit_count;    // N
    double total_value; // W
    int8_t player_to_move; // Player who will make the next move
    
    MCTSNode(const Move& m = Move(), MCTSNode* p = nullptr, int8_t player = BLACK)
        : move(m), parent(p), visit_count(0), total_value(0.0), player_to_move(player) {}
    
    double q_value() const {
        return visit_count > 0 ? total_value / visit_count : 0.0;
    }
    
    bool is_fully_expanded() const {
        return untried_moves.empty();
    }
    
    bool is_leaf() const {
        return children.empty();
    }
};

class MCTS {
public:
    explicit MCTS(const MCTSConfig& config = MCTSConfig());
    
    // Search for best move
    Move search(const Board& board);
    Move search(const Board& board, int time_limit_ms);
    
    // Get statistics
    int get_iterations() const { return iterations_; }
    int get_root_visits() const;
    
    // Access config
    MCTSConfig& config() { return config_; }
    const MCTSConfig& config() const { return config_; }
    
private:
    MCTSConfig config_;
    Heuristic heuristic_;
    std::mt19937_64 rng_;
    int iterations_;
    
    // Core MCTS phases
    MCTSNode* select(MCTSNode* node, Board& board);
    MCTSNode* expand(MCTSNode* node, Board& board);
    double rollout(Board& board);
    void backpropagate(MCTSNode* node, double value, int8_t root_player);
    
    // UCT calculation
    double uct_value(const MCTSNode* node, int parent_visits) const;
    
    // Rollout policies
    double heuristic_rollout(Board& board);
    double random_rollout(Board& board);
    
    // Move selection
    Move select_best_move(MCTSNode* root) const;
    
    // Utility
    void init_untried_moves(MCTSNode* node, const Board& board);
};

} // namespace gomoku
