#include <game/ai.hpp>

TranspositionTable table;
MiniTable pvTable;

u64 BACK_RANK[2];

void AI::init() {
  BACK_RANK[White] = u64FromIndex(0) | u64FromIndex(1) | u64FromIndex(2) |
                     u64FromIndex(3) | u64FromIndex(4) | u64FromIndex(5) |
                     u64FromIndex(6) | u64FromIndex(7);
  BACK_RANK[Black] = u64FromIndex(56) | u64FromIndex(57) | u64FromIndex(58) |
                     u64FromIndex(59) | u64FromIndex(60) | u64FromIndex(61) |
                     u64FromIndex(62) | u64FromIndex(63);
}

int AI::materialEvaluation(Board &board) { return board.material(); }

int AI::evaluation(Board &board) { // positive good for white, negative for
                                   // black
  BoardStatus status = board.status();
  if (status == BoardStatus::Stalemate || status == BoardStatus::Draw) {
    return 0;
  }
  int score = materialEvaluation(board);
  for (int i = 0; i < (int) board.stack.getIndex(); i++) {
    Move mv = board.stack.peekAt(i);
    uint8_t mvtyp = mv.getTypeCode();
    if (mvtyp == MoveTypeCode::CastleLong || mvtyp == MoveTypeCode::CastleShort) {
      if (i % 2 == 0) {
        score += 30; // castling is worth 30 cp
      } else {
        score -= 30;
      }
    }
  }
  u64 minors = board.bitboard[W_Bishop] | board.bitboard[W_Knight];
  int backrank = hadd(minors & BACK_RANK[White]); // 0-8
  score -= backrank * 15;

  minors = board.bitboard[B_Bishop] | board.bitboard[B_Knight];
  backrank = hadd(minors & BACK_RANK[Black]); // 0-8
  score += backrank * 15;

  // mobility
  // naive solution: make a null move
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
    return -1 * AI::evaluation(board);
  }
}

Move AI::rootMove(Board &board, int depth, std::atomic<bool> &stop,
                  int &outscore, Move &prevPv, int &count,
                  std::chrono::_V2::system_clock::time_point start) {
  // IID

  TableNode node(board, depth, NodeType::PV);

  auto moves = board.legalMoves();
  orderMoves(board, moves);

  int alpha = INTMIN;
  int beta = INTMAX;

  if (depth > 0) {
    for (int i = 0; i < (int) moves.size(); i++) {
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

  while (moves.size() > 0) {
    Move mv = moves.back();
    moves.pop_back();
    board.makeMove(mv);
    int score = -1 * AI::alphaBetaNega(board, depth, 0, -1 * beta, -1 * alpha,
                                       stop, count);
    board.unmakeMove();

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

void sendPV(Board &board, int depth, Move &pvMove, int nodeCount, int score,
            std::chrono::_V2::system_clock::time_point start) {
  if (depth == 0)
    return;
  std::string pv = " pv " + board.moveToUCIAlgebraic(pvMove);

  board.makeMove(pvMove);
  int mc = 1;

  for (int k = 0; k < depth - 1; k++) {
    TableNode nodeSearch(board, depth, NodeType::PV);
    auto search = pvTable.find(nodeSearch);
    if (search != pvTable.end()) {
      TableNode node = search->first;
      Move mv = node.bestMove;
      pv += " ";
      pv += board.moveToUCIAlgebraic(mv);
      if (!mv.notNull()) {
        std::cout << score << " "<< pv << "\n";
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

int AI::quiescence(Board &board, int plyCount, int alpha, int beta,
                   std::atomic<bool> &stop, int &count, int depthLimit) {

  count++;

  int baseline = AI::flippedEval(board);
  BoardStatus status = board.status();

  if (status != BoardStatus::Playing || plyCount >= depthLimit) {
    if (status == BoardStatus::BlackWin || status == BoardStatus::WhiteWin) {
      // we got mated
      baseline += plyCount;
      return baseline;
    }
    return min(alpha, baseline); // alpha vs baseline...
  }

  /*LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial;
  Move mv = board.nextMove(movegen, sbuffer, hasGenSpecial);
  */

  if (baseline >= beta)
    return beta; // fail hard

  if (alpha < baseline)
    alpha = baseline; // push alpha up to baseline

  bool deltaPrune = false;

  u64 occ = board.occupancy();

  bool isCheck = board.isCheck();

  auto moves = board.legalMoves();

  int matSwingUpperBound = 1000;
  for (Move &mv : moves) {
    if (mv.isPromotion()) {
      matSwingUpperBound += 1000;
      break;
    }
  }

  if (baseline + matSwingUpperBound < alpha) {
    return alpha;
  }

  while (moves.size() > 0) {
    Move mv = moves.back();
    moves.pop_back();

    if (stop) {
      return INTMIN; // mitigate horizon effect, otherwise we could be in big
                     // trouble
    }
    if (mv.getDest() & occ) {
      // captures, and checks???????????
      if (deltaPrune &&
          (baseline + 200 + MATERIAL_TABLE[board.pieceAt(mv.getDest())] < alpha)) {
        continue;
      }

      int see = board.see(mv);
      if (see >= 0) {
        board.makeMove(mv);
        int score = -1 * quiescence(board, plyCount + 1, -1 * beta, -1 * alpha,
                                    stop, count, depthLimit);
        board.unmakeMove();
        if (score >= beta)
          return beta;
        if (score > alpha)
          alpha = score;
      }
    } else if (isCheck || board.isCheckingMove(mv)) {
      board.makeMove(mv);
      int score = -1 * quiescence(board, plyCount + 1, -1 * beta, -1 * alpha,
                                  stop, count, 10);
      board.unmakeMove();
      if (score >= beta)
        return beta;
      if (score > alpha)
        alpha = score;
    }
    ///mv = board.nextMove(movegen, sbuffer, hasGenSpecial);
  }
  return alpha;
}

void AI::orderMoves(Board &board, std::vector<Move> &mvs) {
  // put see > 0 captures at front first
  std::vector<Move> winningCaptures;
  std::vector<Move> other;
  std::vector<Move> equalCaptures;
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
    } else {
      other.push_back(mv);
    }
  }
  mvs.clear();
  mvs.assign(other.begin(), other.end());
  mvs.insert(mvs.end(), equalCaptures.begin(), equalCaptures.end());
  mvs.insert(mvs.end(), winningCaptures.begin(), winningCaptures.end());
}

void AI::clearPvTable() { pvTable.clear(); }

int AI::alphaBetaNega(Board &board, int depth, int plyCount, int alpha,
                      int beta, std::atomic<bool> &stop, int &count) {

  count++;

  BoardStatus status = board.status();

  if (status != BoardStatus::Playing) {
    int score = AI::flippedEval(board); // store?
    if (status == BoardStatus::BlackWin || status == BoardStatus::WhiteWin) {
      // we got mated
      score += plyCount;
    }
    return score;
  }

  if (depth <= 0) {
    int score = quiescence(board, plyCount+1, alpha, beta, stop, count, INTMAX);
    return score;
  }

  Move pvMove;
  bool foundHashMove = false;

  TableNode node(board, depth, NodeType::All);

  auto found = table.find(node);
  if (found != table.end()) {
      if (found->first.depth >= depth) { //searched already to a higher depth
          NodeType typ = found->first.nodeType;
          if (typ == NodeType::All) {
              //upper bound, the exact score might be less.
              beta = min(beta, found->second);
          } else if (typ == NodeType::Cut) {
              //lower bound
              foundHashMove = true;
              pvMove = found->first.bestMove;
              alpha = max(alpha, found->second);
          } else if (typ == NodeType::PV) {
              return found->second;
          }
          if (alpha >= beta) {
              return found->second;
          }
      }
  }

  bool nodeIsCheck = board.isCheck();

  bool nullmove = true;
  // NULL MOVE PRUNE
  // MAKE SURE NOT IN CHECK??
  if (nullmove && !nodeIsCheck) {
    Move mv = Move::NullMove();
    board.makeMove(mv);
    int score =
        -1 * AI::alphaBetaNega(board, depth - 1, plyCount + 1, -1 * beta,
                               -1 * alpha, stop, count);
    board.unmakeMove();
    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      return beta; // fail hard
    }
    if (score > alpha) {
      alpha = score; // push up alpha
    }
  }

  bool lmr = true;
  if (depth < 3) {
    lmr = false; // reduce on 3, 4, 5
  }
  int movesSearched = 0;
  bool checkExtend = nodeIsCheck && depth < 3;

  bool futilityPrune = true;

  int fscore = 0;
  if (depth == 1 && futilityPrune) {
    fscore = AI::flippedEval(board);
  }

  u64 occ = board.occupancy();

  /*
  LazyMovegen movegen(board.occupancy(board.turn()), board.attackMap);
  std::vector<Move> sbuffer;
  bool hasGenSpecial;
  Move mv = board.nextMove(movegen, sbuffer, hasGenSpecial);

  std::vector<Move> posCaptures;
  std::vector<Move> eqCaptures;
  std::vector<Move> allOther;
  int phase = 0;
  */
  auto moves = board.legalMoves();
  orderMoves(board, moves);

  //put hash move in front
  if (foundHashMove) {
    for (int i = 0; i < (int) moves.size(); i++) {
      if (moves[i] == pvMove) {
        moves.erase(moves.begin() + i);
        moves.push_back(pvMove);
        break;
      }
    }
  }

  while (moves.size() > 0) {
    //sort on the fly?
    Move fmove = moves.back();
    moves.pop_back();
    /*

    if (mv.notNull()) {
      //we are seeing mv for the first time
      if (foundHashMove && mv == pvMove) {
        fmove = mv;
        phase += 1;
      } else if (mv.getDest() & occ) {
        int see = board.see(mv);
        if (see > 0) {
          if (phase > 0) {
            //go forward
            fmove = mv;
          } else {
            posCaptures.push_back(mv);
            //still looking for hash move
            continue;
          }
        } else if (see == 0) {
          eqCaptures.push_back(mv);
          continue;
        } else {
          allOther.push_back(mv);
          continue;
        }
      } else {
        allOther.push_back(mv);
        continue;
      }
    } else {
      //out of moves. fallback
      if (posCaptures.size() > 1) {
        fmove = posCaptures.back();
        posCaptures.pop_back();
      } else if (eqCaptures.size() > 1) {
        fmove = eqCaptures.back();
        eqCaptures.pop_back();
      } else {
        fmove = allOther.back();
        allOther.pop_back();
      }
    }

    if (!fmove.notNull()) continue;
    */
    
    if ((futilityPrune && depth == 1) && movesSearched > 1) {
      if ((!(fmove.getDest() & occ) && !(fmove == pvMove)) 
      && (!board.isCheckingMove(fmove) && !nodeIsCheck)) {
        if (fscore + 900 < alpha) {
          continue;
        }
      }
    }

    board.makeMove(fmove);

    // LMR
    int subdepth = depth - 1;
    if (!nodeIsCheck && !board.isCheck()) {
      if (lmr && movesSearched > 4) {
        subdepth--;
      }
    } else if (checkExtend) {
      subdepth = depth;
    }

    int score =
        -1 * AI::alphaBetaNega(board, subdepth, plyCount + 1, -1 * beta,
                               -1 * alpha, stop, count); // best move for oppo
    board.unmakeMove();
    movesSearched++;

    if (stop) { // if stopped in subcall, then we want to ignore it
      return alpha;
    }

    if (score >= beta) { // our move is better than beta, so this node is cut
                         // off
      node.nodeType = NodeType::Cut;
      node.bestMove = fmove;
      table.insert(node, beta);
      // found a killer

      return beta; // fail hard
    }

    if (score > alpha) {
      node.nodeType = NodeType::PV;
      node.bestMove = fmove;
      alpha = score; // push up alpha
    }

    //mv = board.nextMove(movegen, sbuffer, hasGenSpecial);
  }
  table.insert(node, alpha); // store node
  if (node.nodeType == NodeType::PV) {
    pvTable.insert(node, alpha); // store node
  }
  return alpha;
}
