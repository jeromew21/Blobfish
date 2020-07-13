#include <iostream>
#include <string>
#include <regex>

#include <game/board.hpp>

u64 BISHOP_MOVE_CACHE[64][4]; //outputs a bitboard w/ ray
u64 ROOK_MOVE_CACHE[64][4];

u64 KNIGHT_MOVE_CACHE[64];
u64 KING_MOVE_CACHE[64];
u64 PAWN_CAPTURE_CACHE[64][2];
u64 PAWN_MOVE_CACHE[64][2];
u64 PAWN_DOUBLE_CACHE[64][2];

u64 ONE_ADJACENT_CACHE[64];

u64 CASTLE_LONG_SQUARES[2];
u64 CASTLE_SHORT_SQUARES[2];
u64 CASTLE_LONG_KING_SLIDE[2];
u64 CASTLE_SHORT_KING_SLIDE[2];
u64 CASTLE_LONG_KING_DEST[2];
u64 CASTLE_SHORT_KING_DEST[2];
u64 CASTLE_LONG_ROOK_DEST[2];
u64 CASTLE_SHORT_ROOK_DEST[2];

u64 ZOBRIST_HASHES[781];

int SIDE_TO_MOVE_HASH_POS = 64*12;
int W_LONG_HASH_POS = SIDE_TO_MOVE_HASH_POS + 1;
int W_SHORT_HASH_POS = SIDE_TO_MOVE_HASH_POS + 2;
int B_LONG_HASH_POS = SIDE_TO_MOVE_HASH_POS + 3;
int B_SHORT_HASH_POS = SIDE_TO_MOVE_HASH_POS + 4;
int EP_HASH_POS = SIDE_TO_MOVE_HASH_POS + 5;

PieceType CLASSICAL_BOARD[] = { //64 squares
    W_Rook,
    W_Knight,
    W_Bishop,
    W_Queen,
    W_King,
    W_Bishop,
    W_Knight,
    W_Rook,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    W_Pawn,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    Empty,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Pawn,
    B_Rook,
    B_Knight,
    B_Bishop,
    B_Queen,
    B_King,
    B_Bishop,
    B_Knight,
    B_Rook
};

void initializeZobrist() {
    for (int i = 0; i < 781; i++) {
        u64 hashval = 0;
        u64 bitmover = 1;
        for (int k = 0; k < 64; k++) {
            int randNum = rand100();
            if (randNum > 50) {
                hashval |= bitmover;
            }
            bitmover <<= 1;
        }
        ZOBRIST_HASHES[i] = hashval;
    }
    debugLog("Initialized zobrist hashes");
}

void populateMoveCache() {
    const int ROOK_MOVES[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    const int BISHOP_MOVES[4][2] = {{1, -1}, {1, 1}, {-1, 1}, {-1, -1}};

    const int KNIGHT_MOVES[8][2] = {{1, 2}, {1, -2}, {-1,2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1},};
    const int PAWN_CAPTURES[2] = {-1, 1};

    CASTLE_LONG_SQUARES[White] = u64FromIndex(1) | u64FromIndex(2) | u64FromIndex(3);
    CASTLE_LONG_SQUARES[Black] = u64FromIndex(57) | u64FromIndex(58) | u64FromIndex(59);
    CASTLE_SHORT_SQUARES[White] = u64FromIndex(5) | u64FromIndex(6);
    CASTLE_SHORT_SQUARES[Black] = u64FromIndex(61) | u64FromIndex(62);

    CASTLE_LONG_KING_SLIDE[White] = u64FromIndex(2) | u64FromIndex(3);
    CASTLE_LONG_KING_SLIDE[Black] = u64FromIndex(58) | u64FromIndex(59);
    CASTLE_SHORT_KING_SLIDE[White] = u64FromIndex(5) | u64FromIndex(6);
    CASTLE_SHORT_KING_SLIDE[Black] = u64FromIndex(61) | u64FromIndex(62);

    CASTLE_LONG_KING_DEST[White] = u64FromIndex(2);
    CASTLE_LONG_KING_DEST[Black] = u64FromIndex(58);
    CASTLE_SHORT_KING_DEST[White] = u64FromIndex(6);
    CASTLE_SHORT_KING_DEST[Black] = u64FromIndex(62);

    CASTLE_LONG_ROOK_DEST[White] = u64FromIndex(3);
    CASTLE_LONG_ROOK_DEST[Black] = u64FromIndex(59);
    CASTLE_SHORT_ROOK_DEST[White] = u64FromIndex(5);
    CASTLE_SHORT_ROOK_DEST[Black] = u64FromIndex(61);


    for (int i = 0; i < 64; i++) {
        u64 bitPos = u64FromIndex(i);
        int y0 = u64ToRow(bitPos);
        int x0 = u64ToCol(bitPos);

        //SPECIAL
        u64 pawnAdj = 0;
        if (inBounds(y0, x0 + 1)) {
            pawnAdj |= u64FromPair(y0, x0 + 1);
        }
        if (inBounds(y0, x0 - 1)) {
            pawnAdj |= u64FromPair(y0, x0 - 1);
        }
        ONE_ADJACENT_CACHE[i] = pawnAdj;

        //Kings
        u64 bitmap = 0;
        for (int dir = 0; dir < 4; dir++) {
            int y = y0 + ROOK_MOVES[dir][0];
            int x = x0 + ROOK_MOVES[dir][1];
            if (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
            }
            y = y0 + BISHOP_MOVES[dir][0];
            x = x0 + BISHOP_MOVES[dir][1];
            if (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
            }
        }
        KING_MOVE_CACHE[i] = bitmap;

        //White Pawns
        bitmap = 0;
        for (int dir = 0; dir < 2; dir++) {
            int y = y0 + 1;
            int x = x0 + PAWN_CAPTURES[dir];
            if (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
            }
        }
        PAWN_CAPTURE_CACHE[i][White] = bitmap;
        bitmap = 0;
        if (inBounds(y0 + 1, x0)) {
            bitmap |= u64FromPair(y0 + 1, x0);
        }
        PAWN_MOVE_CACHE[i][White] = bitmap;

        bitmap = 0;
        if (y0 == 1) {
            bitmap |= u64FromPair(y0 + 2, x0);
        }
        PAWN_DOUBLE_CACHE[i][White] = bitmap;

        //Black Pawns
        bitmap = 0;
        for (int dir = 0; dir < 2; dir++) {
            int y = y0 - 1;
            int x = x0 + PAWN_CAPTURES[dir];
            if (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
            }
        }
        PAWN_CAPTURE_CACHE[i][Black] = bitmap;
        bitmap = 0;
        if (inBounds(y0 - 1, x0)) {
            bitmap |= u64FromPair(y0 - 1, x0);
        }
        PAWN_MOVE_CACHE[i][Black] = bitmap;

        bitmap = 0;
        if (y0 == 6) {
            bitmap |= u64FromPair(y0 - 2, x0);
        }
        PAWN_DOUBLE_CACHE[i][Black] = bitmap;

        //Knights
        bitmap = 0;
        for (int dir = 0; dir < 8; dir++) {
            int y = y0 + KNIGHT_MOVES[dir][0];
            int x = x0 + KNIGHT_MOVES[dir][1];
            if (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
            }
        }
        KNIGHT_MOVE_CACHE[i] = bitmap;

        //Bishops
        for (int dir = 0; dir < 4; dir++) {
            u64 bitmap = 0;
            int dy = BISHOP_MOVES[dir][0];
            int dx = BISHOP_MOVES[dir][1];
            int y = y0 + dy;
            int x = x0 + dx;
            while (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
                y += dy;
                x += dx;
            }
            BISHOP_MOVE_CACHE[i][dir] = bitmap;
        }
        
        //Rooks
        for (int dir = 0; dir < 4; dir++) {
            u64 bitmap = 0;
            int dy = ROOK_MOVES[dir][0];
            int dx = ROOK_MOVES[dir][1];
            int y = y0 + dy;
            int x = x0 + dx;
            while (inBounds(y, x)) {
                bitmap |= u64FromPair(y, x);
                y += dy;
                x += dx;
            }
            ROOK_MOVE_CACHE[i][dir] = bitmap;
        }
    }
    debugLog("Initialized move cache");
}

int Board::material() {
    return material(White) - material(Black);
}

int Board::material(Color color) {
    int result = 0;
    int start = pieceIndexFromColor(color);
    int end = start + 6;
    for (PieceType i = start; i < end; i++) {
        result += MATERIAL_TABLE[i] * hadd(bitboard[i]);
    }
    return result;
}

void Board::_generatePseudoLegal() {
    int size = 0;
    std::array<u64, 32> arr;
    for (int i = 0; i < 12; i++) {
        pieceAttacks[i] = 0;
        pieceMoves[i] = 0;
    }
    _moverBuffer[White].clear();
    _srcBuffer[White].clear();
    _destBuffer[White].clear();
    _moverBuffer[Black].clear();
    _srcBuffer[Black].clear();
    _destBuffer[Black].clear();

    arr = bitscanAll(bitboard[W_Knight], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _knightAttacks(arr[i], White);
        pieceAttacks[W_Knight] |= attacks;
        pieceMoves[W_Knight] |= attacks;
        _moverBuffer[White].push_back(W_Knight);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(attacks);
    }
    arr = bitscanAll(bitboard[B_Knight], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _knightAttacks(arr[i], Black);
        pieceAttacks[B_Knight] |= attacks;
        pieceMoves[B_Knight] |= attacks;
        _moverBuffer[Black].push_back(B_Knight);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(attacks);
    }

    arr = bitscanAll(bitboard[W_King], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _kingAttacks(arr[i], White);
        pieceAttacks[W_King] |= attacks;
        pieceMoves[W_King] |= attacks;
        _moverBuffer[White].push_back(W_King);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(attacks);
    }
    arr = bitscanAll(bitboard[B_King], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _kingAttacks(arr[i], Black);
        pieceAttacks[B_King] |= attacks;
        pieceMoves[B_King] |= attacks;
        _moverBuffer[Black].push_back(B_King);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(attacks);
    }

    arr = bitscanAll(bitboard[W_Bishop], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _bishopAttacks(arr[i], White);
        pieceAttacks[W_Bishop] |= attacks;
        pieceMoves[W_Bishop] |= attacks;
        _moverBuffer[White].push_back(W_Bishop);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(attacks);
    }
    arr = bitscanAll(bitboard[B_Bishop], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _bishopAttacks(arr[i], Black);
        pieceAttacks[B_Bishop] |= attacks;
        pieceMoves[B_Bishop] |= attacks;
        _moverBuffer[Black].push_back(B_Bishop);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(attacks);
    }

    arr = bitscanAll(bitboard[W_Rook], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _rookAttacks(arr[i], White);
        pieceAttacks[W_Rook] |= attacks;
        pieceMoves[W_Rook] |= attacks;
        _moverBuffer[White].push_back(W_Rook);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(attacks);
    }
    arr = bitscanAll(bitboard[B_Rook], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _rookAttacks(arr[i], Black);
        pieceAttacks[B_Rook] |= attacks;
        pieceMoves[B_Rook] |= attacks;
        _moverBuffer[Black].push_back(B_Rook);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(attacks);
    }

    arr = bitscanAll(bitboard[W_Queen], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _rookAttacks(arr[i], White) | _bishopAttacks(arr[i], White);
        pieceAttacks[W_Queen] |= attacks;
        pieceMoves[W_Queen] |= attacks;
        _moverBuffer[White].push_back(W_Queen);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(attacks);
    }
    arr = bitscanAll(bitboard[B_Queen], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _rookAttacks(arr[i], Black) | _bishopAttacks(arr[i], Black);
        pieceAttacks[B_Queen] |= attacks;
        pieceMoves[B_Queen] |= attacks;
        _moverBuffer[Black].push_back(B_Queen);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(attacks);
    }

    arr = bitscanAll(bitboard[W_Pawn], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _pawnAttacks(arr[i], White);
        u64 mvs = _pawnMoves(arr[i], White) | attacks;
        pieceAttacks[W_Pawn] |= attacks;
        pieceMoves[W_Pawn] |= mvs;
        _moverBuffer[White].push_back(W_Pawn);
        _srcBuffer[White].push_back(arr[i]);
        _destBuffer[White].push_back(mvs);
    }
    arr = bitscanAll(bitboard[B_Pawn], size);
    for (int i = 0; i < size; i++) {
        u64 attacks = _pawnAttacks(arr[i], Black);
        u64 mvs = _pawnMoves(arr[i], Black) | attacks;
        pieceAttacks[B_Pawn] |= attacks;
        pieceMoves[B_Pawn] |= mvs;
        _moverBuffer[Black].push_back(B_Pawn);
        _srcBuffer[Black].push_back(arr[i]);
        _destBuffer[Black].push_back(mvs);
    }

    _pseudoStack.push_back(PseudoLegalData(
        _moverBuffer[White], _moverBuffer[Black],
        _srcBuffer[White], _srcBuffer[Black],
        _destBuffer[White], _destBuffer[Black], 
        pieceAttacks, pieceMoves));
}

int Board::mobility(Color c) {
    if (c == Black) {
        return hadd(pieceMoves[B_King] 
                  | pieceMoves[B_Queen] 
                  | pieceMoves[B_Knight] 
                  | pieceMoves[B_Bishop]
                  | pieceMoves[B_Rook]
                  | pieceMoves[B_Pawn]);
    } else {
        return hadd(pieceMoves[W_King] 
                  | pieceMoves[W_Queen] 
                  | pieceMoves[W_Knight] 
                  | pieceMoves[W_Bishop]
                  | pieceMoves[W_Rook]
                  | pieceMoves[W_Pawn]);
    }
}

u64 Board::_isUnderAttack(u64 target) {
    return _isUnderAttack(target, White) | _isUnderAttack(target, Black);
}

u64 Board::_isUnderAttack(u64 target, Color byWho) {
    if (byWho == Black) {
        return target & (pieceAttacks[B_King] 
            | pieceAttacks[B_Queen] 
            | pieceAttacks[B_Knight] 
            | pieceAttacks[B_Bishop]
            | pieceAttacks[B_Rook]
            | pieceAttacks[B_Pawn]);
    } else {
        return target & (pieceAttacks[W_King] 
            | pieceAttacks[W_Queen] 
            | pieceAttacks[W_Knight] 
            | pieceAttacks[W_Bishop]
            | pieceAttacks[W_Rook]
            | pieceAttacks[W_Pawn]);
    }
}

std::vector<PieceType> Board::_attackers(u64 target, Color byWho) {
    std::vector<PieceType> result;
    int s = pieceIndexFromColor(byWho);
    for (PieceType piece = s; piece < s + 6; piece++) {
        if (pieceAttacks[piece] & target) {
            result.push_back(piece);
        }
    }
    return result;
}

bool Board::isCheck() {
    Color color = turn();
    u64 kingBB = color == White ? bitboard[W_King] : bitboard[B_King];
    return _isUnderAttack(kingBB, flipColor(color));
}

std::vector<Move> Board::produceUncheckMoves() {
    std::vector<Move> result;
    Color color = turn();
    Color flip = flipColor(color);
    u64 occ = occupancy();
    //count the number of checks...
    PieceType myKing = color == White ? W_King : B_King;
    u64 kingBB = bitboard[myKing];
    auto attackers = _attackers(kingBB, flipColor(color));
    int checkCount = attackers.size();
    //can move away from check
    int count = 0;
    std::array<u64, 32> arr;
    arr = bitscanAll(pieceMoves[myKing], count);
    for (int k = 0; k < count; k++) {
        Move mv = Move::DefaultMove(myKing, kingBB, arr[k]);
        if (verifyLegal(mv)) {
            result.push_back(mv);
        }
    }
    if (checkCount == 1) {
        //can simply capture checking piece.
        //if not a knight, we can interpose in ray
        PieceType checker = attackers[0];
        for (int j = 0; j < (int) _moverBuffer[flip].size(); j++) {
            if (_moverBuffer[flip][j] == checker && _destBuffer[flip][j] & kingBB) {
                u64 target = _srcBuffer[flip][j]; // has to be the attacking one

                //king can capture if the target is not guarded
                if (pieceAttacks[myKing] & target) {
                    if (!_isUnderAttack(target, flip)) {
                        Move mv = Move::DefaultMove(myKing, kingBB, target);
                        bool inFlag = false;
                        for (Move &testmv : result) {
                            if (testmv == mv) {
                                inFlag = true;
                                break;
                            }
                        }
                        if (!inFlag) {
                            result.push_back(mv);
                        }
                    }
                }

                //amend target with rayset

                if (checker % 6 != W_Knight && checker % 6 != W_Pawn) {
                    for (int d = 0; d < 4; d++) {
                        u64 ray = _rookRay(kingBB, d, occ);
                        if (target & ray) {
                            target |= ray;
                            break;
                        }
                        ray = _bishopRay(kingBB, d, occ);
                        if (target & ray) {
                            target |= ray;
                            break;
                        }
                    }
                }

                //find any of our pieces that can capture it
                PieceType s = pieceIndexFromColor(color);
                for (PieceType p = s; p < s + 5; p++) {
                    for (int i = 0; i < (int) _moverBuffer[color].size(); i++) {
                        if (_moverBuffer[color][i] == p && _destBuffer[color][i] & target) {
                            for (u64 dest : bitscanAll(_destBuffer[color][i] & target)) {
                                result.push_back(Move::DefaultMove(p, _srcBuffer[color][i], dest));
                            }
                        }
                    }
                }
                
                break;
            }
        }
    } else if (checkCount == 0) {
        //not in check but we st ill want to produce at least one move to prove stalemate.
        Color c = color;
        for (int i = 0; i < (int) _moverBuffer[c].size(); i++) {
            Move mv = Move::NullMove();
            PieceType mover = _moverBuffer[c][i];
            u64 src = _srcBuffer[c][i];
            u64 destMap = _destBuffer[c][i];
            int count = 0;
            std::array<u64, 32> arr;
            arr = bitscanAll(destMap, count);
            for (int k = 0; k < count; k++) {
                u64 dest = arr[k];
                if (mover == W_Pawn || mover == B_Pawn) {
                    int row0 = u64ToRow(src);
                    int row1 = u64ToRow(dest);
                    if (abs(row0-row1) > 1) {
                        mv = Move::DoublePawnMove(mover, src, dest);
                    } else if (row1 == 0) {
                        for (PieceType prom = B_Knight; prom < B_Knight + 4; prom++) {
                            mv = Move::PromotionMove(mover, src, dest, prom);
                        }
                    } else if (row1 == 7) {
                        for (PieceType prom = W_Knight; prom < W_Knight + 4; prom++) {
                            mv = Move::PromotionMove(mover, src, dest, prom);
                        }
                    } else {
                        mv = Move::DefaultMove(mover, src, dest);
                    }
                } else {
                    mv = Move::DefaultMove(mover, src, dest);
                }
            }
            if (mv.moveType != MoveType::Null && verifyLegal(mv)) {
                result.push_back(mv);
                break;
            }
        }
    }
    
    return result;
}

PieceType Board::pieceAt(u64 space) {
    //temporary solution
    for (PieceType i = 0; i < Empty; i++) {
        if (space & bitboard[i]) {
            return i;
        }
    }
    return Empty;
}

Move Board::moveFromAlgebraic(const std::string& alg) {
    static std::regex rPawn("([a-h][1-8])(-|x)([a-h][1-8])");
    static std::regex rOther("([KQNBR][a-h][1-8])(-|x)([a-h][1-8])");
    static std::regex rPromotion("([a-h][1-8])(-|x)([a-h][1-8])([QNBR])");

    //shortcut way
    auto moves = legalMoves();
    for (Move &mv : moves) {
        if ((moveToExtAlgebraic(mv) == alg || moveToAlgebraic(mv) == alg) || moveToUCIAlgebraic(mv) == alg) {
            return mv;
        }
    }
    return Move::END();
}

std::string Board::moveToExtAlgebraic(const Move &mv) {
    if (mv.moveType == MoveType::CastleLong) {
        return "O-O-O";
    } else if (mv.moveType == MoveType::CastleShort) {
        return "O-O";
    }
    std::string result;
    if (mv.mover != W_Pawn && mv.mover != B_Pawn) {
        result += pieceToStringAlph(mv.mover);
    }
    result += squareName(mv.src);
    if (mv.dest & occupancy() || mv.moveType == MoveType::EnPassant) {
        result += "x";
    } else {
        result += "-";
    }
    result += squareName(mv.dest);
    if (mv.moveType == MoveType::Promotion) {
        result += pieceToStringAlph(mv.promotion);
    }
    return result;
}

std::string Board::moveToUCIAlgebraic(const Move &mv) {
    std::string result;
    result += squareName(mv.src);
    result += squareName(mv.dest);
    if (mv.moveType == MoveType::Promotion) {
        result += pieceToStringAlphLower(mv.promotion);
    }
    return result;
}

bool Board::canUndo() {
    return stack.canPop();
}

std::string Board::moveToAlgebraic(const Move &mv) {
    if (mv.moveType == MoveType::CastleLong) {
        return "O-O-O";
    } else if (mv.moveType == MoveType::CastleShort) {
        return "O-O";
    }
    std::string result;
    if (mv.mover != W_Pawn && mv.mover != B_Pawn) {
        result += pieceToStringAlph(mv.mover);
    }
    //DISAMBIGUATE
    if ((mv.mover == W_Rook || mv.mover == B_Rook) 
        || (mv.mover == W_Knight || mv.mover == B_Knight)
        || (mv.mover == W_Queen || mv.mover == B_Queen)
        || (mv.mover == W_Bishop || mv.mover == W_Bishop)
    ) {
        auto moves = legalMoves();
        std::string colDisambig = "";
        std::string rowDisambig = "";
        int overlapCount = 0;
        for (Move& testMove : moves) {
            if (mv.mover == testMove.mover && mv.dest == testMove.dest) {
                //if col is different, add col
                if (u64ToCol(mv.src) != u64ToCol(testMove.src)) {
                    colDisambig = FILE_NAMES[u64ToCol(mv.src)];
                }
                //if row is different, add row
                if (u64ToRow(mv.src) != u64ToRow(testMove.src)) {
                    rowDisambig = RANK_NAMES[u64ToRow(mv.src)];
                }
                overlapCount += 1;
            }
        }
        if (overlapCount > 2) {
            result += colDisambig + rowDisambig;
        } else {
            if (colDisambig.length() > 0) {
                result += colDisambig;
            } else if (rowDisambig.length() > 0) {
                result += rowDisambig;
            }
        }
    }
    if (mv.dest & occupancy() || mv.moveType == MoveType::EnPassant) {
        if (mv.mover == W_Pawn || mv.mover == B_Pawn) {
            result += FILE_NAMES[u64ToCol(mv.src)];
        }
        result += "x";
    }
    result += squareName(mv.dest);
    if (mv.moveType == MoveType::Promotion) {
        result += "=" + pieceToStringAlph(mv.promotion);
    }
    return result;
}

bool Board::isTerminal() {
    return status() != BoardStatus::Playing;
}

BoardStatus Board::status() {
    //COSTLY, try to use transposition table to avoid calling
    if (material(White) + material(Black) <= 350 || _threefoldFlag) {
        return BoardStatus::Draw;
    }
    auto movelist = produceUncheckMoves();
    if (movelist.size() == 0) {
        if (isCheck()) {
            return turn() == White ? BoardStatus::BlackWin : BoardStatus::WhiteWin;
        } else {
            return BoardStatus::Stalemate;
        }
    } 
    return BoardStatus::Playing;
}

void Board::_switchTurn() {
    _switchTurn(flipColor(turn()));
}

void Board::_switchTurn(Color t) {
    //if turn is black, then xor in
    //else, xor out
    u64 hash = ZOBRIST_HASHES[64*12];
    if (t != turn()) {
        _zobristHash ^= hash;
    }
    boardState[TURN_INDEX] = t;
}

u64 Board::zobrist() {
    return _zobristHash;
}

void Board::_removePiece(PieceType p, u64 location) {
    //location may be empty; if so, do nothing
    //XOR out hash
    if (location & bitboard[p]) {
        u64 hash = ZOBRIST_HASHES[64*p + u64ToIndex(location)];
        _zobristHash ^= hash;
    }
    bitboard[p] &= ~location;
}

void Board::_addPiece(PieceType p, u64 location) {
    //assume that location is empty
    //XOR in hash
    if (location & bitboard[p]) {

    } else {
        u64 hash = ZOBRIST_HASHES[64*p + u64ToIndex(location)];
        _zobristHash ^= hash;
    }
    bitboard[p] |= location;
}

void Board::makeMove(Move &mv) {
    if (mv.moveType != MoveType::Null) {
        //mask out mover
        _removePiece(mv.mover, mv.src);

        if (mv.moveType == MoveType::EnPassant) {
            if (mv.mover == W_Pawn) {
                u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][Black];
                _removePiece(B_Pawn, capturedPawn);
            } if (mv.mover == B_Pawn) {
                u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][White];
                _removePiece(W_Pawn, capturedPawn);
            }
        } else {
            //check if new location has a piece...
            //here we update the move object w/ capture data, so we're careful to return the modified move object.
            if (mv.destFormer != Empty) {
                _removePiece(mv.destFormer, mv.dest);
            }
        }
        
        //move to new location
        if (mv.moveType == MoveType::Promotion) {
            _addPiece(mv.promotion, mv.dest);
        } else {
            _addPiece(mv.mover, mv.dest);
        }

        if (mv.moveType == MoveType::CastleLong) {
            if (mv.mover == W_King) {
                _removePiece(W_Rook, rookStartingPositions[White][0]);
                _addPiece(W_Rook, CASTLE_LONG_ROOK_DEST[White]);
            } else {
                _removePiece(B_Rook, rookStartingPositions[Black][0]);
                _addPiece(B_Rook, CASTLE_LONG_ROOK_DEST[Black]);
            }
        } else if (mv.moveType == MoveType::CastleShort) {
            if (mv.mover == W_King) {
                _removePiece(W_Rook, rookStartingPositions[White][1]);
                _addPiece(W_Rook, CASTLE_SHORT_ROOK_DEST[White]);
            } else {
                _removePiece(B_Rook, rookStartingPositions[Black][1]);
                _addPiece(B_Rook, CASTLE_SHORT_ROOK_DEST[Black]);
            }
        }
    }

    //copy old data and move onto stack
    stack.push(boardState, mv);

    /*Update state!!!*/

    _switchTurn();
                        

    //revoke castling rights
    if (mv.mover == W_King) {
        _setCastlingPrivileges(White, 0, 0);
    } else if (mv.mover == B_King) {
        _setCastlingPrivileges(Black, 0, 0);
    }

    if ((mv.src | mv.dest) & rookStartingPositions[White][0]) {
        _setCastlingPrivileges(White, 0, boardState[W_SHORT_INDEX]);
    } else if ((mv.src | mv.dest) & rookStartingPositions[White][1]) {
        _setCastlingPrivileges(White, boardState[W_LONG_INDEX], 0);
    } else if ((mv.src | mv.dest) & rookStartingPositions[Black][0]) {
        _setCastlingPrivileges(Black, 0, boardState[B_SHORT_INDEX]);
    } else if ((mv.src | mv.dest) & rookStartingPositions[Black][1]) {
        _setCastlingPrivileges(Black, boardState[B_LONG_INDEX], 0);
    }
    

    //handle en passant
    if (mv.moveType == MoveType::PawnDouble) {
        //see if adjacent squares have pawns
        if (mv.mover == W_Pawn) {
            int destIndex = u64ToIndex(mv.dest);
            if (bitboard[B_Pawn] & ONE_ADJACENT_CACHE[destIndex]) {
                _setEpSquare(u64ToIndex(PAWN_MOVE_CACHE[destIndex][Black]));
            } else {
                _setEpSquare(-1);
            }
        } else {
            int destIndex = u64ToIndex(mv.dest);
            if (bitboard[W_Pawn] & ONE_ADJACENT_CACHE[destIndex]) {
                _setEpSquare(u64ToIndex(PAWN_MOVE_CACHE[destIndex][White]));
            } else {
                _setEpSquare(-1);
            }
        }
    } else {
        //all other moves: en passant square is nulled
        _setEpSquare(-1);
    }
    
    auto threefoldCount = _threefoldMap.find(_zobristHash);
    if (threefoldCount != _threefoldMap.end()) {
        _threefoldFlag = true;
    } else {
        _threefoldMap.insert(_zobristHash);
    }

    _generatePseudoLegal();
    _hasGeneratedMoves = false;
}

void Board::_setCastlingPrivileges(Color color, int cLong, int cShort) {
    //If different: then xor in

    if (color == White) {
        if (cLong != boardState[W_LONG_INDEX]) {
            u64 hash = ZOBRIST_HASHES[W_LONG_HASH_POS];
            _zobristHash ^= hash;
        }
        if (cShort != boardState[W_SHORT_INDEX]) {
            u64 hash = ZOBRIST_HASHES[W_SHORT_HASH_POS];
            _zobristHash ^= hash;
        }
    } else {
        if (cLong != boardState[B_LONG_INDEX]) {
            u64 hash = ZOBRIST_HASHES[B_LONG_HASH_POS];
            _zobristHash ^= hash;
        }
        if (cShort != boardState[B_SHORT_INDEX]) {
            u64 hash = ZOBRIST_HASHES[B_SHORT_HASH_POS];
            _zobristHash ^= hash;
        }
    }

    if (color == White) {
        boardState[W_LONG_INDEX] = cLong;
        boardState[W_SHORT_INDEX] = cShort;
    } else {
        boardState[B_LONG_INDEX] = cLong;
        boardState[B_SHORT_INDEX] = cShort;
    }
    //INC UPDATE
}

void Board::_setEpSquare(int sq) {
    int currentSq = boardState[EN_PASSANT_INDEX];
    if (currentSq != -1) {
        //if there is something to reset, then reset it
        int currentRow = u64ToRow(currentSq);
        u64 hash = ZOBRIST_HASHES[currentRow];
        _zobristHash ^= hash;
    }
    if (sq != -1) {
        //add new, if there is one to add in
        int newRow = u64ToRow(sq);
        u64 hash = ZOBRIST_HASHES[newRow];
        _zobristHash ^= hash;
    }
    boardState[EN_PASSANT_INDEX] = sq;
}

void Board::unmakeMove() {
    //state changer
    auto threefoldCount = _threefoldMap.find(_zobristHash);
    _threefoldFlag = false;
    if (threefoldCount != _threefoldMap.end()) {
        _threefoldMap.erase(_zobristHash);
    }

    BoardStateNode &node = stack.peek();
    Move& mv = node.mv;

    if (mv.moveType == MoveType::CastleLong || mv.moveType == MoveType::CastleShort) {
        if (mv.mover == W_King) {
            if (mv.moveType == MoveType::CastleLong) {
                _removePiece(W_Rook, CASTLE_LONG_ROOK_DEST[White]);
                _addPiece(W_Rook, rookStartingPositions[White][0]);
            } else {
                _removePiece(W_Rook, CASTLE_SHORT_ROOK_DEST[White]);
                _addPiece(W_Rook, rookStartingPositions[White][1]);
            }
        } else {
            if (mv.moveType == MoveType::CastleLong) {
                _removePiece(B_Rook, CASTLE_LONG_ROOK_DEST[Black]);
                _addPiece(B_Rook, rookStartingPositions[Black][0]);
            } else {
                _removePiece(B_Rook, CASTLE_SHORT_ROOK_DEST[Black]);
                _addPiece(B_Rook, rookStartingPositions[Black][1]);
            }
        }
    }

    if (mv.moveType != MoveType::Null) {
        //move piece to old src
        _addPiece(mv.mover, mv.src);

        //mask out dest
        _removePiece(mv.mover, mv.dest);
        if (mv.moveType == MoveType::Promotion) {
            _removePiece(mv.promotion, mv.dest);
        }

        //restore dest
        if (mv.moveType == MoveType::EnPassant) { //instead of restoring at capture location, restore one above
            if (mv.mover == W_Pawn) {
                _addPiece(B_Pawn, PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][Black]);
            } else {
                _addPiece(W_Pawn, PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][White]);
            }
        } else if (mv.destFormer != Empty) {
            _addPiece(mv.destFormer, mv.dest); //restore to capture location
        }
    }

    _switchTurn(node.data[TURN_INDEX]);
    _setEpSquare(node.data[EN_PASSANT_INDEX]);
    _setCastlingPrivileges(White, node.data[W_LONG_INDEX], node.data[W_SHORT_INDEX]);
    _setCastlingPrivileges(Black, node.data[B_LONG_INDEX], node.data[B_SHORT_INDEX]);

    stack.pop();

    _hasGeneratedMoves = false;
    _pseudoStack.pop_back(); //pop current

    PseudoLegalData pdata = _pseudoStack.back(); //obtain old
    _moverBuffer[White].assign(pdata.moverBuffer[White].begin(), pdata.moverBuffer[White].end());
    _moverBuffer[Black].assign(pdata.moverBuffer[Black].begin(), pdata.moverBuffer[Black].end());
    _srcBuffer[White].assign(pdata.srcBuffer[White].begin(), pdata.srcBuffer[White].end());
    _srcBuffer[Black].assign(pdata.srcBuffer[Black].begin(), pdata.srcBuffer[Black].end());
    _destBuffer[White].assign(pdata.destBuffer[White].begin(), pdata.destBuffer[White].end());
    _destBuffer[Black].assign(pdata.destBuffer[Black].begin(), pdata.destBuffer[Black].end());
    for (int i = 0; i < 12; i++) {
        pieceAttacks[i] = pdata.pieceAttacks[i];
        pieceMoves[i] = pdata.pieceMoves[i];
    }
}

void Board::dump() {
    dump(false);
}

std::string Board::fen() {
    std::string result = "";
    for (int i = 7; i >= 0; i--) {
        if (i < 7) {
            result += "/";
        }
        int ecounter = 0;
        for (int k = 0; k < 8; k++) {
            PieceType piece = pieceAt(u64FromPair(i, k));
            if (piece == Empty) {
                ecounter ++;
            } else {
                if (ecounter > 0) {
                    result += std::to_string(ecounter);
                }
                result += pieceToStringFen(piece);
                ecounter = 0;
            }
        }
        if (ecounter > 0) {
            result += std::to_string(ecounter);
        }
    }
    result += " " ;
    result += turn() == White ? "w" : "b";
    if (boardState[W_SHORT_INDEX] || boardState[W_LONG_INDEX] || boardState[B_SHORT_INDEX] || boardState[B_LONG_INDEX]) {
        result += " ";
        result += boardState[W_SHORT_INDEX] ? "K" : "";
        result += boardState[W_LONG_INDEX] ? "Q" : "";
        result += boardState[B_SHORT_INDEX] ? "k" : "";
        result += boardState[B_LONG_INDEX] ? "q" : "";
    } else {
        result += " -";
    } 
    result += " ";
    if (boardState[EN_PASSANT_INDEX] >= 0) {
        result += squareName(boardState[EN_PASSANT_INDEX]);
    } else {
        result += "-";
    }
    result += " 0 "; //clock
    result += std::to_string(stack.getIndex()); //num moves
    return result;
}

void Board::dump(bool debug) {
    for (int i = 7; i >= 0; i--) {
        for (int k = 0; k < 8; k++) {
            std::cout << pieceToString(pieceAt(u64FromPair(i, k)));
        }
        std::cout << "\n";
    }
    if (debug) {
        //dump64(occupancy());
        auto moves = legalMoves();
        std::cout << "Turn: " << colorToString(turn()) << "\n";
        std::cout << "Game Status: " << statusToString(status(), false) << "\n";
        std::cout << "Is Check: " << yesorno(isCheck()) << "\n";
        std::cout << "White can castle: (" << yesorno(boardState[W_LONG_INDEX]) << ", " << yesorno(boardState[W_SHORT_INDEX]) << ")\n";
        std::cout << "Black can castle: (" << yesorno(boardState[B_LONG_INDEX]) << ", " << yesorno(boardState[B_SHORT_INDEX]) << ")\n";
        std::cout << "EP square: ";
        if (boardState[EN_PASSANT_INDEX] >= 0) {
            std::cout << squareName(boardState[EN_PASSANT_INDEX]);
        } else {
            std::cout << "none";
        }
        std::cout << "\nLegal: ";
        for (Move &mv : moves) {
            std::cout << moveToExtAlgebraic(mv);
            if (mv.destFormer != Empty) {
                std::cout << " see: " << see(mv.src, mv.dest, mv.mover, mv.destFormer);
            }
            std::cout << "\n";
        }
        std::cout << "\nOut-Of-Check: ";
        for (Move &mv : produceUncheckMoves()) {
            std::cout << moveToAlgebraic(mv) << " ";
        }
        //std::cout << "\n" << fen();
        std::cout << "\n";
        std::cout << "\n";
    }
}

u64 Board::_knightAttacks(u64 index64, Color color) {
    u64 friendlies = occupancy(color);
    u64 ray = 0;
    int index = u64ToIndex(index64);
    u64 result = 0;
    ray = KNIGHT_MOVE_CACHE[index];
    result |= ray;
    result &= ~friendlies; //remove friendlies
    return result;
}

u64 Board::_pawnMoves(u64 index64, Color color) {
    u64 occupants = occupancy();
    int index = u64ToIndex(index64);
    u64 result = 0;
    u64 forward1 = PAWN_MOVE_CACHE[index][color];
    if (forward1 & ~occupants) { //if one forward is empty
        result |= forward1;
        if ((color == White && u64ToRow(index64) == 1) ||
            (color == Black && u64ToRow(index64) == 6)) {
            u64 forward2 = PAWN_DOUBLE_CACHE[index][color];
            if (!((forward1 | forward2) & occupants)) { //two forward are empty
                result |= forward2;
            }
        }
    }
    return result;
}

u64 Board::_pawnAttacks(u64 index64, Color color) {
    int index = u64ToIndex(index64);
    u64 result = 0;
    u64 enemies = occupancy(flipColor(color));
    u64 captures = PAWN_CAPTURE_CACHE[index][color];
    result |= captures & enemies; //plus en passant sqaure
    if (color == turn() && boardState[EN_PASSANT_INDEX] >= 0) {
        u64 epSquare = u64FromIndex(boardState[EN_PASSANT_INDEX]);
        result |= captures & epSquare;
    }
    return result;
}

u64 Board::_kingAttacks(u64 index64, Color color) {
    u64 friendlies = occupancy(color);
    u64 ray = 0;
    u64 result = 0;
    int index = u64ToIndex(index64);
    ray = KING_MOVE_CACHE[index];
    result |= ray;
    result &= ~friendlies; //remove friendlies
    return result;
}

u64 Board::_rookRay(u64 origin, int direction, u64 mask) {
    u64 ray = ROOK_MOVE_CACHE[u64ToIndex(origin)][direction];
    u64 overlaps = ray & mask;
    if (direction < 2) {
        if (overlaps) {
            ray &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][direction];
        }
    } else {
        if (overlaps) {
            ray &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][direction];
        }
    }
    return ray;
}

u64 Board::_bishopRay(u64 origin, int direction, u64 mask) {
    u64 ray = BISHOP_MOVE_CACHE[u64ToIndex(origin)][direction];
    u64 overlaps = ray & mask;
    if (direction < 2) {
        if (overlaps) {
            ray &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][direction];
        }
    } else {
        if (overlaps) {
            ray &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][direction];
        }
    }
    return ray;
}

u64 Board::_rookAttacks(u64 index64, Color color) {
    //given a color rook at index64
    u64 friendlies = occupancy(color);
    u64 enemies = occupancy(flipColor(color));
    u64 occupants = friendlies | enemies;
    int index = u64ToIndex(index64);
    u64 ray = 0;
    u64 overlaps = 0;
    u64 result = 0;

    ray = ROOK_MOVE_CACHE[index][0];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][0];
    }

    ray = ROOK_MOVE_CACHE[index][1];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~ROOK_MOVE_CACHE[bitscanForward(overlaps)][1];
    }

    ray = ROOK_MOVE_CACHE[index][2];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][2];
    }

    ray = ROOK_MOVE_CACHE[index][3];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~ROOK_MOVE_CACHE[bitscanReverse(overlaps)][3];
    }

    result &= ~friendlies; //remove friendlies
    return result;
}

u64 Board::_bishopAttacks(u64 index64, Color color) {
    //given a color bishop at index64
    u64 friendlies = occupancy(color);
    u64 enemies = occupancy(flipColor(color));
    u64 occupants = friendlies | enemies;
    int index = u64ToIndex(index64);
    u64 ray = 0;
    u64 result = 0;
    u64 overlaps = 0;

    ray = BISHOP_MOVE_CACHE[index][0];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][0];
    }

    ray = BISHOP_MOVE_CACHE[index][1];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~BISHOP_MOVE_CACHE[bitscanForward(overlaps)][1];
    }

    ray = BISHOP_MOVE_CACHE[index][2];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][2];
    }

    ray = BISHOP_MOVE_CACHE[index][3];
    overlaps = ray & occupants;
    result |= ray;
    if (overlaps) {
        result &= ~BISHOP_MOVE_CACHE[bitscanReverse(overlaps)][3];
    }

    result &= ~friendlies; //remove friendlies

    return result;
}

u64 Board::occupancy(Color color) {
    //cache this?
    if (color == White) {
        return bitboard[W_King] 
        | bitboard[W_Queen] 
        | bitboard[W_Knight] 
        | bitboard[W_Bishop]
        | bitboard[W_Rook]
        | bitboard[W_Pawn];
    } else {
        return bitboard[B_King] 
        | bitboard[B_Queen] 
        | bitboard[B_Knight] 
        | bitboard[B_Bishop]
        | bitboard[B_Rook]
        | bitboard[B_Pawn];
    }
}

u64 Board::occupancy() {
    return occupancy(White) | occupancy(Black);
}

PieceType Board::_leastValuablePiece(u64 sqset, Color color, u64 &outposition) {  //returns the least valuable piece of color color in sqset
    PieceType s = pieceIndexFromColor(color);
    for (PieceType p = s; p < s + 6; p++) {
        u64 mask = bitboard[p] & sqset;
        if (mask) {
            outposition = u64FromIndex(bitscanForward(mask)); // pop out a position
            return p;
        }
    }
    outposition = 0;
    return Empty;
}

u64 Board::_attackSet(u64 target, Color c) {
    u64 result = 0;
    for (int i = 0; i < _srcBuffer[c].size(); i++) {
        if (_destBuffer[c][i] & target) {
            result |= _srcBuffer[c][i];
        }
    }
    return result;
}

u64 Board::_attackSet(u64 target) {
    return _attackSet(target, White) | _attackSet(target, Black);
}

int Board::see(u64 src, u64 dest, PieceType attacker, PieceType targetPiece) {
    Color color = colorOf(attacker);
    u64 attackSet = _attackSet(dest);
    u64 usedAttackers = src;
    int scores[32];
    int depth = 0;
    scores[0] = MATERIAL_TABLE[targetPiece]; // first player is up by capturing
    color = flipColor(color);
    std::cout << "\nscores[" << depth << "] = " << scores[depth] << "\n";
    PieceType piece = attacker; // piece at dest
    while (true) {
        depth++;
        scores[depth] = MATERIAL_TABLE[piece] - scores[depth - 1]; //capture!
        std::cout << "scores[" << depth << "] = " << scores[depth] << "\n";

        if (max(-1*scores[depth - 1], scores[depth]) < 0) break;

        //add x-ray or conditional attackers to attack set
        //add pawns
        for (u64 loc : bitscanAll(bitboard[W_Pawn])) {
            if (PAWN_CAPTURE_CACHE[u64ToIndex(loc)][White] & dest) {
                attackSet |= loc;
            }
        }
        for (u64 loc : bitscanAll(bitboard[B_Pawn])) {
            if (PAWN_CAPTURE_CACHE[u64ToIndex(loc)][Black] & dest) {
                attackSet |= loc;
            }
        }

        u64 attPos = 0;
        attackSet = attackSet & ~usedAttackers; // remove attacker from attack set

        piece = _leastValuablePiece(attackSet, color, attPos);
        usedAttackers |= attPos;
        if (piece == Empty) {
            break;
        }
        color = flipColor(color);
    }
    while (--depth) {
        scores[depth - 1] = -1*max(-1*scores[depth - 1], scores[depth]);
    }
    return scores[0];
    //static exch eval for move ordering and quiescience pruning
}

bool Board::verifyLegal(Move &mv) {
    bool legal = true;
    Color myColor = turn();
    PieceType myKing = myColor == White ? W_King : B_King;
    //check that king is not moving into check
    //1. Edit bitboards
    //mask out mover
    bitboard[mv.mover] &= ~mv.src;

    if (mv.moveType == MoveType::EnPassant) {
        if (mv.mover == W_Pawn) {
            u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][Black];
            bitboard[B_Pawn] &= ~capturedPawn;
        } if (mv.mover == B_Pawn) {
            u64 capturedPawn = PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][White];
            bitboard[W_Pawn] &= ~capturedPawn;
        }
    } else {
        if (mv.destFormer != Empty) {
            bitboard[mv.destFormer] &= ~mv.dest;
        }
    }
    
    //move to new location
    if (mv.moveType == MoveType::Promotion) {
        bitboard[mv.promotion] |= mv.dest;
    } else {
        bitboard[mv.mover] |= mv.dest;
    }


    //2. Check if king is in line of fire


    //ray out, knight out, king out, pawn out
    PieceType offset = 0;
    if (myColor == White) {
        offset = 6;
    }
    PieceType eknight = W_Knight + offset;
    PieceType eking = W_King + offset;
    PieceType erook = W_Rook + offset;
    PieceType ebishop = W_Bishop + offset;
    PieceType equeen = W_Queen + offset;
    PieceType epawn = W_Pawn + offset;

    u64 kingBB = bitboard[myKing];
    int kingIndex = u64ToIndex(kingBB);
    u64 occ = occupancy();

    if (PAWN_CAPTURE_CACHE[kingIndex][myColor] & bitboard[epawn]
     || (KNIGHT_MOVE_CACHE[kingIndex] & bitboard[eknight] || KING_MOVE_CACHE[kingIndex] & bitboard[eking])) {
        legal = false;
    }

    for (int d = 0; d < 4; d++) {
        u64 ray;
        ray = _bishopRay(kingBB, d, occ);
        if ((bitboard[ebishop] | bitboard[equeen]) & ray) {
            legal = false;
        }
        ray = _rookRay(kingBB, d, occ);
        if ((bitboard[erook] | bitboard[equeen]) & ray) {
            legal = false;
        }
    }

    //3. Revert bitboards

    //move piece to old src
    bitboard[mv.mover] |= mv.src;

    //mask out dest
    bitboard[mv.mover] &= ~mv.dest;
    if (mv.moveType == MoveType::Promotion) {
        bitboard[mv.promotion] &= ~mv.dest;
    }

    //restore dest
    if (mv.moveType == MoveType::EnPassant) { //instead of restoring at capture location, restore one above
        if (mv.mover == W_Pawn) {
            bitboard[B_Pawn] |= PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][Black];
        } else {
            bitboard[W_Pawn] |= PAWN_MOVE_CACHE[u64ToIndex(mv.dest)][White];
        }
    } else if (mv.destFormer != Empty) {
        bitboard[mv.destFormer] |= mv.dest;
    }
    return legal;
}

void Board::generateLegalMoves() {
    _legalMovesBuffer.clear();
    _quietMoveBuffer.clear();
    _tacticalMoveBuffer.clear();
    Color c = turn();
    int size = _moverBuffer[c].size();
    u64 occ = occupancy();
    for (int i = 0; i < size; i++) {
        PieceType mover = _moverBuffer[c][i];
        u64 src = _srcBuffer[c][i];
        u64 destMap = _destBuffer[c][i];
        int count = 0;
        std::array<u64, 32> arr;
        arr = bitscanAll(destMap, count);
        for (int k = 0; k < count; k++) {
            u64 dest = arr[k];
            if (mover == W_Pawn || mover == B_Pawn) {
                int row0 = u64ToRow(src);
                int row1 = u64ToRow(dest);
                if (abs(row0-row1) > 1) {
                    Move mv = Move::DoublePawnMove(mover, src, dest);
                    _quietMoveBuffer.push_back(mv);
                } else if (row1 == 0) {
                    for (PieceType prom = B_Knight; prom < B_Knight + 4; prom++) {
                        Move mv = Move::PromotionMove(mover, src, dest, prom);
                        _tacticalMoveBuffer.push_back(mv);
                    }
                } else if (row1 == 7) {
                    for (PieceType prom = W_Knight; prom < W_Knight + 4; prom++) {
                        Move mv = Move::PromotionMove(mover, src, dest, prom);
                        _tacticalMoveBuffer.push_back(mv);
                    }
                } else {
                    if (boardState[EN_PASSANT_INDEX] >= 0 && u64FromIndex(boardState[EN_PASSANT_INDEX]) == dest) {
                        Move mv = Move::SpecialMove(MoveType::EnPassant, mover, src, dest);
                        if (mover == W_Pawn) {
                            mv.destFormer = B_Pawn;
                        } else {
                            mv.destFormer = W_Pawn;
                        }
                        _tacticalMoveBuffer.push_back(mv);
                    } else {
                        Move mv = Move::DefaultMove(mover, src, dest);
                        //check legality
                        if (occ & dest) {
                            mv.destFormer = pieceAt(dest);
                            _tacticalMoveBuffer.push_back(mv);
                        } else {
                            _quietMoveBuffer.push_back(mv);
                        }
                    }
                }
            } else {
                Move mv = Move::DefaultMove(mover, src, dest);
                //check legality
                if (occ & dest) {
                    mv.destFormer = pieceAt(dest);
                    _tacticalMoveBuffer.push_back(mv);
                } else {
                    _quietMoveBuffer.push_back(mv);
                }
            }
        }
    }
    _legalMovesBuffer.reserve(_quietMoveBuffer.size() + _tacticalMoveBuffer.size() + 2); // preallocate memory
    for (Move &mv : _quietMoveBuffer) {
        if (verifyLegal(mv)) {
            _legalMovesBuffer.push_back(mv);
        }
    }
    for (Move &mv : _tacticalMoveBuffer) {
        if (verifyLegal(mv)) {
            _legalMovesBuffer.push_back(mv);
        }
    }
    //add castling
    if (!isCheck()) {
        PieceType myKing = c == White ? W_King : B_King;
        size_t myLongIndex = B_LONG_INDEX;
        size_t myShortIndex = B_SHORT_INDEX;
        if (myKing == W_King) {
            myLongIndex = W_LONG_INDEX;
            myShortIndex = W_SHORT_INDEX;
        }
        Color opponent = flipColor(c);
        if (boardState[myLongIndex]) { //is allowed
            if (!(CASTLE_LONG_SQUARES[c] & occ)) { //in-between is empty
                if (!_isUnderAttack(CASTLE_LONG_KING_SLIDE[c], opponent)) {
                    Move mv = Move::SpecialMove(MoveType::CastleLong, myKing, bitboard[myKing], CASTLE_LONG_KING_DEST[c]);
                    _tacticalMoveBuffer.push_back(mv);
                    _legalMovesBuffer.push_back(mv);
                }
            }
        }
        if (boardState[myShortIndex]) { //is allowed
            if (!(CASTLE_SHORT_SQUARES[c] & occ)) { //in-between is empty
                if (!_isUnderAttack(CASTLE_SHORT_KING_SLIDE[c], opponent)) {
                    Move mv = Move::SpecialMove(MoveType::CastleShort, myKing, bitboard[myKing], CASTLE_SHORT_KING_DEST[c]);
                    _tacticalMoveBuffer.push_back(mv);
                    _legalMovesBuffer.push_back(mv);
                }
            }
        }
    }
    _hasGeneratedMoves = true;
}

std::vector<Move> Board::legalMoves() {
    if (_hasGeneratedMoves) {
        return _legalMovesBuffer;
    } else {
        generateLegalMoves();
        return _legalMovesBuffer;
    }
}

Color Board::turn() {
    return boardState[TURN_INDEX];
}

void Board::loadPosition(std::string fen) {
    PieceType piecelist[64];
    auto elems = tokenize(fen);

    int i = 0;
    int row = 7;
    int col = 0;
    while (row > 0 || col < 8) {
        std::string ch(1, elems[0][i]);
        if (ch == "/") {
            row -= 1;
            col = 0;
        } else {
            PieceType p = pieceFromString(ch);
            if (p == Empty) {
                int count = std::stoi(ch);
                for (int k = 0; k < count; k++) {
                    piecelist[row*8 + col] = p;
                    col += 1;
                }
            } else {
                piecelist[row*8 + col] = p;
                col += 1;
            }
        }
        i += 1;
    }

    Color t;
    if (elems[1] == "w") {
        t = White;
    } else {
        t = Black;
    }

    int wlong, wshort, blong, bshort;
    wlong = 0;
    wshort = 0;
    blong = 0;
    bshort = 0;
    for (u_int8_t k = 0; k < elems[2].size(); k++) {
        char ch = elems[2][k];
        if (ch == 'K') {
            wshort = 1;
        } else if (ch == 'Q') {
            wlong = 1;
        } else if (ch == 'k') {
            bshort = 1;
        } else if (ch == 'q') {
            blong = 1;
        }
    }

    int epIndex = -1;
    if (elems[3] != "-") {
        epIndex = indexFromSquareName(elems[3]);
    }

    loadPosition(piecelist, t, epIndex, wlong, wshort, blong, bshort);
}

void Board::loadPosition(PieceType* piecelist, Color turn, int epIndex, int wlong, int wshort, int blong, int bshort) {
    //set board, bitboards
    for (int i = 0; i < 12; i++) {
        bitboard[i] = 0;
    }
    _zobristHash = 0; //ZERO OUT

    for (int i = 0; i < 64; i++) {
        PieceType piece = piecelist[i];

        if (piece != Empty) {
            _addPiece(piece, u64FromIndex(i));
        }
    }
    boardState[TURN_INDEX] = White;

    _switchTurn(turn);
    _setEpSquare(epIndex);
    _setCastlingPrivileges(White, wlong, wshort);
    _setCastlingPrivileges(Black, blong, bshort);

    stack.clear();
    _pseudoStack.clear();

    _hasGeneratedMoves = false;
    _generatePseudoLegal();

    _threefoldFlag = false;
    _threefoldMap.clear();

    _threefoldMap.insert(_zobristHash);
}

void Board::reset() {
    rookStartingPositions[White][0] = u64FromIndex(0);
    rookStartingPositions[White][1] = u64FromIndex(7);
    rookStartingPositions[Black][0] = u64FromIndex(56);
    rookStartingPositions[Black][1] = u64FromIndex(63);
    kingStartingPositions[White] = u64FromIndex(4);
    kingStartingPositions[Black] = u64FromIndex(60);
    loadPosition(CLASSICAL_BOARD, White, -1, 1, 1, 1, 1);
}
