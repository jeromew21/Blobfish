#include <game/ai.hpp>

TranspositionTable table;
MiniTable pvTable;
KillerTable kTable;
HistoryTable hTable;
CounterMoveTable cTable;

u64 BACK_RANK[2];
u64 FILES[64];

void AI::init() {
  BACK_RANK[White] = u64FromIndex(0) | u64FromIndex(1) | u64FromIndex(2) |
                     u64FromIndex(3) | u64FromIndex(4) | u64FromIndex(5) |
                     u64FromIndex(6) | u64FromIndex(7);
  BACK_RANK[Black] = u64FromIndex(56) | u64FromIndex(57) | u64FromIndex(58) |
                     u64FromIndex(59) | u64FromIndex(60) | u64FromIndex(61) |
                     u64FromIndex(62) | u64FromIndex(63);
  for (int col = 0; col < 8; col++) {
    for (int row = 0; row < 8; row++) {
      u64 res = 0;
      for (int i = 0; i < 8; i++) {
        res |= u64FromPair(i, col);
      }
      FILES[intFromPair(row, col)] = res;
    }
  }
}

void AI::reset() {
  kTable.clear();
  hTable.clear();
  cTable.clear();
}

int AI::materialEvaluation(Board &board) { return board.material(); }

float AI::kingSafety(Board &board, Color c) {
  // open files -10 for being on one
  //-5 for being next to one
  // pawn shield +10 bonus for 3 pawns and bottom row
  int score = 0;
  // u64 friendlies = board.occupancy(c);
  // u64 enemies = board.occupancy(flipColor(c));
  u64 kingBB = c == White ? board.bitboard[W_King] : board.bitboard[B_King];
  u64 pawns = c == White ? board.bitboard[W_Pawn] : board.bitboard[B_Pawn];
  int index = u64ToIndex(kingBB);
  // int col = u64ToCol(kingBB);
  u64 file = FILES[index];
  int pawnsAdj = 0;
  if (pawns & file) {
    score += 1;
    pawnsAdj += hadd(kingMoves(index) & pawns);
  }
  if (kingBB & BACK_RANK[c]) {
    pawnsAdj += hadd(kingMoves(index) & pawns);
    score += 1;
  }
  score += pawnsAdj;

  return score;
}

int AI::mobility(Board &board, Color c) { // Minor piece and king mobility
  int result = 0;
  u64 friendlies = board.occupancy(c);
  u64 pieces = c == White
                   ? board.bitboard[W_Knight] | board.bitboard[W_Bishop] |
                         board.bitboard[W_Rook]
                   : board.bitboard[B_Knight] | board.bitboard[B_Bishop] |
                         board.bitboard[B_Rook];
  std::array<u64, 64> arr;
  int count;

  bitscanAll(arr, pieces, count);
  for (int i = 0; i < count; i++) {
    result += hadd(board.attackMap[u64ToIndex(arr[i])] & ~friendlies);
  }

  return result; // todo fix
}

int AI::evaluation(Board &board) {
  BoardStatus status = board.status();
  if (status == BoardStatus::Stalemate || status == BoardStatus::Draw)
    return 0;
  if (status == BoardStatus::WhiteWin)
    return INTMAX;
  if (status == BoardStatus::BlackWin)
    return INTMIN;

  int score = materialEvaluation(board);

  // mobility
  int mcwhite = mobility(board, White) - 31;
  int mcblack = mobility(board, Black) - 31;
  score += mcwhite - mcblack;

  // Piece-squares
  // Interpolate between 32 pieces and 2 pieces
  float pieceCount = ((float)hadd(board.occupancy())) / 32.0f;
  float earlyWeight = pieceCount;
  float lateWeight = 1.0f - pieceCount;
  float pscoreEarly = 0.0f;
  float pscoreLate = 0.0f;
  for (PieceType p = 0; p < 6; p++) {
    pscoreEarly += board.pieceScoreEarlyGame[p];
    pscoreLate += board.pieceScoreLateGame[p];
  }
  score += (int)(pscoreEarly * earlyWeight + pscoreLate * lateWeight);

  pscoreEarly = 0.0f;
  pscoreLate = 0.0f;
  for (PieceType p = 6; p < 12; p++) {
    pscoreEarly += board.pieceScoreEarlyGame[p];
    pscoreLate += board.pieceScoreLateGame[p];
  }
  score -= (int)(pscoreEarly * earlyWeight + pscoreLate * lateWeight);

  score += ((float)kingSafety(board, White) * earlyWeight);
  score -= ((float)kingSafety(board, Black) * earlyWeight);

  return score;
}

int AI::flippedEval(Board &board) {
  if (board.turn() == White) {
    return AI::evaluation(board);
  } else {
    return -1 * AI::evaluation(board);
  }
}

void AI::clearKillerTable() { kTable.clear(); }

Move AI::rootMove(Board &board, int depth, std::atomic<bool> &stop,
                  int &outscore, Move prevPv, int &count,
                  std::chrono::_V2::system_clock::time_point start) {
  // IID
  TableNode node(board, depth, PV);

  auto moves = board.legalMoves();
  orderMoves(board, moves, 0);

  int alpha = INTMIN;
  int beta = INTMAX;

  if (depth > 0) {
    for (int i = 0; i < (int)moves.size(); i++) {
      if (moves[i] == prevPv) {
        moves.erase(moves.begin() + i);
        moves.push_back(prevPv);
        break;
      }
    }
  } // otherwise at depth = 0 we want our first move to be the PV move

  Move chosen = moves[moves.size() - 1]; // PV-move

  /*
  std::cout << "info string moveorder";
  for (Move &mv : moves) {
      std::cout << board.moveToAlgebraic(mv) << " ";
  }
  std::cout << "\n";
  */
  outscore = alpha;

  bool raisedAlpha = false;
  NodeType nodeTyp = PV;

  while (moves.size() > 0) {
    Move mv = moves.back();
    moves.pop_back();
    board.makeMove(mv);
    int score = -1 * AI::alphaBetaNega(board, depth, 0, -1 * beta, -1 * alpha,
                                       stop, count, nodeTyp);
    board.unmakeMove();

    nodeTyp = All; // predicted cut??

    if (stop) {
      outscore = alpha;
      return chosen; // returns best found so far
    }                // here bc if AB call stopped, it won't be full search

    if (score > alpha) {
      raisedAlpha = true;
      alpha = score;
      chosen = mv;
      outscore = alpha;
      node.bestMove = chosen;

      table.insert(node, alpha); // new PV found
      pvTable.insert(node, alpha);

      sendPV(board, depth, chosen, count, alpha, start);
    }
  }
  if (!raisedAlpha) {
    table.insert(node, alpha);
    pvTable.insert(node, alpha); // new PV
    sendPV(board, depth, chosen, count, alpha, start);
  }
  return chosen;
}

void AI::sendPV(Board &board, int depth, Move pvMove, int nodeCount, int score,
                std::chrono::_V2::system_clock::time_point start) {
  if (depth == 0)
    return;
  std::string pv = " pv " + moveToUCIAlgebraic(pvMove);

  board.makeMove(pvMove);
  int mc = 1;

  depth = min(depth, 15);

  for (int k = 0; k < depth - 1; k++) {
    TableNode nodeSearch(board, depth, PV);
    auto search = pvTable.find(nodeSearch);
    if (search != pvTable.end()) {
      TableNode node = search->first;
      Move mv = node.bestMove;
      pv += " ";
      pv += moveToUCIAlgebraic(mv);
      if (mv.isNull()) {
        std::cout << score << " " << pv << "\n";
        board.dump(true);
        throw;
      }
      board.makeMove(mv);
      mc++;
    } else {
      break;
    }
  }
  for (int k = 0; k < mc; k++) {
    board.unmakeMove();
  }

  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      stop - start); // or milliseconds
  int time = duration.count();

  sendCommand(
      "info depth " + std::to_string(depth) + " score cp " +
      std::to_string(score) + " time " + std::to_string(time) + " nps " +
      std::to_string((int)((double)nodeCount / ((double)time / 1000.0))) +
      " nodes " + std::to_string(nodeCount) + pv);
}

TranspositionTable &AI::getTable() { return table; }

void AI::orderMoves(Board &board, std::vector<Move> &mvs, int ply) {
  // put see > 0 captures at front first
  std::vector<Move> winningCaptures;
  std::vector<Move> equalCaptures;
  std::vector<Move> heuristics;
  std::vector<Move> other;

  u64 piecemap = board.occupancy();
  for (Move &mv : mvs) {
    u64 dest = mv.getDest();

    if (dest & piecemap) {
      int see = board.see(mv);
      if (see > 0) {
        winningCaptures.push_back(mv);
      } else if (see == 0) {
        equalCaptures.push_back(mv);
      } else {
        other.push_back(mv);
      }
    } else if (kTable.contains(mv, ply)) {
      // two plies also???
      heuristics.push_back(mv);
    } else if (cTable.contains(board.lastMove(), mv, board.turn())) {
      heuristics.push_back(mv);
    } else {
      other.push_back(mv);
    }
  }
  mvs.clear();
  mvs.assign(other.begin(), other.end());
  mvs.insert(mvs.end(), equalCaptures.begin(), equalCaptures.end());
  mvs.insert(mvs.end(), heuristics.begin(), heuristics.end());
  mvs.insert(mvs.end(), winningCaptures.begin(), winningCaptures.end());
}

int AI::quiescence(Board &board, int plyCount, int alpha, int beta,
                   std::atomic<bool> &stop, int &count, int kickoff) {

  count++;

  int baseline = AI::flippedEval(board);

  BoardStatus status = board.status();

  if (status != BoardStatus::Playing) {
    if (status == BoardStatus::BlackWin || status == BoardStatus::WhiteWin) {
      // we got mated
      baseline += plyCount;
      return baseline;
    }
    return baseline; // alpha vs baseline...
  }

  if (baseline >= beta)
    return beta; // fail hard

  if (alpha < baseline)
    alpha = baseline; // push alpha up to baseline

  u64 occ = board.occupancy();

  bool deltaPrune = hadd(occ) > 8;

  bool isCheck = board.isCheck();

  LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial = false;
  Move mv = board.nextMove(movegen, sbuffer, hasGenSpecial);

  std::vector<Move> quietMoves;

  bool raisedAlpha = false;

  while (mv.notNull()) {
    if (stop) {
      return INTMIN; // mitigate horizon effect
    }

    int see = -1;
    bool isCapture = mv.getDest() & occ;
    if (isCapture) {
      see = board.see(mv);
    }
    bool isPromotion = mv.isPromotion();
    bool isDeltaPrune =
        deltaPrune &&
        (baseline + 200 + MATERIAL_TABLE[board.pieceAt(mv.getDest())] < alpha);
    bool isChecking = kickoff <= 5 && board.isCheckingMove(mv);

    if (((!isDeltaPrune && see >= 0) || isChecking) ||
        (isPromotion || isCheck)) {
      board.makeMove(mv);
      int score = -1 * quiescence(board, plyCount + 1, -1 * beta, -1 * alpha,
                                  stop, count, kickoff + 1);
      board.unmakeMove();
      if (score >= beta)
        return beta;
      if (score > alpha) {
        raisedAlpha = true;
        alpha = score;
      }
    } else {
      quietMoves.push_back(mv);
    }
    mv = board.nextMove(movegen, sbuffer, hasGenSpecial);
  }

  if (!raisedAlpha) {
    for (Move mv : quietMoves) {
      if (stop) {
        return INTMIN;
      }
      if (board.isCheckingMove(mv)) {
        board.makeMove(mv);
        int score = -1 * quiescence(board, plyCount + 1, -1 * beta, -1 * alpha,
                                    stop, count, kickoff + 1);
        board.unmakeMove();
        if (score >= beta)
          return beta;
        if (score > alpha) {
          alpha = score;
        }
      }
    }
  }
  return alpha;
}

int AI::alphaBetaNega(Board &board, int depth, int plyCount, int alpha,
                      int beta, std::atomic<bool> &stop, int &count,
                      NodeType myNodeType) {
  count++;

  BoardStatus status = board.status();
  TableNode node(board, depth, myNodeType);

  if (status != BoardStatus::Playing) {
    int score = AI::flippedEval(board); // store?
    if (status == BoardStatus::BlackWin || status == BoardStatus::WhiteWin) {
      // we got mated
      score += plyCount;
    }
    // update the table
    node.nodeType = PV;
    node.depth = INTMAX;
    // import for threefold positions. If it's seen the position before as a
    // non-repeated position, then we want to always overwrite.
    table.insert(node, score);
    return score;
  }

  if (depth <= 0) {
    int score = quiescence(board, plyCount + 1, alpha, beta, stop, count, 0);
    node.nodeType = PV;
    node.depth = 0;
    table.insert(node, score);
    return score;
  }

  static int hits_;
  static int total_;
  total_ += 1;
  if (total_ % 100000 == 0) {
    sendCommand("info string hitrate " +
                std::to_string((double)hits_ / (double)total_));
  }

  Move refMove;

  auto found = table.find(node);
  if (found != table.end()) {
    hits_++;
    if (found->first.depth >= depth) { // searched already to a higher depth
      NodeType typ = found->first.nodeType;
      if (typ == All) {
        // upper bound, the exact score might be less.
        beta = min(beta, found->second);
      } else if (typ == Cut) {
        // lower bound
        refMove = found->first.bestMove;
        alpha = max(alpha, found->second);
      } else if (typ == PV) {
        return found->second;
      }
      if (alpha >= beta) {
        return found->second;
      }
    } else {
      // Ideally a PV-node from prior iteration
      refMove = found->first.bestMove;
    }
  }

  // use IID to find best move????

  bool nodeIsCheck = board.isCheck();

  bool nullmove = true;
  bool lmr = true;
  bool futilityPrune = true;

  // NULL MOVE PRUNE
  if ((nullmove && !nodeIsCheck) &&
      (board.lastMove().notNull() && hadd(board.occupancy()) > 8)) {
    int r = 2;
    Move mv = Move::NullMove();
    board.makeMove(mv);
    int score = -1 * AI::alphaBetaNega(board, depth - 1 - r, plyCount + 1,
                                       -1 * beta, -1 * alpha, stop, count, All);
    board.unmakeMove();
    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      node.nodeType = Cut;
      node.bestMove = Move::NullMove();
      // node.depth = depth - r;
      table.insert(node, beta);
      return beta; // fail hard
    }
    if (score > alpha) {
      alpha = score; // push up alpha
    }
  }

  int fscore = 0;
  if (depth == 1 && futilityPrune) {
    fscore = AI::flippedEval(board);
  }

  u64 occ = board.occupancy();

  LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial = false;

  std::vector<Move> hashMoves;
  std::vector<Move> posCaptures;
  std::vector<Move> eqCaptures;
  std::vector<Move> heuristics;
  std::vector<Move> other;
  int phase = 0;
  // priority:
  // 0) hashmove (1)
  // 1) winning caps (?)
  // 2) heuristic moves: killer and counter move (2)
  // 3) equal caps (?)
  // 4) losingCaps (?)
  // 5) all other, sorted by history heuristic

  int movesSearched = 0;
  Move lastMove = board.lastMove();
  bool seenAll = false;
  std::vector<Move> allMoves;

  NodeType childNodeType = All;

  bool nullWindow = false;
  bool foundMove = refMove.notNull();
  bool raisedAlpha = false;

  Move firstMove;

  while (true) {
    if (!seenAll) {
      Move mv = board.nextMove(movegen, sbuffer, hasGenSpecial);
      if (mv.isNull()) {
        seenAll = true;
        allMoves.clear();
        allMoves.reserve(hashMoves.size() + posCaptures.size() +
                         eqCaptures.size() + heuristics.size() + other.size());
        allMoves.insert(allMoves.end(), other.begin(), other.end());
        allMoves.insert(allMoves.end(), eqCaptures.begin(), eqCaptures.end());
        allMoves.insert(allMoves.end(), heuristics.begin(), heuristics.end());
        allMoves.insert(allMoves.end(), posCaptures.begin(), posCaptures.end());
        allMoves.insert(allMoves.end(), hashMoves.begin(), hashMoves.end());
      } else {
        // sort move into correct bucket
        u64 dest = mv.getDest();
        if (mv == refMove) {
          hashMoves.push_back(mv);
        } else if (dest & occ) {
          int see = board.see(mv);
          if (see > 0) {
            posCaptures.push_back(mv);
          } else if (see == 0) {
            eqCaptures.push_back(mv);
          } else {
            other.push_back(mv);
          }
        } else if (kTable.contains(mv, plyCount)) {
          heuristics.push_back(mv);
        } else if (cTable.contains(lastMove, mv, board.turn())) {
          heuristics.push_back(mv);
        } else {
          other.push_back(mv);
        }
      }
    }

    Move fmove;

    if (seenAll) { // we have seen all the moves.
      if (allMoves.size() > 0) {
        fmove = allMoves.back();
        allMoves.pop_back();
      } else {
        break;
      }
    } else {
      // grab depending on phase
      if (phase == 0) {
        if (hashMoves.size() > 0) {
          fmove = hashMoves.back();
          hashMoves.pop_back();
          phase = 1;
        } else {
          continue;
        }
      } else if (phase == 1) {
        // looking for pos captures
        if (posCaptures.size() > 0) {
          fmove = posCaptures.back();
          posCaptures.pop_back();
        } else {
          continue;
        }
      } else {
        continue;
      }
    }

    if (movesSearched == 0) {
      firstMove = fmove;
    }

    if ((futilityPrune && depth == 1) && movesSearched >= 1) {
      if ((fmove.getDest() & ~occ && fmove != refMove) &&
          (!board.isCheckingMove(fmove) && !nodeIsCheck)) {
        if (fscore + 900 < alpha) {
          // skip this move
          continue;
        }
      }
    }

    bool isCapture = fmove.getDest() & occ;
    bool isPromotion = fmove.isPromotion();
    bool isReduced = false;

    board.makeMove(fmove);

    int subdepth = depth - 1;
    if (lmr && !nodeIsCheck && !isPromotion && !board.isCheck() && !isCapture &&
        (myNodeType != PV) && depth > 2 && movesSearched > 4) {
      subdepth = depth - 2; // LMR: need to re-search
      isReduced = true;
    } else if (board.isCheck()) {
      subdepth = depth; // Check ext
    }
    int score;
    if (nullWindow) {
      score =
          -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * alpha - 1,
                                 -1 * alpha, stop, count, All);
      if (score > alpha) {
        if (isReduced) {
          subdepth = depth - 1;
        }
        score = -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * beta,
                                       -1 * alpha, stop, count, PV);
      }
    } else {
      score = -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * beta,
                                     -1 * alpha, stop, count, childNodeType);
      if (isReduced && score > alpha) {
        subdepth = depth - 1;
        score =
            -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * beta,
                                   -1 * alpha, stop, count, PV);
      }
    }

    board.unmakeMove();
    movesSearched++;

    if (stop) { // if stopped in subcall, then we want to ignore it
      return alpha;
    }

    if (score >= beta) { // our move is better than beta, so this node is cut
      node.nodeType = Cut;
      node.bestMove = fmove;
      table.insert(node, beta);

      if (fmove.getDest() & ~occ) {
        hTable.insert(fmove, board.turn(), depth);
        kTable.insert(fmove, plyCount);
        cTable.insert(board.turn(), board.lastMove(), fmove);
      }
      return beta; // fail hard
    }

    if (score > alpha) {
      nullWindow = foundMove;
      raisedAlpha = true;
      node.nodeType = PV;
      node.bestMove = fmove;
      alpha = score; // push up alpha
      childNodeType = All;
    }
  }
  if (!raisedAlpha) {
    node.nodeType = All;
  }
  table.insert(node, alpha); // store node
  if (raisedAlpha) {
    pvTable.insert(node, alpha);
  }
  return alpha;
}
