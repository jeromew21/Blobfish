# Blobfish ![picture of a blobfish](https://raw.githubusercontent.com/jeromew21/Blobfish/mcts/resources/pic.png)

Blobfish is a command-line only chess engine, but you can plug it into a GUI or lichess.

# Intended features

1. Human vs AI move discrimination: using statistical models classify moves as engine or human; using this to play more like a human
2. Training a neural net with Monte-Carlo Tree Search
3. Support for other chess variants, and Shogi

# Running list of features
1. UCI compatability (only partially implemented; tested with Lichess-Bot and Cutechess)
4. Alpha-beta pruning and Principal Variation Search
2. Bitboard move generation
3. Outputs principal variation
6. Static exchange evaluation
4. Late move reduction
5. Null move pruning
6. Killer heuristic
7. Countermove heuristic
7. Threefold Repetition detection (though with the transposition table it gets very messy)
3. Passes Perft test (this means that the move generator is 100% correct)

# Contributing

Feel free to make a branch or something

# Todos (somewhat in order of priority)
8. Improve evaluation (king safety, ML tuning)
1. Improve hash table eviction policy
1. Look into parallel search
1. Optimize move generation (magic BB, partial movegen) and legality checking (xrays, pins, etc) 
7. Late move pruning and move ordering concerns
3. Create testing/benchmark suite6. Resolve exit during search issues
6. Resolve exit during search issues and time management
7. Aspiration windows and/or fail-soft
4. Improve quiescience search
9. Test if different random seeds improve hash table performance
6. Complete implementation of UCI interface (pondering, etc)
9. Look at MCTS / alternative search algorithms
