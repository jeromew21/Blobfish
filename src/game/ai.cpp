#include <game/ai.hpp>

TranspositionTable table;
MiniTable pvTable;
KillerTable kTable;
HistoryTable hTable;
CounterMoveTable cTable;

u64 FILES[64];

void AI::init() {
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

Move popMax(std::vector<MoveScore> &vec) {
  int m = SCORE_MIN;
  int maxI = 0;
  for (int i = 0; i < (int)vec.size(); i++) {
    if (vec[i].score > m) {
      m = vec[i].score;
      maxI = i;
    }
  }
  Move bm = vec[maxI].mv;
  vec.erase(vec.begin() + maxI);
  return bm;
}

void AI::reset() {
  kTable.clear();
  hTable.clear();
  cTable.clear();
}

int AI::materialEvaluation(Board &board) { return board.material(); }

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
    return SCORE_MAX;
  if (status == BoardStatus::BlackWin)
    return SCORE_MIN;

  int score = 0;

  // mobility
  int mcwhite = mobility(board, White) - 31;
  int mcblack = mobility(board, Black) - 31;

  // Piece-squares
  // Interpolate between 32 pieces and 12 pieces
  float pieceCount = (((float)(max(hadd(board.occupancy()), 12) - 12)) / 20.0f);
  float earlyWeight = pieceCount;
  float lateWeight = 1.0f - pieceCount;
  float pscoreEarly = 0.0f;
  float pscoreLate = 0.0f;
  for (PieceType p = 0; p < 6; p++) {
    pscoreEarly += board.pieceScoreEarlyGame[p];
    pscoreLate += board.pieceScoreLateGame[p];
  }
  float wpScore = pscoreEarly * earlyWeight + pscoreLate * lateWeight;

  pscoreEarly = 0.0f;
  pscoreLate = 0.0f;
  for (PieceType p = 6; p < 12; p++) {
    pscoreEarly += board.pieceScoreEarlyGame[p];
    pscoreLate += board.pieceScoreLateGame[p];
  }
  float bpScore = pscoreEarly * earlyWeight + pscoreLate * lateWeight;

  // combine features
  score += materialEvaluation(board);
  score += mcwhite - mcblack;
  score += (wpScore - bpScore) * 10.0f;
  score += board.kingSafety(White) * 5.0f * earlyWeight;
  score -= board.kingSafety(Black) * 5.0f * earlyWeight;

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
                  std::chrono::_V2::system_clock::time_point start,
                  std::vector<MoveScore> &prevScores) {
  // IID
  TableNode node(board, depth, PV);

  auto moves = board.legalMoves();

  int alpha = SCORE_MIN;
  int beta = SCORE_MAX;

  Move refMove;

  if (depth > 1) {
    auto found = table.find(node);
    if (found != table.end()) {
      if (found->first.depth > depth) {
        NodeType typ = found->first.nodeType;
        if (typ == All) {
          // upper bound, the exact score might be less.
          beta = found->second;
        } else if (typ == Cut) {
          // lower bound
          refMove = found->first.bestMove;
          alpha = found->second;
        }
      } else {
        // Ideally a PV-node from prior iteration
        refMove = found->first.bestMove;
      }
    }
  }

  if (depth > 1) {
    moves.clear();
    while (!prevScores.empty()) {
      moves.insert(moves.begin(), popMax(prevScores));
    }
    for (int i = 0; i < (int)moves.size(); i++) {
      if (moves[i] == refMove) {
        moves.erase(moves.begin() + i);
        moves.push_back(refMove);
        break;
      }
    }
    for (int i = 0; i < (int)moves.size(); i++) {
      if (moves[i] == prevPv) {
        moves.erase(moves.begin() + i);
        moves.push_back(prevPv);
        break;
      }
    }
  } else if (depth == 1) {
    std::vector<MoveScore> moveScores;
    for (Move mv : moves) {
      board.makeMove(mv);
      int score = -1 * quiescence(board, 0, 0, alpha, beta, stop, count, 0);
      board.unmakeMove();
      moveScores.push_back(MoveScore(mv, score));
    }
    moves.clear();
    while (!moveScores.empty()) {
      moves.insert(moves.begin(), popMax(moveScores));
    }
    for (int i = 0; i < (int)moves.size(); i++) {
      if (moves[i] == refMove) {
        moves.erase(moves.begin() + i);
        moves.push_back(refMove);
        break;
      }
    }
  }

  Move chosen = moves[moves.size() - 1]; // PV-move
  outscore = alpha;

  /*std::cout << "info string moveorder";
  for (Move &mv : moves) {
      std::cout << board.moveToAlgebraic(mv) << " ";
  }
  std::cout << "\n";
  */

  bool raisedAlpha = false;

  prevScores.clear();
  bool nullWindow = false;

  while (!moves.empty()) {
    int subtreeCount = 0;
    Move mv = moves.back();
    moves.pop_back();
    board.makeMove(mv);

    int score;
    if (nullWindow) {
      score = -1 * AI::alphaBetaNega(board, depth, 0, -1 * alpha - 1,
                                     -1 * alpha, stop, subtreeCount, All);
      if (score > alpha) {
        score = -1 * AI::alphaBetaNega(board, depth, 0, -1 * beta, -1 * alpha,
                                       stop, subtreeCount, PV);
      }
    } else {
      score = -1 * AI::alphaBetaNega(board, depth, 0, -1 * beta, -1 * alpha,
                                     stop, subtreeCount, PV);
    }
    board.unmakeMove();

    count += subtreeCount;
    prevScores.push_back(MoveScore(mv, subtreeCount));

    if (stop) {
      outscore = alpha;
      return chosen; // returns best found so far
    }                // here bc if AB call stopped, it won't be full search

    if (score > alpha) {
      raisedAlpha = true;
      nullWindow = true;
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
    sendCommand("info string ROOT FAIL-LOW");
    outscore = alpha;
    return prevPv;
  }

  return chosen;
}

void AI::sendPV(Board &board, int depth, Move pvMove, int nodeCount, int score,
                std::chrono::_V2::system_clock::time_point start) {
  std::string pv = " pv " + moveToUCIAlgebraic(pvMove);

  board.makeMove(pvMove);
  int mc = 1;

  for (int k = 0; k < depth; k++) {
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

  std::string scoreStr = " score cp " + std::to_string(score);
  if (abs(score - SCORE_MAX) < 30 || abs(score - SCORE_MIN) < 30) {
    int plies = abs(abs(score) - SCORE_MAX);
    int y = plies;
    /*
    if (y % 2 == 1) {
      y += 1;
    }
    if (score < 0) {
      y *= -1;
    }
    y = y / 2;
    */
    scoreStr = " score mate " + std::to_string(y);
    sendCommand("info string mate plies " + std::to_string(plies));
  }

  sendCommand(
      "info depth " + std::to_string(depth + 1) + scoreStr + " time " +
      std::to_string(time) + " nps " +
      std::to_string((int)((double)nodeCount / ((double)time / 1000.0))) +
      " nodes " + std::to_string(nodeCount) + pv);
}

TranspositionTable &AI::getTable() { return table; }

int AI::quiescence(Board &board, int depth, int plyCount, int alpha, int beta,
                   std::atomic<bool> &stop, int &count, int kickoff) {

  count++;

  TableNode node(board, depth, PV);
  auto found = table.find(node);
  if (found != table.end()) {
    if (found->first.depth >=
        depth + kickoff) { // searched already to a higher depth
      NodeType typ = found->first.nodeType;
      if (typ == All) {
        // upper bound, the exact score might be less.
        beta = min(beta, found->second);
      } else if (typ == Cut) {
        // lower bound
        alpha = max(alpha, found->second);
      }
      if (typ == PV || alpha >= beta) {
        // make mate in n
        int score = found->second;
        // finds mate in N from current position...so make absolute mate in N
        if (abs(score - SCORE_MAX) < 30 || abs(score - SCORE_MIN) < 30) {
          if (score < 0) {
            score += plyCount;
          } else {
            score -= plyCount;
          }
        }
        return score;
      }
    }
  }

  int baseline = AI::flippedEval(board);

  BoardStatus status = board.status();

  if (status != BoardStatus::Playing) {
    if (baseline == SCORE_MIN) {
      // we got mated
      return baseline + plyCount;
    }
    return baseline; // alpha vs baseline...
  }

  if (baseline >= beta)
    return beta; // fail hard

  bool isCheck = board.isCheck();

  if (alpha < baseline && !isCheck)
    alpha = baseline; // push alpha up to baseline

  u64 occ = board.occupancy();

  bool deltaPrune = true && hadd(occ) > 12;

  LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
  Move mv = board.nextMove(movegen); // order by MVV-LVA

  std::vector<Move> quietMoves;

  bool raisedAlpha = false;

  while (mv.notNull()) {
    if (stop) {
      return SCORE_MIN; // mitigate horizon effect like this for now
    }

    int see = -1;
    bool isCapture = mv.getDest() & occ;
    if (isCapture) {
      see = board.see(mv);
    }
    bool isPromotion = mv.isPromotion();
    bool isDeltaPrune =
        deltaPrune && isCapture &&
        (baseline + 200 + MATERIAL_TABLE[board.pieceAt(mv.getDest())] < alpha);
    bool isChecking =
        (kickoff == 0 || kickoff == 2) && board.isCheckingMove(mv);
    if (isChecking && isCapture) {
      if (see < 0 || isDeltaPrune) {
        isChecking = false;
      }
    }

    if ((!isDeltaPrune && see >= 0) || isChecking || isPromotion || isCheck) {
      board.makeMove(mv);
      int score = -1 * quiescence(board, depth, plyCount + 1, -1 * beta,
                                  -1 * alpha, stop, count, kickoff + 1);
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
    mv = board.nextMove(movegen);
  }

  if (!raisedAlpha) {
    for (Move mv : quietMoves) {
      if (stop) {
        return SCORE_MIN;
      }
      if (board.isCheckingMove(mv)) {
        board.makeMove(mv);
        int score = -1 * quiescence(board, depth, plyCount + 1, -1 * beta,
                                    -1 * alpha, stop, count, kickoff + 1);
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
    if (score == SCORE_MIN) {
      score += plyCount;
    } // would not be at score max
    return score;
  }

  if (depth <= 0) {
    int score = quiescence(board, depth, plyCount, alpha, beta, stop, count, 0);
    return score;
  }

  static int hits_;
  static int total_;
  total_ += 1;
  if (total_ % 80000 == 0) {
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
      }
      if (typ == PV || alpha >= beta) {
        // make mate in n
        int score = found->second;
        // finds mate in N from current position...so make absolute mate in N
        if (abs(score - SCORE_MAX) < 30 || abs(score - SCORE_MIN) < 30) {
          if (score < 0) {
            score += plyCount;
          } else {
            score -= plyCount;
          }
        }
        return score;
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
  if (nullmove && (!nodeIsCheck) && board.lastMove().notNull() &&
      (hadd(board.occupancy()) > 12)) {
    int r = 3;
    Move mv = Move::NullMove();
    board.makeMove(mv);
    int score = -1 * AI::alphaBetaNega(board, depth - 1 - r, plyCount + 1,
                                       -1 * beta, -1 * alpha, stop, count, All);
    // quiescence(board, plyCount, -1*beta, -1*alpha, stop, count, 0);
    board.unmakeMove();
    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      node.nodeType = Cut;
      node.bestMove = Move::NullMove();
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

  std::vector<Move> hashMoves;
  std::vector<Move> posCaptures;
  std::vector<Move> eqCaptures;
  std::vector<Move> heuristics;
  std::vector<Move> other;

  std::vector<MoveScore> otherWScore;

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
  if (myNodeType == PV) {
    childNodeType = PV;
  }

  bool nullWindow = false;
  bool foundMove = refMove.notNull(); // iid
  bool raisedAlpha = false;

  int moveCount = 0;
  Move firstMove;

  while (true) {
    if (!seenAll) {
      Move mv = board.nextMove(movegen);
      if (mv.isNull()) {
        seenAll = true;
        allMoves.clear();
        allMoves.reserve(hashMoves.size() + posCaptures.size() +
                         eqCaptures.size() + heuristics.size() + other.size());
        // history sort
        Color tn = board.turn();
        for (Move mv : other) {
          otherWScore.push_back(MoveScore(mv, hTable.get(mv, tn)));
        }
        allMoves.insert(allMoves.end(), eqCaptures.begin(), eqCaptures.end());
        allMoves.insert(allMoves.end(), heuristics.begin(), heuristics.end());
        allMoves.insert(allMoves.end(), posCaptures.begin(), posCaptures.end());
        allMoves.insert(allMoves.end(), hashMoves.begin(), hashMoves.end());
      } else {
        // sort move into correct bucket
        moveCount++;
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
      } else if (otherWScore.size() > 0) {
        fmove = popMax(otherWScore);
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

    if ((futilityPrune && depth == 1) && movesSearched >= 1) {
      if ((fmove.getDest() & ~occ && fmove != refMove) &&
          (!board.isCheckingMove(fmove) && !nodeIsCheck)) {
        if (fscore + 900 < alpha) {
          // skip this move
          continue;
        }
      }
    }

    if (movesSearched == 0) {
      firstMove = fmove;
    }

    bool isCapture = fmove.getDest() & occ;
    bool isPromotion = fmove.isPromotion();
    bool isPawnMove =
        fmove.getSrc() & (board.bitboard[W_Pawn] & board.bitboard[B_Pawn]);
    bool isReduced = false;

    board.makeMove(fmove);

    int subdepth = depth - 1;
    if (lmr && !nodeIsCheck && !isPromotion && !board.isCheck() && !isCapture &&
        (myNodeType != PV) && depth > 2 && movesSearched > 4 && !isPawnMove) {
      int half = 4 + (moveCount - 4) / 2;
      if (movesSearched > half) {
        subdepth = depth - 3;
      } else {
        subdepth = depth - 2;
      }
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
        score = -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * beta,
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
  } else if (myNodeType == PV) {
    //didn't raise alpha but was an expected PV-node: we have to insert move 1
    node.bestMove = firstMove;
    pvTable.insert(node, alpha);
  }
  return alpha;
}
