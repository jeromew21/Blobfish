#include <game/ai.hpp>

TranspositionTable table;

u64 BACK_RANK[2];

void AI::init() {
    BACK_RANK[White] = u64FromIndex(0) | u64FromIndex(1) | u64FromIndex(2) | u64FromIndex(3) | u64FromIndex(4) | u64FromIndex(5) | u64FromIndex(6) | u64FromIndex(7);
    BACK_RANK[Black] = u64FromIndex(56) | u64FromIndex(57) | u64FromIndex(58) | u64FromIndex(59) | u64FromIndex(60) | u64FromIndex(61) | u64FromIndex(62) | u64FromIndex(63);
}

int AI::max(int i1, int i2) {
    if (i1 > i2) {
        return i1;
    } else {
        return i2;
    }
}

int AI::min(int i1, int i2) {
    if (i1 < i2) {
        return i1;
    } else {
        return i2;
    }
}

int AI::materialEvaluation(Board &board) {
    return board.material();
}

int AI::evaluation(Board &board) { //positive good for white, negative for black
    BoardStatus status = board.status();
    if (status == BoardStatus::Stalemate || status == BoardStatus::Draw) {
        return 0;
    }
    int score = materialEvaluation(board);
    for (int i = 0; i < (int) board.stack.getIndex(); i++) {
        Move mv = board.stack.peekAt(i);
        MoveType mvtyp = mv.moveType;
        if (mvtyp == MoveType::CastleLong || mvtyp == MoveType::CastleShort) {
            if (mv.mover == W_King) {
                score += 30; //castling is worth 30 cp
            } else {
                score -= 30;
            }
        }
    }
    u64 minors = board.bitboard[W_Bishop] | board.bitboard[W_Knight];
    int backrank = hadd(minors & BACK_RANK[White]); //0-8
    score -= backrank*15;

    minors = board.bitboard[B_Bishop] | board.bitboard[B_Knight];
    backrank = hadd(minors & BACK_RANK[Black]); //0-8
    score += backrank*15;

    //mobility
    //naive solution: make a null move
    int mcwhite = board.mobility(White);
    int mcblack = board.mobility(Black);
    score += mcwhite;
    score -= mcblack;

    if (status == BoardStatus::WhiteWin) {
        return INTMAX;
    }
    if (status == BoardStatus::BlackWin) {
        return INTMIN;
    }
    return score;
}

int AI::flippedEval(Board &board) {
    if (board.turn() == White) {
        return AI::evaluation(board);
    } else {
        return -1*AI::evaluation(board);
    }
}

Move AI::rootMove(Board &board, int depth, std::atomic<bool> &stop, int &outscore) {
    sendCommand("info string ID DEPTH=" + std::to_string(depth));
    TableNode node(board, depth, NodeType::PV);
    int count = 1;

    auto moves = board.legalMoves();

    board.dump(true);

    orderMoves(board, moves);
    int alpha = INTMIN;
    Move chosen = moves[moves.size() - 1]; //PV-move

    //order moves
    auto search = table.find(node);
    if (search != table.end()) {
        if (search->first.nodeType == NodeType::PV) {
            Move pvMove = search->first.bestMove;

            for (int i = 0; i < (int) moves.size(); i++) {
                if (moves[i] == pvMove) {
                    Move temp = moves[moves.size() - 1];
                    moves[moves.size() - 1] = pvMove;
                    moves[i] = temp;
                    break;
                }
            }
            chosen = moves[moves.size() - 1];
        }
    } else {
        node.bestMove = chosen; //not in table, so change it
        table.insert(node, alpha);
    }
    node.bestMove = chosen; //not in table, so change it

    int beta = INTMAX;
    outscore = alpha;

    bool raisedAlpha = false;
    
    bool pvFlag = true;
    auto start = std::chrono::high_resolution_clock::now();

    while (moves.size() > 0) {
        Move mv = moves.back();
        moves.pop_back();
        board.makeMove(mv);
        int score = -1*AI::alphaBetaNega(board, depth, -1*beta, -1*alpha, stop, count, pvFlag);
        board.unmakeMove();
        pvFlag = false;

        if (stop) {
            outscore = alpha;
            return chosen; //returns best found so far
        } //here bc if AB call stopped, it won't be full search

        if (score > alpha) {
            raisedAlpha = true;
            alpha = score;
            chosen = mv;
            outscore = alpha;
            node.bestMove = chosen;
            table.insert(node, alpha); //new PV
            auto stop = std::chrono::high_resolution_clock::now(); 
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start); //or milliseconds
            int ms = duration.count();
            sendPV(board, depth, count, alpha, ms);
            sendCommand("info hashfull " + std::to_string(table.ppm()));
        }
    }
    if (!raisedAlpha) {
        table.insert(node, alpha);
        auto stop = std::chrono::high_resolution_clock::now(); 
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start); //or milliseconds
        int ms = duration.count();
        sendPV(board, depth, count, alpha, ms);
    }
    return chosen;
}

void sendPV(Board &board, int depth, int nodeCount, int score, int time) {
    std::string pv = " pv ";
    int mc = 0;
    for (int k = 0; k < depth; k++) {
        TableNode nodeSearch(board, depth, NodeType::PV);
        auto search = table.find(nodeSearch);
        if (search != table.end()) {
            TableNode node = search->first;
            if (node.nodeType == NodeType::PV || true) {
                Move mv = node.bestMove;
                pv += board.moveToUCIAlgebraic(mv);
                pv += " ";
                board.makeMove(mv);
                mc ++;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    for (int k = 0; k < mc; k++) {
        board.unmakeMove();
    }
    sendCommand(
        "info depth " + std::to_string(depth) + 
        " score cp " + std::to_string(score) + 
        " time " + std::to_string(time) + 
        " nps " + std::to_string( (int) ((double)nodeCount/((double)time/1000.0))) + 
        " nodes " + std::to_string(nodeCount) + pv);
}

TranspositionTable& AI::getTable() {
    return table;
}

int AI::quiescence(Board &board, int plyCount, int alpha, int beta, std::atomic<bool> &stop, int &count) {
    count++;
    auto moves = board.legalMoves();
    int baseline = AI::flippedEval(board);
    if (board.isTerminal()) {
        return baseline; //why was this ommitted?
    }

    if (plyCount == 0) {
        //check table
        TableNode node(board, 0, NodeType::All);
        auto found = table.find(node);
        if (found != table.end()) {
            NodeType typ = found->first.nodeType;
            if (typ == NodeType::All) {
                //upper bound, the exact score might be less.
                beta = AI::min(beta, found->second);
            } else if (typ == NodeType::Cut) {
                //lower bound
                alpha = AI::max(alpha, found->second);
                
            } else if (typ == NodeType::PV) {
                return found->second;
            }
            if (alpha >= beta) {
                return found->second;
            }
        }
    }

    if (baseline >= beta) return beta; // fail hard

    if (alpha < baseline) alpha = baseline; //push alpha up to baseline

    u64 piecemap = board.occupancy();
    for (Move& mv : moves) {
        if (stop) {
            return INTMIN; //mitigate horizon effect, otherwise we could be in big trouble
        }
        if (mv.dest & piecemap) {
            //captures, and checks???????????
            board.makeMove(mv);
            int score = -1*quiescence(board, plyCount + 1, -1*beta, -1*alpha, stop, count);
            board.unmakeMove();
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
    }
    return alpha;
}

void AI::orderMoves(Board &board, std::vector<Move> &mvs) {
    //put captures first
}


int AI::alphaBetaNega(Board &board, int depth, int alpha, int beta, std::atomic<bool> &stop, int &count, bool isPv) {
    count++;

    if (board.isTerminal()) {
        int score = AI::flippedEval(board); // store?
        return score;
    }

    if (depth == 0) {
        int score = quiescence(board, 0, alpha, beta, stop, count);
        return score;
    }

    auto moves = board.legalMoves();
    orderMoves(board, moves);
    //order moves

    TableNode node(board, depth, NodeType::All);
    auto found = table.find(node);
    if (found != table.end()) {
        if (found->first.depth >= depth) { //searched already to a higher depth
            NodeType typ = found->first.nodeType;
            if (typ == NodeType::All) {
                //upper bound, the exact score might be less.
                beta = AI::min(beta, found->second);
            } else if (typ == NodeType::Cut) {
                //lower bound
                alpha = AI::max(alpha, found->second);
                
            } else if (typ == NodeType::PV) {
                return found->second;
            }
            if (alpha >= beta) {
                return found->second;
            }
        }
        //Put hash move first
        Move pvMove = found->first.bestMove;
        for (int i = 0; i < (int) moves.size(); i++) {
            if (moves[i] == pvMove) {
                Move temp = moves[moves.size() - 1];
                moves[moves.size() - 1] = pvMove;
                moves[i] = temp;
                break;
            }
        }
    }

    if (isPv) {
        node.nodeType = NodeType::PV;
        node.bestMove = moves[moves.size() - 1]; //PV move is first move
        table.insert(node, alpha); //overwrite with score as alpha
    }

    
    /*
    bool nullmove = true;
    //NULL MOVE PRUNE
    //MAKE SURE NOT IN CHECK??
    if (nullmove && !board.isInCheck(board.turn())) {
        board.makeMove(Move::NullMove());
        int score = -1*AI::alphaBetaNega(board, AI::max(0, depth-2), -1*beta, -1*alpha, stop, count, false); //limit depth
        board.unmakeMove();
        if (score >= beta) { //our move is better than beta, so this node is cut off
            node.nodeType = NodeType::Cut;
            node.bestMove = moves[moves.size() - 1];
            return beta; //fail hard
        }
        if (score > alpha) {
            alpha = score; //push up alpha
        }
    }
    */

    while (moves.size() > 0) {
        Move mv = moves.back();
        moves.pop_back();
    //for (Move& mv : moves) {
        board.makeMove(mv);
        int score = -1*AI::alphaBetaNega(board, depth-1, -1*beta, -1*alpha, stop, count, false); //best move for oppo
        board.unmakeMove();

        if (stop) { //if stopped in subcall, then we want to ignore it
            return alpha;
        }
        
        if (score >= beta) { //our move is better than beta, so this node is cut off
            node.nodeType = NodeType::Cut;
            node.bestMove = mv;
            table.insert(node, beta);
            return beta; //fail hard
        }

        if (score > alpha) {
            node.nodeType = NodeType::PV;
            node.bestMove = mv;
            alpha = score; //push up alpha
        }
    }
    table.insert(node, alpha); //store node
    return alpha;
}
