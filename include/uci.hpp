#pragma once

#include "board.hpp"
#include "mcts.hpp"
#include <string>
#include <sstream>
#include <functional>

namespace gomoku {

class UCIEngine {
public:
    UCIEngine();
    
    // Main loop
    void run();
    
    // Process single command
    std::string process_command(const std::string& input);
    
    // Set output callback (for testing)
    void set_output_handler(std::function<void(const std::string&)> handler);
    
private:
    Board board_;
    MCTS mcts_;
    bool running_;
    std::function<void(const std::string&)> output_handler_;
    
    // Command handlers
    std::string cmd_uci();
    std::string cmd_isready();
    std::string cmd_position(std::istringstream& args);
    std::string cmd_go(std::istringstream& args);
    std::string cmd_stop();
    std::string cmd_quit();
    std::string cmd_display();
    std::string cmd_perft(std::istringstream& args);
    
    // Parsing helpers
    Move parse_move(const std::string& move_str) const;
    std::string move_to_string(const Move& move) const;
    
    // Output
    void output(const std::string& msg);
};

} // namespace gomoku
