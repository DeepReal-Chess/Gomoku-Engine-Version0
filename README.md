# Gomoku MCTS Engine

A high-performance Gomoku (Five-in-a-Row) engine with Monte Carlo Tree Search.

## Features

- 15×15 board with efficient bitboard representation
- Locality-aware legal move generation (Chebyshev radius ≤ 2)
- Pattern-based heuristic evaluation with threat detection
- Scientific MCTS with dual rollout policy (heuristic + random)
- Priority-based move selection with tactical awareness
- Terminal state detection for faster tree convergence
- UCI-style command interface
- Demo mode with animated self-play and game logging

## Move Selection Priority

The engine uses a priority-based move selection system that ensures correct tactical play:

| Priority | Move Type | Description |
|----------|-----------|-------------|
| 1 | **Immediate Win** | Complete 5-in-a-row to win the game |
| 2 | **Block 4-in-a-row** | Must block opponent's winning threat |
| 3 | **Create Open Four** | 4 stones with both ends open (guaranteed win) |
| 4 | **Block Open Three** | Prevent opponent from getting open four |
| 5 | **MCTS Result** | Most-visited move from tree search |

This ensures the engine:
- Always takes winning moves when available
- Never misses critical defensive moves
- Prioritizes creating unstoppable threats over blocking weaker ones

## Heuristic Evaluation

The engine uses a sophisticated pattern-based heuristic that evaluates moves in all 4 directions:

| Pattern | Score | Description |
|---------|-------|-------------|
| Win (5 in a row) | 1,000,000 | Completing or blocking a winning line |
| Open Four | 100,000 | 4 stones with both ends open |
| Closed Four | 10,000 | 4 stones with one end blocked |
| Open Three | 5,000 | 3 stones with both ends open, or gapped patterns (X_XX) |
| Closed Three | 500 | 3 stones with one end blocked |
| Open Two | 200 | 2 stones with extension potential |
| Closed Two | 20 | 2 stones with limited space |
| Space Bonus | 10/square | Bonus per empty square within radius 2 |
| Cluster Bonus | 10/stone | Bonus for nearby stones (weighted by distance) |

**Key features:**
- **All 4 directions scanned**: Horizontal, vertical, diagonal, and anti-diagonal
- **Offensive + Defensive scoring**: Each move is evaluated for both attack and defense
- **Defensive multiplier (1.1×)**: Slightly favors blocking opponent threats
- **Gap pattern detection**: Recognizes broken patterns like `X_XX` or `XX_X`
- **Forced move detection**: `find_winning_move()`, `find_open_four_move()`, `find_blocking_move()`, and `find_open_three_block()`

## MCTS Implementation

The Monte Carlo Tree Search implementation includes several optimizations:

- **UCT Selection**: Uses UCB1 formula with configurable exploration constant (default: 1.2)
- **Heuristic-guided expansion**: Prioritizes promising moves during node expansion
- **Dual rollout policy**: Combines heuristic-guided and random rollouts for balanced evaluation
- **Terminal state detection**: Marks nodes as terminal when a win is detected, using fixed values instead of rollouts
- **Backpropagation**: Updates values from the perspective of the root player for consistent evaluation

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Running

### Quick Start - Demo Game
Watch the engine play against itself with animated board display:
```bash
./gomoku demo           # Default 1000ms per move
./gomoku demo 2000      # 2 seconds per move
./gomoku demo 500       # Fast mode (500ms per move)
```

The demo will:
- Display the board updating in real-time
- Show each move with timing information
- Save the complete game log to `build/game_YYYYMMDD_HHMMSS.txt`

**Sample game result** (see `game_20260103_073440.txt`):
- BLACK (X) wins in 29 moves
- Winning line: J7-J8-J9-J10-J11 (vertical)

### Interactive UCI Mode
```bash
./gomoku
```

### Help
```bash
./gomoku --help
```

### UCI Commands
```
uci           - Initialize UCI mode
isready       - Check if engine is ready
position startpos moves a8 b8 ...  - Set position
go movetime 1000   - Search for best move (1 second)
d             - Display board
quit          - Exit
```

### Example Session
```
> uci
id name Gomoku MCTS
id author DeepReaL
uciok
> isready
readyok
> position startpos moves h8 h9 i8
> d
   A B C D E F G H I J K L M N O 
 1 . . . . . . . . . . . . . . . 
 2 . . . . . . . . . . . . . . . 
 3 . . . . . . . . . . . . . . . 
 4 . . . . . . . . . . . . . . . 
 5 . . . . . . . . . . . . . . . 
 6 . . . . . . . . . . . . . . . 
 7 . . . . . . . . . . . . . . . 
 8 . . . . . . . X X . . . . . . 
 9 . . . . . . . O . . . . . . . 
10 . . . . . . . . . . . . . . . 
11 . . . . . . . . . . . . . . . 
12 . . . . . . . . . . . . . . . 
13 . . . . . . . . . . . . . . . 
14 . . . . . . . . . . . . . . . 
15 . . . . . . . . . . . . . . . 

Current player: WHITE (O)
Move count: 3
> go movetime 1000
bestmove g8
> quit
```

## Running Tests

```bash
./test_engine
```

### Test Suite
- **Board Logic Tests**: Legal radius, incremental win detection, unmake move
- **Heuristic Tests**: Forced blocking, opportunity preference, winning move detection
- **MCTS Tests**: Winning in one, defensive necessity
- **Performance Tests**: Move operations, heuristic evaluation, MCTS iteration timing

## Performance Targets

| Component | Target | Achieved |
|-----------|--------|----------|
| Make/unmake move | ~0.1 μs | ✓ |
| Heuristic evaluation | ~0.1 μs | ✓ |
| Rollout (avg) | < 5 μs | ✓ |
| MCTS iteration | < 10 μs | ✓ |

## Project Structure

```
├── CMakeLists.txt
├── README.md
├── include/
│   ├── types.hpp      # Core types, Move struct, BitBoard, constants
│   ├── board.hpp      # Board representation with bitboard
│   ├── heuristic.hpp  # Pattern-based move evaluation
│   ├── mcts.hpp       # Monte Carlo Tree Search with UCT
│   └── uci.hpp        # UCI protocol handler
├── src/
│   ├── board.cpp      # Board implementation, win detection
│   ├── heuristic.cpp  # Pattern scoring, threat detection
│   ├── mcts.cpp       # MCTS with dual rollout policy
│   ├── uci.cpp        # UCI command parsing
│   └── main.cpp       # Entry point, demo game
└── tests/
    └── test_engine.cpp
```

## License

MIT License
