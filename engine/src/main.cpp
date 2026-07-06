// parseFEN

// FEN string example rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR
// [piece placement] [side to move] [castling] [en passant] [halfmove] [fullmove]

// separates ranks (rank 8 to rank 1).
//Digits represent empty squares.
//Letters represent pieces:
//Uppercase = White
//Lowercase = Black

// square = rank * 8 + file
// rank and file go 0-7

#include <iostream>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include "crow_all.h"
#include <nlohmann/json.hpp>

using U64 = uint64_t;
using json = nlohmann::json;

// position.h — your single source of truth
struct Position {
  U64 pieces[2][6];   // [color 0=W,1=B][piece 0=P,1=N,2=B,3=R,4=Q,5=K]
  U64 occupancy[3];   // [white, black, both]
  int sideToMove;     // 0=white, 1=black
  int castlingRights; // bits: KQkq = 0b1111
  int enPassantSq;    // -1 if none
  int halfmoveClock;
  int fullmoveNumber;

  Position() : pieces{}, occupancy{}, sideToMove(0),
                 castlingRights(0), enPassantSq(-1),
                 halfmoveClock(0), fullmoveNumber(1) {}

  void updateOccupancy() {
    occupancy[0] = occupancy[1] = 0;
    for (int i = 0; i < 6; i++) {
        occupancy[0] |= pieces[0][i];
        occupancy[1] |= pieces[1][i];
    }
    occupancy[2] = occupancy[0] | occupancy[1];
  }
};

struct Move {
  int from;
  int to;
  int pieceType;
  int promotion = 0; // piece type to promote to, 0 if none
};

inline int getLSB(U64 bb) {
  return __builtin_ctzll(bb); // index of lowest set bit
}

inline int popLSB(U64& bb) {
  int sq = getLSB(bb);
  bb &= bb - 1; // clear lowest set bit
  return sq;
}

inline int squareFromAlgebraic(const std::string& s) {
  int file = s[0] - 'a'; // 'a'=0 .. 'h'=7
  int rank = s[1] - '1'; // '1'=0 .. '8'=7
  return rank * 8 + file;
}

void parseFEN(const std::string& fen, Position& pos) {
  // Clear all piece data before parsing
  for (int c = 0; c < 2; c++)
    for (int p = 0; p < 6; p++)
      pos.pieces[c][p] = 0;
  pos.occupancy[0] = pos.occupancy[1] = pos.occupancy[2] = 0;
  pos.castlingRights = 0;
  pos.enPassantSq = -1;
  pos.halfmoveClock = 0;
  pos.fullmoveNumber = 1;

  int rank = 7;  // Start at rank 8
  int file = 0;

  for (char c : fen) {
    if (c == ' ') break; // Stop after piece placement section

    if (c == '/') {
      rank--;
      file = 0;
    }
    else if (isdigit(c)) {
      file += c - '0';  // Skip empty squares
    }
    else {
      int square = rank * 8 + file;
      U64 bit = 1ULL << square;

      switch (c) {
          case 'P': pos.pieces[0][0] |= bit; break;
          case 'N': pos.pieces[0][1] |= bit; break;
          case 'B': pos.pieces[0][2] |= bit; break;
          case 'R': pos.pieces[0][3] |= bit; break;
          case 'Q': pos.pieces[0][4] |= bit; break;
          case 'K': pos.pieces[0][5] |= bit; break;

          case 'p': pos.pieces[1][0] |= bit; break;
          case 'n': pos.pieces[1][1] |= bit; break;
          case 'b': pos.pieces[1][2] |= bit; break;
          case 'r': pos.pieces[1][3] |= bit; break;
          case 'q': pos.pieces[1][4] |= bit; break;
          case 'k': pos.pieces[1][5] |= bit; break;
      }

      file++;
    }
  }

  std::istringstream ss(fen);
  std::string placement, side, castling, ep, half, full;
  ss >> placement >> side >> castling >> ep >> half >> full;

  pos.sideToMove = (side == "w") ? 0 : 1;
  pos.castlingRights = 0;
  if (castling.find('K') != std::string::npos) pos.castlingRights |= 8;
  if (castling.find('Q') != std::string::npos) pos.castlingRights |= 4;
  if (castling.find('k') != std::string::npos) pos.castlingRights |= 2;
  if (castling.find('q') != std::string::npos) pos.castlingRights |= 1;
  pos.enPassantSq = (ep == "-") ? -1 : squareFromAlgebraic(ep);
  pos.halfmoveClock = std::stoi(half);
  pos.fullmoveNumber = std::stoi(full);
  pos.updateOccupancy();
}

void makeMove(Position& pos, Move move) {
  int from  = move.from;
  int to    = move.to;
  int piece = move.pieceType;
  int c     = pos.sideToMove;
  int opp   = !c;

  // Standard move
  pos.pieces[c][piece] &= ~(1ULL << from);
  pos.pieces[c][piece] |=  (1ULL << to);

  // Captures
  for (int i = 0; i < 6; i++)
    pos.pieces[opp][i] &= ~(1ULL << to);

  // En passant capture
  if (piece == 0 && to == pos.enPassantSq) {
    int capSq = (c == 0) ? to - 8 : to + 8;
    pos.pieces[opp][0] &= ~(1ULL << capSq);
  }

  // Update en passant square
  pos.enPassantSq = -1;
  if (piece == 0 && abs(to - from) == 16)
    pos.enPassantSq = (from + to) / 2;

  // Promotion — auto-queen for now
  if (piece == 0 && (to >= 56 || to <= 7)) {
    pos.pieces[c][0] &= ~(1ULL << to); // remove pawn
    pos.pieces[c][4] |=  (1ULL << to); // place queen
  }

  // Castling — move the rook
  if (piece == 5 && abs(to - from) == 2) {
    if (to == 6)  { pos.pieces[0][3] &= ~(1ULL<<7);  pos.pieces[0][3] |= (1ULL<<5);  } // WK
    if (to == 2)  { pos.pieces[0][3] &= ~(1ULL<<0);  pos.pieces[0][3] |= (1ULL<<3);  } // WQ
    if (to == 62) { pos.pieces[1][3] &= ~(1ULL<<63); pos.pieces[1][3] |= (1ULL<<61); } // BK
    if (to == 58) { pos.pieces[1][3] &= ~(1ULL<<56); pos.pieces[1][3] |= (1ULL<<59); } // BQ
  }

  // Update castling rights
  if (piece == 5) pos.castlingRights &= (c == 0) ? 0b0011 : 0b1100;
  if (piece == 3 && from == 0)  pos.castlingRights &= ~4;
  if (piece == 3 && from == 7)  pos.castlingRights &= ~8;
  if (piece == 3 && from == 56) pos.castlingRights &= ~1;
  if (piece == 3 && from == 63) pos.castlingRights &= ~2;

  // Update castling rights — rook captured
  if (to == 0)  pos.castlingRights &= ~4;
  if (to == 7)  pos.castlingRights &= ~8;
  if (to == 56) pos.castlingRights &= ~1;
  if (to == 63) pos.castlingRights &= ~2;

  pos.updateOccupancy();
  pos.sideToMove = opp;
}

U64 KnightMasks[64];

void initKnightMasks() {
  for (int sq = 0; sq < 64; sq++) {
      U64 bit = 1ULL << sq;
      U64 moves = 0;
      int r = sq / 8, f = sq % 8;

      if (r < 7 && f < 6) moves |= bit << 10;
      if (r < 6 && f < 7) moves |= bit << 17;
      if (r < 6 && f > 0) moves |= bit << 15;
      if (r < 7 && f > 1) moves |= bit << 6;
      if (r > 0 && f < 6) moves |= bit >> 6;
      if (r > 1 && f < 7) moves |= bit >> 15;
      if (r > 1 && f > 0) moves |= bit >> 17;
      if (r > 0 && f > 1) moves |= bit >> 10;

      KnightMasks[sq] = moves;
  }
}

// Directions: rook=4, bishop=4, queen=all 8
// Rook:   N=+8, S=-8, E=+1, W=-1
// Bishop: NE=+9, NW=+7, SE=-7, SW=-9

const int ROOK_DIRS[4]   = { 8, -8, 1, -1 };
const int BISHOP_DIRS[4] = { 9,  7, -7, -9 };

// File masks to detect wrapping (E/W directions cross file boundary)
const U64 FILE_A = 0x0101010101010101ULL;
const U64 FILE_H = 0x8080808080808080ULL;

U64 slidingAttacks(int sq, U64 occupied, const int* dirs, int numDirs) {
  U64 attacks = 0;
  
  for (int i = 0; i < numDirs; i++) {
    int current = sq;
    
    while (true) {
      // Prevent wrapping: E direction can't leave file H,
      // W direction can't leave file A
      if (dirs[i] ==  1 && (current % 8 == 7)) break;
      if (dirs[i] == -1 && (current % 8 == 0)) break;
      if (dirs[i] ==  9 && (current % 8 == 7)) break;
      if (dirs[i] == -9 && (current % 8 == 0)) break;
      if (dirs[i] ==  7 && (current % 8 == 0)) break;
      if (dirs[i] == -7 && (current % 8 == 7)) break;
      
      int next = current + dirs[i];
      if (next < 0 || next > 63) break; // Board edge
      
      attacks |= (1ULL << next);
      
      if (occupied & (1ULL << next)) break; // Blocked — stop ray
      
      current = next;
    }
  }
  return attacks;
}

U64 getRookAttacks(int sq, U64 occupied) {
  return slidingAttacks(sq, occupied, ROOK_DIRS, 4);
}

U64 getBishopAttacks(int sq, U64 occupied) {
  return slidingAttacks(sq, occupied, BISHOP_DIRS, 4);
}

U64 getQueenAttacks(int sq, U64 occupied) {
  return getRookAttacks(sq, occupied) | getBishopAttacks(sq, occupied);
}

U64 KingMasks[64];

void initKingMasks() {
  for (int sq = 0; sq < 64; sq++) {
    U64 bit = 1ULL << sq;
    U64 moves = 0;
    int r = sq / 8, f = sq % 8;

    if (r < 7) moves |= bit << 8;
    if (r > 0) moves |= bit >> 8;
    if (f < 7) moves |= bit << 1;
    if (f > 0) moves |= bit >> 1;
    if (r < 7 && f < 7) moves |= bit << 9;
    if (r < 7 && f > 0) moves |= bit << 7;
    if (r > 0 && f < 7) moves |= bit >> 7;
    if (r > 0 && f > 0) moves |= bit >> 9;

    KingMasks[sq] = moves;
  }
}

bool isSquareAttacked(int sq, int byColor, const Position& pos) {
  U64 occ = pos.occupancy[2];
  // Pawns
  if (byColor == 0) { // white pawns attack upward
    U64 pawnAtk = ((1ULL << sq) >> 7 & 0xFEFEFEFEFEFEFEFEULL) |
                  ((1ULL << sq) >> 9 & 0x7F7F7F7F7F7F7F7FULL);
    if (pawnAtk & pos.pieces[0][0]) return true;
  } else {
    U64 pawnAtk = ((1ULL << sq) << 9 & 0xFEFEFEFEFEFEFEFEULL) |
                  ((1ULL << sq) << 7 & 0x7F7F7F7F7F7F7F7FULL);
    if (pawnAtk & pos.pieces[1][0]) return true;
  }
  if (KnightMasks[sq]            & pos.pieces[byColor][1]) return true;
  if (getBishopAttacks(sq, occ)  & pos.pieces[byColor][2]) return true;
  if (getRookAttacks(sq, occ)    & pos.pieces[byColor][3]) return true;
  if (getQueenAttacks(sq, occ)   & pos.pieces[byColor][4]) return true;
  if (KingMasks[sq]              & pos.pieces[byColor][5]) return true;
  return false;
}

bool isInCheck(const Position& pos) {
  int kingSq = getLSB(pos.pieces[pos.sideToMove][5]);
  return isSquareAttacked(kingSq, !pos.sideToMove, pos);
}

void generateMoves(const Position& pos, std::vector<Move>& moveList) {
  int c          = pos.sideToMove;
  U64 ownPieces  = pos.occupancy[c];
  U64 occupied   = pos.occupancy[2];

  // Pawns
  if (c == 0) { // White
    U64 pawns = pos.pieces[0][0];
    U64 empty = ~occupied;

    // Single push
    U64 push1 = (pawns << 8) & empty;
    U64 tmp = push1;
    while (tmp) {
      int to = popLSB(tmp);
      moveList.push_back({to - 8, to, 0});
    }

    // Double push from rank 2 (squares 8-15)
    U64 push2 = ((push1 & 0x0000000000FF0000ULL) << 8) & empty;
    while (push2) {
      int to = popLSB(push2);
      moveList.push_back({to - 16, to, 0});
    }

    // Captures
    U64 capR = (pawns << 9) & 0xFEFEFEFEFEFEFEFEULL & pos.occupancy[1];
    U64 capL = (pawns << 7) & 0x7F7F7F7F7F7F7F7FULL & pos.occupancy[1];
    while (capR) { int to = popLSB(capR); moveList.push_back({to - 9, to, 0}); }
    while (capL) { int to = popLSB(capL); moveList.push_back({to - 7, to, 0}); }

  } else { // Black (directions reversed)
    U64 pawns = pos.pieces[1][0];
    U64 empty = ~occupied;

    U64 push1 = (pawns >> 8) & empty;
    U64 tmp = push1;
    while (tmp) {
      int to = popLSB(tmp);
      moveList.push_back({to + 8, to, 0});
    }

    U64 push2 = ((push1 & 0x0000FF0000000000ULL) >> 8) & empty;
    while (push2) {
      int to = popLSB(push2);
      moveList.push_back({to + 16, to, 0});
    }

    U64 capR = (pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL & pos.occupancy[0];
    U64 capL = (pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL & pos.occupancy[0];
    while (capR) { int to = popLSB(capR); moveList.push_back({to + 7, to, 0}); }
    while (capL) { int to = popLSB(capL); moveList.push_back({to + 9, to, 0}); }
  }

  // Knights
  U64 knightBB = pos.pieces[c][1];
  while (knightBB) {
    int from = popLSB(knightBB);
    U64 moves = KnightMasks[from] & ~ownPieces;
    while (moves) moveList.push_back({from, popLSB(moves), 1});
  }

  // Bishop
  U64 bishopBB = pos.pieces[c][2];
  while (bishopBB) {
      int from = popLSB(bishopBB);
      U64 moves = getBishopAttacks(from, occupied) & ~ownPieces;
      while (moves) moveList.push_back({from, popLSB(moves), 2});
  }

  // Rook
  U64 rookBB = pos.pieces[c][3];
  while (rookBB) {
    int from = popLSB(rookBB);
    U64 moves = getRookAttacks(from, occupied) & ~ownPieces;
    while (moves) moveList.push_back({from, popLSB(moves), 3});
  }

  // Queen
  U64 queenBB = pos.pieces[c][4];
  while (queenBB) {
    int from = popLSB(queenBB);
    U64 moves = getQueenAttacks(from, occupied) & ~ownPieces;
    while (moves) moveList.push_back({from, popLSB(moves), 4});
  }

  // King
  int kingSq = getLSB(pos.pieces[c][5]);
  U64 kingMoves = KingMasks[kingSq] & ~ownPieces;
  while (kingMoves) moveList.push_back({kingSq, popLSB(kingMoves), 5});

  // Castling
  if (c == 0) {
    if ((pos.castlingRights & 8) && !(occupied & 0x60ULL)
      && !isSquareAttacked(4, 1, pos)
      && !isSquareAttacked(5, 1, pos)
      && !isSquareAttacked(6, 1, pos))
      moveList.push_back({4, 6, 5});

    if ((pos.castlingRights & 4) && !(occupied & 0xEULL)
      && !isSquareAttacked(4, 1, pos)
      && !isSquareAttacked(3, 1, pos)
      && !isSquareAttacked(2, 1, pos))
      moveList.push_back({4, 2, 5});
    } else {
    if ((pos.castlingRights & 2) && !(occupied & 0x6000000000000000ULL)
      && !isSquareAttacked(60, 0, pos)
      && !isSquareAttacked(61, 0, pos)
      && !isSquareAttacked(62, 0, pos))
      moveList.push_back({60, 62, 5});

    if ((pos.castlingRights & 1) && !(occupied & 0xE00000000000000ULL)
      && !isSquareAttacked(60, 0, pos)
      && !isSquareAttacked(59, 0, pos)
      && !isSquareAttacked(58, 0, pos))
      moveList.push_back({60, 58, 5});
  }
}

int evaluate(const Position& pos) {
  int score = 0;
  score += __builtin_popcountll(pos.pieces[0][0]) * 100;
  score -= __builtin_popcountll(pos.pieces[1][0]) * 100;
  score += __builtin_popcountll(pos.pieces[0][1]) * 320;
  score -= __builtin_popcountll(pos.pieces[1][1]) * 320;
  score += __builtin_popcountll(pos.pieces[0][2]) * 330;
  score -= __builtin_popcountll(pos.pieces[1][2]) * 330;
  score += __builtin_popcountll(pos.pieces[0][3]) * 500;
  score -= __builtin_popcountll(pos.pieces[1][3]) * 500;
  score += __builtin_popcountll(pos.pieces[0][4]) * 900;
  score -= __builtin_popcountll(pos.pieces[1][4]) * 900;
  // Negate for black — negamax expects score from side-to-move's perspective
  return pos.sideToMove == 0 ? score : -score;
}

//alpha beta search function
int alphaBeta(Position pos, int depth, int alpha, int beta) {
  // Pass by VALUE — the copy IS your unmake.
  // Slower but correct. Optimize with undo stack later.
  if (depth == 0) return evaluate(pos);
  
  std::vector<Move> moves;
  generateMoves(pos, moves);
  if (moves.empty()) return isInCheck(pos) ? -100000 : 0;

  for (auto& move : moves) {
    Position next = pos;
    makeMove(next, move);
    next.sideToMove = !next.sideToMove; // temporarily flip back to check OUR king
    if (isInCheck(next)) continue;      // illegal — king left in check
    next.sideToMove = !next.sideToMove; // flip forward again
    int score = -alphaBeta(next, depth-1, -beta, -alpha);
    alpha = std::max(alpha, score);
    if (alpha >= beta) break;
}
  return alpha;
}

// Convert position to FEN string (needed for responses)
std::string toFEN(const Position& pos) {
  const char pieceChars[2][6] = {
      {'P','N','B','R','Q','K'},
      {'p','n','b','r','q','k'}
  };
  std::string fen = "";

  for (int rank = 7; rank >= 0; rank--) {
      int empty = 0;
      for (int file = 0; file < 8; file++) {
          int sq = rank * 8 + file;
          char found = 0;
          for (int c = 0; c < 2; c++)
            for (int p = 0; p < 6; p++)
              if (pos.pieces[c][p] & (1ULL << sq))
                found = pieceChars[c][p];
          if (found) {
            if (empty) { fen += ('0' + empty); empty = 0; }
            fen += found;
          } else empty++;
      }
      if (empty) fen += ('0' + empty);
      if (rank > 0) fen += '/';
  }

  fen += (pos.sideToMove == 0) ? " w " : " b ";

  std::string castling = "";

  if (pos.castlingRights & 8) castling += 'K';
  if (pos.castlingRights & 4) castling += 'Q';
  if (pos.castlingRights & 2) castling += 'k';
  if (pos.castlingRights & 1) castling += 'q';
  fen += castling.empty() ? "-" : castling;
  fen += " ";

  if (pos.enPassantSq == -1) fen += "-";
  else {
      fen += (char)('a' + pos.enPassantSq % 8);
      fen += (char)('1' + pos.enPassantSq / 8);
  }
  fen += " " + std::to_string(pos.halfmoveClock);
  fen += " " + std::to_string(pos.fullmoveNumber);
  return fen;
}

// Extract just the piece placement + side to move for book lookup
std::string fenKey(const Position& pos) {
std::string full = toFEN(pos);
// Take first two space-separated fields: placement + side to move
size_t first = full.find(' ');
size_t second = full.find(' ', first + 1);
return full.substr(0, second);
}

// Simple opening book keyed on FEN position string (piece placement only)
const std::unordered_map<std::string, std::string> OPENING_BOOK = {
  // White first moves
  {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w", "e2e4"},

  // After 1.e4
  {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b", "e7e5"},
  {"rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w", "g1f3"},
  {"rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b", "b8c6"},
  {"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w", "f1b5"}, // Ruy Lopez

  // After 1.e4 e5 2.Nf3 Nc6 3.Bb5
  {"r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b", "a7a6"},
  {"r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w", "b5a4"},

  // Sicilian
  {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w", "g1f3"},
  {"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b", "d7d6"},

  // French
  {"rnbqkbnr/pppp1ppp/4p3/8/4P3/8/PPPP1PPP/RNBQKBNR w", "d2d4"},
  {"rnbqkbnr/pppp1ppp/4p3/8/3PP3/8/PPP2PPP/RNBQKBNR b", "d7d5"},

  // After 1.d4
  {"rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b", "g8f6"},
  {"rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR w", "d2d4"},

  // Queen's Gambit
  {"rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR w", "c2c4"},
  {"rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b", "e7e6"},
};

std::string moveToAlgebraic(const Move& move) {
  auto toAlg = [](int sq) -> std::string {
      std::string s;
      s += (char)('a' + sq % 8);
      s += (char)('1' + sq / 8);
      return s;
  };
  return toAlg(move.from) + toAlg(move.to);
}

Move moveFromAlgebraic(const std::string& alg, const Position& pos) {
  int from = squareFromAlgebraic(alg.substr(0, 2));
  int to   = squareFromAlgebraic(alg.substr(2, 2));

  // Find which piece type is on the from square
  int pieceType = -1;
  int c = pos.sideToMove;
  for (int i = 0; i < 6; i++) {
    if (pos.pieces[c][i] & (1ULL << from)) {
      pieceType = i;
      break;
    }
  }
  return {from, to, pieceType};
}

Move getBestMove(Position& pos, int depth) {
  // Check opening book first
  std::string key = fenKey(pos);
  auto it = OPENING_BOOK.find(key);
  if (it != OPENING_BOOK.end()) {
      Move bookMove = moveFromAlgebraic(it->second, pos);
      if (bookMove.pieceType != -1) return bookMove;
  }

  std::vector<Move> moves;
  generateMoves(pos, moves);

  Move bestMove{-1, -1, -1};  // sentinel
  int bestScore = -1000000;

  for (auto& move : moves) {
    Position next = pos;
    makeMove(next, move);
    next.sideToMove = !next.sideToMove;
    if (isInCheck(next)) continue;
    next.sideToMove = !next.sideToMove;

    int score = -alphaBeta(next, depth - 1, -1000000, 1000000);
    if (score > bestScore) {
      bestScore = score;
      bestMove = move;
    }
  }
  return bestMove;  // caller should check pieceType != -1
}

int main() {
  initKnightMasks();
  initKingMasks();

  crow::App<crow::CORSHandler> app;
  Position pos;
  parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", pos);

  // CORS middleware — needed for your TS frontend to talk to this
  auto& cors = app.get_middleware<crow::CORSHandler>();
  cors.global()
      .origin("*")
      .methods("GET"_method, "POST"_method, "OPTIONS"_method);

  // POST /position
  // Body: { "fen": "rnbqkbnr/..." }
  // Sets the current board position
  CROW_ROUTE(app, "/position").methods("POST"_method)
  ([&pos](const crow::request& req) {
    auto body = json::parse(req.body);
    parseFEN(body["fen"].get<std::string>(), pos);
    return crow::response(200, json{{"ok", true}}.dump());
  });

  // POST /move
  // Body: { "move": "e2e4" }
  // Applies a player move, returns updated FEN
  CROW_ROUTE(app, "/move").methods("POST"_method)
  ([&pos](const crow::request& req) {
    auto body = json::parse(req.body);
    std::string alg = body["move"].get<std::string>();

    Move move = moveFromAlgebraic(alg, pos);
    if (move.pieceType == -1)
        return crow::response(400, json{{"error", "invalid move"}}.dump());

    makeMove(pos, move);
    return crow::response(200, json{{"fen", toFEN(pos)}}.dump());
  });

  // GET /bestmove?depth=5
  // Returns the engine's best move for the current position
  CROW_ROUTE(app, "/bestmove").methods("GET"_method)
  ([&pos](const crow::request& req) {
    int depth = 6;
    auto depthParam = req.url_params.get("depth");
    if (depthParam) depth = std::stoi(depthParam);

    Move best = getBestMove(pos, depth);
    std::string moveStr = moveToAlgebraic(best);

    // Apply the engine's move to the shared position
    makeMove(pos, best);

    return crow::response(200, json{
        {"move", moveStr},
        {"fen",  toFEN(pos)}
    }.dump());
  });

  // GET /fen
  // Returns the current board state as FEN
  CROW_ROUTE(app, "/fen")
  ([&pos]() {
    return crow::response(200, json{{"fen", toFEN(pos)}}.dump());
  });

  app.port(8080).multithreaded().run();
  return 0;
}