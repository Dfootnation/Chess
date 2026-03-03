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
#include <cstdint>

using U64 = uint64_t;

struct Bitboards {
  U64 WP = 0, WN = 0, WB = 0, WR = 0, WQ = 0, WK = 0;
  U64 BP = 0, BN = 0, BB = 0, BR = 0, BQ = 0, BK = 0;
};

void parseFEN(const std::string& fen, Bitboards& bb) {
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
          case 'P': bb.WP |= bit; break;
          case 'N': bb.WN |= bit; break;
          case 'B': bb.WB |= bit; break;
          case 'R': bb.WR |= bit; break;
          case 'Q': bb.WQ |= bit; break;
          case 'K': bb.WK |= bit; break;

          case 'p': bb.BP |= bit; break;
          case 'n': bb.BN |= bit; break;
          case 'b': bb.BB |= bit; break;
          case 'r': bb.BR |= bit; break;
          case 'q': bb.BQ |= bit; break;
          case 'k': bb.BK |= bit; break;
      }

      file++;
    }
  }
}

int main() {
    Bitboards bb;
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

    parseFEN(fen, bb);

    std::cout << "White Pawns Bitboard: " << bb.WP << std::endl;
}