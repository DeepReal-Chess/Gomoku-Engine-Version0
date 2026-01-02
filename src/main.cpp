#include "uci.hpp"
#include "board.hpp"
#include "mcts.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    // ANSI escape code to clear screen and move cursor to top
    std::cout << "\033[2J\033[H";
#endif
}

std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string move_to_str(const gomoku::Move& move) {
    if (!move.is_valid()) return "none";
    char col = 'A' + move.x;
    int row = move.y + 1;
    return std::string(1, col) + std::to_string(row);
}

void demo_game(int movetime_ms) {
    using namespace gomoku;
    
    Board board;
    MCTSConfig config;
    config.max_time_ms = movetime_ms;
    config.max_iterations = 100000;
    MCTS mcts(config);
    
    // Create log file
    std::string filename = "game_" + get_timestamp() + ".txt";
    std::ofstream log_file(filename);
    
    std::cout << "=== Gomoku Demo Game ===" << std::endl;
    std::cout << "Search time: " << movetime_ms << "ms per move" << std::endl;
    std::cout << "Game log: " << filename << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Log header
    log_file << "========================================" << std::endl;
    log_file << "         GOMOKU GAME LOG" << std::endl;
    log_file << "========================================" << std::endl;
    log_file << "Date: " << get_timestamp() << std::endl;
    log_file << "Search time: " << movetime_ms << "ms per move" << std::endl;
    log_file << "----------------------------------------" << std::endl;
    log_file << std::endl;
    
    int move_num = 0;
    std::vector<std::string> move_list;
    
    while (!board.is_terminal()) {
        ++move_num;
        
        // Search for best move
        auto start = std::chrono::high_resolution_clock::now();
        Move best = mcts.search(board, movetime_ms);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::string player_name = (board.current_player() == BLACK) ? "BLACK (X)" : "WHITE (O)";
        std::string move_str = move_to_str(best);
        move_list.push_back(move_str);
        
        // Make the move
        board.make_move(best);
        
        // Clear screen and display
        clear_screen();
        
        std::cout << "=== Gomoku Demo Game ===" << std::endl;
        std::cout << std::endl;
        std::cout << board.to_string();
        std::cout << std::endl;
        std::cout << "Move " << move_num << ": " << player_name << " plays " << move_str;
        std::cout << " (" << duration << "ms, " << mcts.get_iterations() << " iterations)" << std::endl;
        std::cout << std::endl;
        
        // Print move history
        std::cout << "Moves: ";
        for (size_t i = 0; i < move_list.size(); ++i) {
            if (i > 0) std::cout << " ";
            if (i % 2 == 0) {
                std::cout << (i/2 + 1) << ".";
            }
            std::cout << move_list[i];
        }
        std::cout << std::endl;
        
        // Log the move
        log_file << "Move " << std::setw(3) << move_num << ": ";
        log_file << std::setw(10) << player_name << " -> " << move_str;
        log_file << " (" << duration << "ms)" << std::endl;
        
        // Wait before next move
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Game over
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::string result_str;
    switch (board.get_result()) {
        case GameResult::BLACK_WIN:
            result_str = "BLACK (X) WINS!";
            break;
        case GameResult::WHITE_WIN:
            result_str = "WHITE (O) WINS!";
            break;
        case GameResult::DRAW:
            result_str = "DRAW!";
            break;
        default:
            result_str = "Unknown";
    }
    std::cout << "GAME OVER: " << result_str << std::endl;
    std::cout << "Total moves: " << move_num << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Log final result
    log_file << std::endl;
    log_file << "----------------------------------------" << std::endl;
    log_file << "RESULT: " << result_str << std::endl;
    log_file << "Total moves: " << move_num << std::endl;
    log_file << "----------------------------------------" << std::endl;
    log_file << std::endl;
    log_file << "Final position:" << std::endl;
    log_file << board.to_string();
    log_file << std::endl;
    log_file << "Move list: ";
    for (size_t i = 0; i < move_list.size(); ++i) {
        if (i > 0) log_file << " ";
        if (i % 2 == 0) {
            log_file << (i/2 + 1) << ".";
        }
        log_file << move_list[i];
    }
    log_file << std::endl;
    
    log_file.close();
    std::cout << std::endl;
    std::cout << "Game saved to: " << filename << std::endl;
}

void print_usage(const char* program) {
    std::cout << "Gomoku MCTS Engine" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  (no args)     Start UCI mode (interactive)" << std::endl;
    std::cout << "  demo          Play a demo game (self-play)" << std::endl;
    std::cout << "  demo <ms>     Demo with custom think time (default: 1000ms)" << std::endl;
    std::cout << "  --help, -h    Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "UCI Commands (in interactive mode):" << std::endl;
    std::cout << "  uci           Initialize engine" << std::endl;
    std::cout << "  isready       Check if ready" << std::endl;
    std::cout << "  position startpos [moves ...]" << std::endl;
    std::cout << "  go movetime <ms>" << std::endl;
    std::cout << "  d             Display board" << std::endl;
    std::cout << "  quit          Exit" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (std::strcmp(argv[1], "demo") == 0) {
            int movetime = 1000;
            if (argc > 2) {
                movetime = std::atoi(argv[2]);
                if (movetime <= 0) movetime = 1000;
            }
            demo_game(movetime);
            return 0;
        } else if (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    // Default: UCI mode
    gomoku::UCIEngine engine;
    engine.run();
    return 0;
}
