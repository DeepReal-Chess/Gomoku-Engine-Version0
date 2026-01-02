#include "uci.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace gomoku {

UCIEngine::UCIEngine() : running_(false) {
    output_handler_ = [](const std::string& msg) {
        std::cout << msg << std::endl;
    };
}

void UCIEngine::run() {
    running_ = true;
    std::string line;
    
    while (running_ && std::getline(std::cin, line)) {
        std::string response = process_command(line);
        if (!response.empty()) {
            output(response);
        }
    }
}

std::string UCIEngine::process_command(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;
    
    // Convert to lowercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    
    if (cmd == "uci") {
        return cmd_uci();
    } else if (cmd == "isready") {
        return cmd_isready();
    } else if (cmd == "position") {
        return cmd_position(iss);
    } else if (cmd == "go") {
        return cmd_go(iss);
    } else if (cmd == "stop") {
        return cmd_stop();
    } else if (cmd == "quit" || cmd == "exit") {
        return cmd_quit();
    } else if (cmd == "d" || cmd == "display") {
        return cmd_display();
    } else if (cmd == "perft") {
        return cmd_perft(iss);
    } else if (cmd == "ucinewgame") {
        board_.reset();
        return "";
    }
    
    return "";
}

void UCIEngine::set_output_handler(std::function<void(const std::string&)> handler) {
    output_handler_ = std::move(handler);
}

std::string UCIEngine::cmd_uci() {
    return "id name Gomoku MCTS\nid author DeepReaL\nuciok";
}

std::string UCIEngine::cmd_isready() {
    return "readyok";
}

std::string UCIEngine::cmd_position(std::istringstream& args) {
    std::string token;
    args >> token;
    
    if (token == "startpos") {
        board_.reset();
        args >> token; // Try to get "moves"
    } else if (token == "fen") {
        // Custom FEN-like format: not implemented, reset instead
        board_.reset();
        // Skip until "moves"
        while (args >> token && token != "moves") {}
    }
    
    // Process moves
    if (token == "moves") {
        while (args >> token) {
            Move m = parse_move(token);
            if (m.is_valid() && board_.is_legal(m)) {
                board_.make_move(m);
            }
        }
    }
    
    return "";
}

std::string UCIEngine::cmd_go(std::istringstream& args) {
    int time_ms = 1000; // Default
    
    std::string token;
    while (args >> token) {
        if (token == "movetime") {
            args >> time_ms;
        } else if (token == "depth") {
            int depth;
            args >> depth;
            mcts_.config().max_iterations = depth * 1000;
        } else if (token == "nodes") {
            int nodes;
            args >> nodes;
            mcts_.config().max_iterations = nodes;
        }
    }
    
    Move best = mcts_.search(board_, time_ms);
    return "bestmove " + move_to_string(best);
}

std::string UCIEngine::cmd_stop() {
    // In a real implementation, this would stop an ongoing search
    return "";
}

std::string UCIEngine::cmd_quit() {
    running_ = false;
    return "";
}

std::string UCIEngine::cmd_display() {
    std::string result = board_.to_string();
    result += "\nCurrent player: ";
    result += (board_.current_player() == BLACK ? "BLACK (X)" : "WHITE (O)");
    result += "\nMove count: " + std::to_string(board_.move_count());
    if (board_.is_terminal()) {
        result += "\nGame over: ";
        switch (board_.get_result()) {
            case GameResult::BLACK_WIN: result += "BLACK wins"; break;
            case GameResult::WHITE_WIN: result += "WHITE wins"; break;
            case GameResult::DRAW: result += "Draw"; break;
            default: break;
        }
    }
    return result;
}

std::string UCIEngine::cmd_perft(std::istringstream& args) {
    int depth = 1;
    args >> depth;
    
    // Simple perft - count legal moves at depth
    std::function<uint64_t(Board&, int)> perft = [&](Board& b, int d) -> uint64_t {
        if (d == 0) return 1;
        if (b.is_terminal()) return 0;
        
        uint64_t count = 0;
        auto moves = b.get_legal_moves();
        for (const auto& m : moves) {
            b.make_move(m);
            count += perft(b, d - 1);
            b.unmake_move(m);
        }
        return count;
    };
    
    Board b = board_;
    uint64_t nodes = perft(b, depth);
    return "perft " + std::to_string(depth) + ": " + std::to_string(nodes);
}

Move UCIEngine::parse_move(const std::string& move_str) const {
    if (move_str.length() < 2) return Move();
    
    // Format: a1, b2, etc. or 0,0 style
    int x = -1, y = -1;
    
    if (std::isalpha(move_str[0])) {
        // Letter-number format (a1 = 0,0)
        char col = std::tolower(move_str[0]);
        x = col - 'a';
        
        try {
            y = std::stoi(move_str.substr(1)) - 1;
        } catch (...) {
            return Move();
        }
    } else if (move_str.find(',') != std::string::npos) {
        // x,y format
        size_t comma = move_str.find(',');
        try {
            x = std::stoi(move_str.substr(0, comma));
            y = std::stoi(move_str.substr(comma + 1));
        } catch (...) {
            return Move();
        }
    }
    
    if (in_bounds(x, y)) {
        return Move(x, y);
    }
    return Move();
}

std::string UCIEngine::move_to_string(const Move& move) const {
    if (!move.is_valid()) return "none";
    
    char col = 'a' + move.x;
    int row = move.y + 1;
    return std::string(1, col) + std::to_string(row);
}

void UCIEngine::output(const std::string& msg) {
    if (output_handler_) {
        output_handler_(msg);
    }
}

} // namespace gomoku
