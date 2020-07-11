# Chess

# Features
1. UCI compatability (only partially implemented but tested with Lichess-Bot, and Cutechess)
2. Bitboard move generation
3. Outputs principal variation

# Todos (in order of priority)
1. Improve hash table
1. Optimize for lazy move generation
7. Move ordering concerns (heuristics, static exchange evaluation)
3. Create testing suite
4. Search extensions and reductions
3. Speed up and fix threefold repetition (significant bottleneck)
4. Improve quiescience search (delta pruning, checks, etc)
5. Add more pruning (null, etc)
8. Improve evaluation (neural eval?)
6. Complete implementation of UCI interface (pondering, etc)
6. Resolve exit during search issues (instead of defaulting to previous ply)
2. Fix redundant calls in board (unsure of how much this effects performance)
9. Implement Scout/PVS on top of alpha-beta
1. parallel search ideas
9. Look at MCTS / alternative search algorithms
9. Test if different random seeds improve hash table performance


## Problematic/instructive inputs
r4rk1/p1pq1ppp/bp1ppn2/6P1/2P2B1P/1PN2Q2/P1P2P2/2KR3R b - - 0 15
- complex position gets stuck on depth 1 (FIXED with move ordering)

r4rk1/p1pq1ppp/bp1p1n2/4B1P1/2P4P/1PN2Q2/P1P2P2/2KR3R b - - 0 2
- position with pin, decides to move king   

r2q1rk1/p1p2p1p/bp1p1Pp1/8/2P2Q1P/1PN5/P1P2P2/2KR2R1 w - - 0 4
- can it defend the attack?

8/5k2/8/1R6/8/2K5/4R3/8 w - - 0 1
- can it checkmate with 2 rooks?

r4rk1/pbpq1p1p/1p1p1PpQ/8/2P4P/1PN5/P1P2P2/2KR2R1 b - - 2 5
- engine tweaks, no PV printed??? (FIXED: insert root node even if node never breaches alpha)

r4rk1/p1pq1p1p/bp1p1Pp1/8/2P2Q1P/1PN5/P1P2P2/2KR2R1 b - - 0 4
- trouble: can't defend attack problem: finds that PV from previous layer is busted, but we don't want to take (FIXED: takes a risk averse approach now)

r5k1/p1pr1p1p/bp1p1Pp1/8/2P4P/1PN1R3/P1P2P2/2KR4 b - - 3 7 
- Bxc4?? seems a blunder. Move ordering needs to be fixed... (fixed, sort of. rootMove wasn't getting the score for last iteration)

2r4k/p4pp1/1p2p3/8/4n3/P2bP3/1B1R1PPP/7K w - - 0 27
- white is in big trouble: threats of back rank, can't capture bishop bc of knight fork. all moves probably losing 

8/Q4p1p/6k1/1p6/8/2q2P2/P4P1P/4K3 w - - 1 35
- repeated position issues?