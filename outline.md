# Gomoku Engine Outline (15×15)

This document specifies a high‑performance Gomoku (Five‑in‑a‑Row) engine intended for an autonomous coding AI agent. The emphasis is on *extreme speed*, *locality‑aware logic*, *scientific Monte Carlo Tree Search*, and *deterministic, testable behavior*.

---

## 1. Board Representation and Move Logic

### 1.1 Core Constraints
- Board size: **15×15** (225 cells)
- Two players: **BLACK = 1**, **WHITE = -1**, **EMPTY = 0**
- Only moves within **Chebyshev radius ≤ 2** of any existing stone are legal
- Target performance: **~0.1 μs** per move application and update

### 1.2 Data Structures

#### 1.2.1 Board Storage
- Use a **flat 1D array** of length 225 for cache locality
  - Index: `idx = y * 15 + x`
- Store as `int8` or `int16`

#### 1.2.2 Occupancy Masks
Maintain bitsets (or equivalent) for speed:
- `occupied_mask` (225 bits)
- `black_mask`, `white_mask`

If language supports 64‑bit integers:
- Represent board as **4 x uint64 + 1 x uint33** segments

#### 1.2.3 Legal Move Mask
- Maintain a **legal_mask** bitset
- Initially empty
- On placing a stone at `(x,y)`:
  - For all `(dx,dy)` where `|dx| ≤ 2`, `|dy| ≤ 2`:
    - If inside board and empty → set legal bit
- Remove occupied square from legal mask

This ensures legality checks are **O(1)** bit operations.

### 1.3 Incremental Win Detection

#### 1.3.1 Directional Counters
For each move, check only **4 directions**:
- Horizontal `(1,0)`
- Vertical `(0,1)`
- Diagonal `(1,1)`
- Anti‑diagonal `(1,-1)`

Procedure:
- Starting from placed stone, extend in both directions
- Count consecutive same‑color stones
- Stop at first mismatch or board edge

Winning condition: `count ≥ 5`

#### 1.3.2 Terminal State Cache
- Store:
  - `is_terminal`
  - `winner` (1, -1, or 0)
- Update only on last move

---

## 2. Move Generating Heuristic

### 2.1 Design Goals
- Target: **~0.1 μs per move score**
- Purely local evaluation
- No global scans

### 2.2 Move Categories (Priority Order)

1. **Immediate Winning Moves**
   - Completing 5 in a row
2. **Forced Defensive Moves**
   - Blocking opponent’s 4‑in‑a‑row or double‑threat
3. **High Opportunity Moves**
   - Open lines, low blockage
4. **Clustering Moves**
   - Strengthen existing formations

### 2.3 Pattern‑Based Local Scoring

For candidate move `(x,y)`:

#### 2.3.1 Line Pattern Extraction
- For each direction, extract up to **9‑cell line** centered on `(x,y)`
- Encode as small integers (e.g. bit‑packed)

#### 2.3.2 Scoring Table
Precompute pattern scores:
- `XXXXX` → +∞ (win)
- `XXXX_` or `_XXXX` → very high
- `XXX_X` → high
- `_XXX_` → medium
- Blocked patterns score less

#### 2.3.3 Defensive Override
- Repeat scoring as if opponent plays `(x,y)`
- If opponent gets a win or open‑four:
  - Mark as **forced defensive**

### 2.4 Opportunity / Space Heuristic

- For each direction:
  - Count contiguous empty cells before hitting opponent stone
- Prefer moves where sum of empty spaces is higher

### 2.5 Clustering Bonus

- Count friendly stones within radius 2
- Penalize if opponent stone blocks all extensions

Final score = weighted sum with hard overrides for win/block.

---

## 3. Monte Carlo Tree Search (MCTS)

### 3.1 Tree Structure

Each node stores:
- `N` (visit count)
- `W` (total value)
- `Q = W / N`
- `children[]`
- `untried_moves[]`

### 3.2 Selection (UCT)

```
UCT = Q + c * sqrt(ln(parent.N) / N)
```
- `c ≈ 1.2`

### 3.3 Expansion

- Expand one move from `untried_moves`
- Apply move incrementally

### 3.4 Rollout Policy (Scientific Budgeting)

Each leaf node is evaluated **twice**:
1. **Pure Heuristic Playout**
   - Use move heuristic only
2. **Pure Random Playout**
   - Uniform random legal moves

Final rollout value = average of both results

### 3.5 Backpropagation

- Win = +1
- Loss = -1
- Draw = 0

---

## 4. Test Suites

### 4.1 Board Logic Tests

#### Test 1: Legal Radius
- Place stone at (7,7)
- Assert:
  - (5,5) legal
  - (9,9) legal
  - (10,7) illegal

#### Test 2: Incremental Win
- Place BLACK at:
  - (3,7),(4,7),(5,7),(6,7)
- Play (7,7)
- Assert terminal == true, winner == BLACK

### 4.2 Heuristic Tests

#### Test 3: Forced Block
- WHITE has four in a row with open end
- Heuristic must rank blocking move highest

#### Test 4: Opportunity Preference
- Two candidate moves:
  - One boxed by opponent
  - One with open extensions
- Assert open move score > blocked move score

### 4.3 MCTS Tests

#### Test 5: Winning in One
- Position with immediate winning move
- Run MCTS with small budget
- Assert chosen move is winning move

#### Test 6: Defensive Necessity
- Opponent threatens win next move
- MCTS must choose blocking move

---

## 5. UCI‑Style Interface

### 5.1 Commands

```
uci
isready
position startpos moves ...
position fen <custom_gomoku_fen>
go movetime <ms>
stop
quit
```

### 5.2 Response

```
bestmove x y
```

### 5.3 Clean Design Rules
- Stateless parsing layer
- Engine core independent of UCI
- Deterministic output for fixed random seed

---

## 6. Performance Targets Summary

| Component | Target |
|---------|-------|
| Make / unmake move | ~0.1 μs |
| Heuristic evaluation | ~0.1 μs |
| Rollout (avg) | < 5 μs |
| MCTS iteration | < 10 μs |

---

This specification is designed to be directly consumable by an autonomous coding agent, with minimal ambiguity and strong test guidance.

