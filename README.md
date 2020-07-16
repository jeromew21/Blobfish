# Chess

# Running list of features
1. UCI compatability (only partially implemented but tested with Lichess-Bot, and Cutechess)
4. Alpha-beta pruning
2. Bitboard move generation
3. Outputs principal variation
6. Static exchange evaluation
4. Late move reduction
5. Null move pruning

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
- repeated position issues? (resolved i think)

r4rk1/1pp2p1p/p1b1pPp1/4N3/4P3/2N4Q/PPPq1bPP/R6K b - - 1 1
- trouble defending mate threat (likely issue with hash table leading to not enough positions searched)

r1bq1rk1/ppppbppp/2n1p3/4P3/1nPP4/PQ3N2/1P1B1PPP/RN2KB1R b KQ - 0 8
- loses knight, poor move ordering leading to not enough positions searched (best move is ordered last)
- fixed with improved quiescience

r4rk1/4bppp/p2p1n2/1ppP4/2nP4/1Pq2N1P/P1B2PP1/1RBQR1K1 b - - 2 17
- doesn't move knight out of attack (actually moving the knight traps the queen)

2kr1b1r/p2ppppp/2p5/8/q4B2/6P1/1R2bPBP/1R4K1 b - - 0 1
- missed mate in 3 with e5??? (fixed with check extension)

8/pb1k3p/4p3/1p4P1/1K4B1/7P/5r2/8 b - - 10 42
- gets stuck checking?? (seems fixed)

1q6/6K1/8/8/8/8/4k3/1q6 b - - 41 25
- why throw (seems fixed)

2k5/p5r1/1p6/4R1pp/2pPpp2/P1n1B1P1/2P2K1P/8 w - - 0 37
- why throw (seems fixed)

6k1/r2q4/p2b3r/3ppQNp/7R/1P5P/1P3PP1/2R3K1 w - - 3 34
- Nf7?? (fixed unchecking bug)

8/8/p5k1/3p3p/5pP1/8/1q3K2/2r5 w - - 0 50
- stalls?? (mate counter is busted)

6k1/r2q1N2/p2b3r/3ppQ1p/7R/1P5P/1P3PP1/2R3K1 b - - 4 1
- why Re6 (issue with produceUncheck)

8/5k1p/8/4p1p1/4P1P1/6K1/2q5/8 b - - 5 49
- UHHH (the mate is rather deep, doesn't know the line to convert. hash table issue?)

8/pp4pp/2k1b3/P7/1B6/2K5/4rPPP/3q4 b - - 0 1
- M1 missed?? (depth as 0 indexed, was skipping 0-depth)

r4rk1/1pp1p3/1bq4p/p2n2p1/P1P1Np2/1Q1P3P/1P1B1P1P/R4RK1 b - - 0 20
- Nb4 is blunder, so is e6.. time trouble??
- very deep, tricky idea. gets Black out of this mess: Qd6 threatening mate ideas... probably just too deep for now

4r1k1/1bp2pp1/1pn1p3/1R1pr1p1/2P1B3/B7/P2P1PPP/4R1K1 w - - 0 24
- Be6?! not a blunder actually. complex position, White loses the bishop. Generating counterplay after Rxb6 

r1bqkb1r/pp3ppp/2n1p3/3p3n/2pP1B2/2N2NPB/PPP1PP1P/R2Q1RK1 w kq - 2 8
- depth seems way too shallow, stuck in q search? (fixed)

1r4k1/4Rp1p/6p1/2PQ2P1/4P3/5q2/8/4K3 w - - 1 49
- blunders Qe5

1k2r3/4p3/p4np1/2p4p/1p6/3N2PP/PPP5/2K1R3 b - - 2 33
- c4?

2kr3r/1qp1p2p/4Qnpb/1p6/8/2N1P3/PPP2PPP/2KRB2R b - - 0 19
- blocks instead of moving, loses in a very bizarre line


1b1r3k/8/5q1r/2Q5/N1B4P/1P2K1P1/1P2P3/6R1 b - - 15 47
- chess20 vs Stockfish 1600
- doesn't realize it's getting mated after 48. Qa5
- given more time it finds 48. Bd3 averting the threat?
- moves: 47. Qc5 +2.31/5 11s Rh7 +16.33/22 8.7s 48. Qa5 -1.55/4 11s Re7+ +M5/22 3.1s 49. Qe5 -100000.00/2 0s Rxe5# 

8/8/5q2/8/7k/1K6/8/8 w - - 0 1
- mate with KQ vs K

r1b3kr/3pR1p1/ppq4p/5P2/4Q3/B7/P5PP/5RK1 w - - 1 0
- isn't working (fixed due to mate glitch)

r3r1k1/1b1nq2p/p1p1N3/2p2p2/1b1PNP2/PQ1B3P/4RP2/5RK1 b - - 0 24
- blunders Qxd6.. stuck in q search... (fixed???)

5k2/1R6/4r3/7K/P7/7P/1b6/8 b - - 0 70
- wtf is Re7
