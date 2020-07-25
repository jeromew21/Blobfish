# Blobfish

Blobfish is a command-line only chess engine.

# Rating progress

Tested in one-second games versus Stockfish with set ELO

7/17/2020: ~1950

# Running list of features
1. UCI compatability (only partially implemented but tested with Lichess-Bot, and Cutechess)
4. Alpha-beta pruning
2. Bitboard move generation
3. Outputs principal variation
6. Static exchange evaluation
4. Late move reduction
5. Null move pruning
6. Killer heuristic
7. Countermove heuristic
7. Threefold Repetition detection (though with the transposition table it gets very messy)

# Todos (in order of priority)
8. Improve evaluation (king safety, ML tuning)
1. Improve hash table eviction policy
1. Optimize move generation (magic BB, partial movegen) and legality checking (xrays, pins, etc) 
10. Fix Mate-in-N numbering, ply adjustment
7. Move ordering concerns (history heuristic)
10. Internal iterative deepening: use low-depth search to find bestmove candidate if hash miss
3. Create testing/benchmark suite
7. Aspiration windows/fail-soft
4. Improve quiescience search
9. Test if different random seeds improve hash table performance
6. Complete implementation of UCI interface (pondering, etc)
6. Resolve exit during search issues
1. Look into parallel search
9. Look at MCTS / alternative search algorithms
