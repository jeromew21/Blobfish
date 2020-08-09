#include <game/ai.hpp>

TranspositionTable<4194304> table;
MiniTable<16384> pvTable;
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

Move popMin(std::vector<MoveScore> &vec) {
  int m = SCORE_MAX;
  int minI = 0;
  for (int i = 0; i < (int)vec.size(); i++) {
    if (vec[i].score < m) {
      m = vec[i].score;
      minI = i;
    }
  }
  Move bm = vec[minI].mv;
  vec.erase(vec.begin() + minI);
  return bm;
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
  int mcwhite = board.mobility(White) - 31;
  int mcblack = board.mobility(Black) - 31;

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
  score += board.tropism(board.bitboard[W_King], Black) * 0.03f * earlyWeight;
  score -= board.tropism(board.bitboard[B_King], White) * 0.03f * earlyWeight;
  return score;
}

int AI::flippedEval(Board &board) {
  if (board.turn() == White) {
    return AI::evaluation(board);
  } else {
    return -1 * AI::evaluation(board);
  }
}

Move AI::rootMove(Board &board, int depth, std::atomic<bool> &stop,
                  int &outscore, Move prevPv, int &count,
                  std::chrono::_V2::system_clock::time_point start,
                  std::vector<MoveScore> &prevScores) {

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
        } else if (typ == PV) {
          refMove = found->first.bestMove;
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

  Move chosen = moves[moves.size() - 1]; // PV-move
  outscore = alpha;

  /*std::cout << "info string moveorder";
  for (Move &mv : moves) {
      std::cout << board.moveToAlgebraic(mv) << " ";
  }
  std::cout << "\n";
  */

  prevScores.clear();
  bool nullWindow = false;
  bool raisedAlpha = false;

  while (!moves.empty()) {
    int subtreeCount = 0;
    Move mv = moves.back();
    moves.pop_back();
    board.makeMove(mv);

    int score;
    if (nullWindow) {
      score = -1 * AI::zeroWindowSearch(board, depth, 0, -1 * alpha, stop,
                                        subtreeCount, All);
      if (score > alpha) {
        score = -1 * AI::alphaBetaSearch(board, depth, 0, -1 * beta, -1 * alpha,
                                         stop, subtreeCount, PV, true);
      }
    } else {
      score = -1 * AI::alphaBetaSearch(board, depth, 0, -1 * beta, -1 * alpha,
                                       stop, subtreeCount, PV, true);
      if (refMove.notNull()) {
        nullWindow = true;
      }
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
      sendPV(board, depth, chosen, count, alpha, start);
    }
  }
  if (!raisedAlpha) {
    sendCommand("info string root fail-low");
    outscore = alpha;
    return prevPv;
  }

  return chosen;
}

void AI::sendPV(Board &board, int depth, Move pvMove, int nodeCount, int score,
                std::chrono::_V2::system_clock::time_point start) {
  std::string pv = " pv " + moveToUCIAlgebraic(pvMove);
  board.makeMove(pvMove);
  MiniTableBucket *search = pvTable.find(board.zobrist());
  if (search != pvTable.end()) {
    for (int k = 0; k < search->depth; k++) {
      Move mv = search->seq[k];
      // if (mv.isNull()) break;
      pv += " " + moveToUCIAlgebraic(mv);
    }
  }
  board.unmakeMove();

  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      stop - start); // or milliseconds
  int time = duration.count();

  std::string scoreStr = " score cp " + std::to_string(score);
  if (abs(score - SCORE_MAX) < 30 || abs(score - SCORE_MIN) < 30) {
    int plies = abs(abs(score) - SCORE_MAX);
    int y = plies;

    if (y % 2 == 0) {
      y += 1;
    }
    y += 1;
    y = y / 2;
    if (score < 0) {
      y *= -1;
    }

    scoreStr = " score mate " + std::to_string(y);
    sendCommand("info string mate plies " + std::to_string(plies));
  } else if (depth == 0) {
    return;
  }

  sendCommand(
      "info depth " + std::to_string(max(1, depth)) + scoreStr + " time " +
      std::to_string(time) + " nps " +
      std::to_string((int)((double)nodeCount / ((double)time / 1000.0))) +
      " nodes " + std::to_string(nodeCount) + " hashfull " +
      std::to_string(table.ppm()) + pv);
}

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
        beta = min(beta, found->second);
      } else if (typ == Cut) {
        alpha = max(alpha, found->second);
      }
      if (typ == PV || alpha >= beta) {
        int score = found->second;
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
    if (stop)
      return alpha;
    int see = -1;
    bool isCapture = mv.getDest() & occ;
    if (isCapture) {
      see = board.see(mv);
    }
    bool isPromotion = mv.isPromotion();
    // bool isPawn = mv.getSrc() & (board.bitboard[W_Pawn] &
    // board.bitboard[B_Pawn]);
    bool isDeltaPrune =
        deltaPrune && isCapture &&
        (baseline + 200 + MATERIAL_TABLE[board.pieceAt(mv.getDest())] < alpha);
    bool isCheckingMove = board.isCheckingMove(mv);
    bool isChecking = (kickoff == 0 || kickoff == 2) && isCheckingMove;
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
    } else if (isCheckingMove) {
      quietMoves.push_back(mv);
    }
    mv = board.nextMove(movegen);
  }
  if (!raisedAlpha) {
    for (Move mv : quietMoves) {
      if (stop)
        return alpha;
      board.makeMove(mv);
      int score = -1 * quiescence(board, depth, plyCount + 1, -1 * beta,
                                  -1 * alpha, stop, count, kickoff + 1);
      board.unmakeMove();
      if (score >= beta)
        return beta;
      if (score > alpha)
        alpha = score;
    }
  }
  return alpha;
}

std::vector<Move> generateMovesOrdered(Board &board, Move refMove,
                                       int plyCount) {
  // order moves for non-qsearch
  u64 occ = board.occupancy();
  LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);

  std::vector<Move> hashMoves;
  std::vector<Move> posCaptures;
  std::vector<Move> eqCaptures;
  std::vector<Move> heuristics;
  std::vector<Move> negCaptures;
  std::vector<Move> other;

  std::vector<MoveScore> otherWScore;

  Move lastMove = board.lastMove();

  // priority:
  // 0) hashmove (1)
  // 1) winning caps (?)
  // 2) heuristic moves: killer and counter move (2)
  // 3) equal caps (?)
  // 4) losingCaps (?)
  // 5) all other, sorted by history heuristic

  std::vector<Move> allMoves;
  Move mv = board.nextMove(movegen);
  while (mv.notNull()) {
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
        negCaptures.push_back(mv);
      }
    } else if (kTable.contains(mv, plyCount)) {
      heuristics.push_back(mv);
    } else if (cTable.contains(lastMove, mv, board.turn())) {
      heuristics.push_back(mv);
    } else {
      other.push_back(mv);
    }
    mv = board.nextMove(movegen);
  }
  allMoves.clear();
  allMoves.reserve(hashMoves.size() + posCaptures.size() + eqCaptures.size() +
                   heuristics.size() + other.size() + negCaptures.size());
  // history sort
  Color tn = board.turn();
  allMoves.insert(allMoves.end(), negCaptures.begin(), negCaptures.end());
  for (Move mv : other) {
    otherWScore.push_back(MoveScore(mv, hTable.get(mv, tn)));
  }
  while (!otherWScore.empty()) {
    allMoves.push_back(popMin(otherWScore));
  }
  allMoves.insert(allMoves.end(), eqCaptures.begin(), eqCaptures.end());
  allMoves.insert(allMoves.end(), heuristics.begin(), heuristics.end());
  allMoves.insert(allMoves.end(), posCaptures.begin(), posCaptures.end());
  allMoves.insert(allMoves.end(), hashMoves.begin(), hashMoves.end());
  return allMoves;
}

int AI::alphaBetaSearch(Board &board, int depth, int plyCount, int alpha,
                        int beta, std::atomic<bool> &stop, int &count,
                        NodeType myNodeType, bool isSave) {
  count++;

  bool nullmove = true;
  bool futilityPrune = true;

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

  /*static int hits_;
  static int total_;
  total_ += 1;
  if (total_ % 80000 == 0) {
    sendCommand("info string hitrate " +
                std::to_string((double)hits_ / (double)total_));
  }*/

  Move refMove;

  auto found = table.find(node);
  if (found != table.end()) {
    // hits_++;
    if (found->first.depth >= depth) { // searched already to a higher depth
      NodeType typ = found->first.nodeType;
      if (typ == All) {
        beta = min(beta, found->second);
      } else if (typ == Cut) {
        refMove = found->first.bestMove;
        alpha = max(alpha, found->second);
      } else if (typ == PV) {
        refMove = found->first.bestMove;
        // reconstruct PV here
        // have to append child pv to current node
        MiniTableBucket *currentPVNode = pvTable.find(board.zobrist());
        if (currentPVNode != pvTable.end()) {
          if (currentPVNode->depth < depth) {
            board.makeMove(refMove);
            MiniTableBucket *childPVNode = pvTable.find(board.zobrist());
            board.unmakeMove();
            if (childPVNode != pvTable.end()) {
              currentPVNode->seq[0] = refMove;
              currentPVNode->depth = depth;
              for (int i = 0; i < depth - 1; i++) {
                currentPVNode->seq[i + 1] = childPVNode->seq[i];
              }
            }
          }
        }
      }
      if (typ == PV || alpha >= beta) {
        int score = found->second;
        return score;
      }
    } else {
      // Ideally a PV-node from prior iteration
      refMove = found->first.bestMove;
    }
  }

  // use IID to find best move????

  bool nodeIsCheck = board.isCheck();
  Move lastMove = board.lastMove();

  // NULL MOVE PRUNE
  int rNull = 3;
  if (nullmove && (!nodeIsCheck) && lastMove.notNull() &&
      (hadd(board.occupancy()) > 12)) {
    Move mv = Move::NullMove();
    board.makeMove(mv);
    int score =
        -1 * AI::zeroWindowSearch(board, depth - 1 - rNull, plyCount + 1,
                                  -1 * beta + 1, stop, count, All);
    board.unmakeMove();
    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      node.nodeType = Cut;
      node.bestMove = Move::NullMove();
      table.insert(node, beta);
      return beta; // fail hard
    }
  }

  int fscore = 0;
  if (depth == 1 && futilityPrune) {
    fscore = AI::flippedEval(board);
  }

  u64 occ = board.occupancy();

  bool nullWindow = false;
  bool raisedAlpha = false;

  Move firstMove;

  std::vector<Move> moves = generateMovesOrdered(board, refMove, plyCount);
  int movesSearched = 0;

  while (!moves.empty()) {
    Move fmove = moves.back();
    moves.pop_back();

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

    bool isReduced = false;

    board.makeMove(fmove);

    int subdepth = depth - 1;
    if (board.isCheck()) {
      subdepth = depth; // Check ext
    }
    int score;
    if (nullWindow) {
      score = -1 * AI::zeroWindowSearch(board, subdepth, plyCount + 1,
                                        -1 * alpha, stop, count, All);
      if (score > alpha) {
        if (isReduced) {
          subdepth = depth - 1;
        }
        score =
            -1 * AI::alphaBetaSearch(board, subdepth, plyCount + 1, -1 * beta,
                                     -1 * alpha, stop, count, PV, isSave);
      }
    } else {
      score = -1 * AI::alphaBetaSearch(board, subdepth, plyCount + 1, -1 * beta,
                                       -1 * alpha, stop, count, PV, isSave);
      if (refMove.notNull()) {
        nullWindow = true;
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
        cTable.insert(board.turn(), lastMove, fmove);
      }
      return beta; // fail hard
    }

    if (score > alpha) {
      nullWindow = true;
      raisedAlpha = true;
      node.nodeType = PV;
      node.bestMove = fmove;
      alpha = score; // push up alpha
    }
  }
  if (!raisedAlpha) {
    node.nodeType = All;
    node.bestMove = firstMove;
  }
  table.insert(node, alpha); // store node
  if (isSave && raisedAlpha) {
    std::array<Move, 64> movelist;
    int mc = 0;
    for (int k = 0; k < depth; k++) {
      TableNode nodeSearch(board, depth, PV);
      auto search = table.find(nodeSearch);
      if (search != table.end()) {
        TableNode node = search->first;
        Move mv = node.bestMove;
        movelist[k] = mv;
        board.makeMove(mv);
        mc++;
      } else {
        depth = k;
        break;
      }
    }
    for (int k = 0; k < mc; k++) {
      board.unmakeMove();
    }
    pvTable.insert(node.hash, depth, &movelist);
  }
  return alpha;
}

int AI::zeroWindowSearch(Board &board, int depth, int plyCount, int beta,
                         std::atomic<bool> &stop, int &count,
                         NodeType myNodeType) {
  count++;

  int alpha = beta - 1;

  NodeType childNodeType = myNodeType == Cut ? All : Cut;

  bool nullmove = true;
  bool lmr = true;
  bool futilityPrune = true;
  bool multiCut = true;

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

  Move refMove;

  auto found = table.find(node);
  if (found != table.end()) {
    if (found->first.depth >= depth) { // searched already to a higher depth
      NodeType typ = found->first.nodeType;
      if (typ == All) {
        beta = min(beta, found->second);
      } else if (typ == Cut) {
        refMove = found->first.bestMove;
        alpha = max(alpha, found->second);
      }
      if (typ == PV || alpha >= beta) {
        int score = found->second;
        return score;
      }
    } else {
      refMove = found->first.bestMove;
    }
  }

  // Multi-Cut
  int rMc = 3;
  int cMc = 3;
  int mMc = 6;
  if (multiCut && myNodeType == Cut && depth >= rMc) {
    int cCount = 0;
    LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
    int i = 0;
    Move mv = board.nextMove(movegen);
    while (mv.notNull() && i < mMc) {
      i++;
      board.makeMove(mv);
      int score = -1 * zeroWindowSearch(board, depth - 1 - rMc, plyCount + 1,
                                        -1 * alpha, stop, count, childNodeType);
      board.unmakeMove();
      if (score >= beta) {
        cCount++;
        if (cCount == cMc) {
          return beta;
        }
      }
      mv = board.nextMove(movegen);
    }
  }

  u64 occ = board.occupancy();
  bool nodeIsCheck = board.isCheck();
  Move lastMove = board.lastMove();

  // NULL MOVE PRUNE
  int rNull = 3;
  if (nullmove && (!nodeIsCheck) && lastMove.notNull() && (hadd(occ) > 12)) {
    Move mv = Move::NullMove();
    board.makeMove(mv);
    int score =
        -1 * AI::zeroWindowSearch(board, depth - 1 - rNull, plyCount + 1,
                                  -1 * beta + 1, stop, count, childNodeType);
    board.unmakeMove();
    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      node.nodeType = Cut;
      node.bestMove = Move::NullMove();
      table.insert(node, beta);
      return beta; // fail hard
    }
  }

  int fscore = 0;
  if (depth == 1 && futilityPrune) {
    fscore = AI::flippedEval(board);
  }

  std::vector<Move> moves = generateMovesOrdered(board, refMove, plyCount);
  int movesSearched = 0;
  int moveCount = moves.size();
  Move firstMove;

  while (!moves.empty()) {
    Move fmove = moves.back();
    moves.pop_back();

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
    bool isPawnMove =
        fmove.getSrc() & (board.bitboard[W_Pawn] & board.bitboard[B_Pawn]);
    bool isReduced = false;

    board.makeMove(fmove);

    int subdepth = depth - 1;
    if (lmr && (myNodeType == All) && (!nodeIsCheck) && (!board.isCheck()) &&
        (!isCapture) && (depth > 2) && (movesSearched > 4) && (!isPawnMove)) {
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

    score = -1 * AI::zeroWindowSearch(board, subdepth, plyCount + 1, -1 * alpha,
                                      stop, count, childNodeType);

    // LMR here
    if (isReduced && score >= beta) {
      // re-search
      subdepth = depth - 1;
      score = -1 * AI::zeroWindowSearch(board, subdepth, plyCount + 1,
                                        -1 * alpha, stop, count, childNodeType);
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
        cTable.insert(board.turn(), lastMove, fmove);
      }

      return beta; // fail hard
    }
  }
  node.nodeType = All;
  table.insert(node, alpha); // store node
  return alpha;
}
