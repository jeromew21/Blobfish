# Chess

# Features
1. UCI compatability (only partially implemented but tested with Lichess-Bot, and Cutechess)
2. Bitboard move generation
3. Outputs principal variation

# Todos (in order of priority)
1. Improve hash table
1. Optimize for lazy move generation
3. Create testing suite
7. Move ordering concerns (heuristics, static exchange evaluation)
3. Speed up and fix threefold repetition (significant bottleneck)
4. Improve quiescience search (delta pruning, checks, etc)
5. Add more pruning (null, etc)
8. Improve evaluation (neural eval?)
6. Complete implementation of UCI interface (pondering, etc)
6. Resolve exit during search issues (instead of defaulting to previous ply)
2. Fix redundant calls in board (unsure of how much this effects performance)
9. Implement Scout/PVS on top of alpha-beta
9. Look at MCTS / alternative search algorithms
9. Test if different random seeds improve hash table performance
