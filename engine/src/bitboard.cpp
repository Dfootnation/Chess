// Bitboard

#include <cstdint>
#include <iostream>

using Bitboard = uint64_t;

struct Board {
    // One bitboard for each piece type and color
    Bitboard pieceBB[2][6]; // [Color: White=0, Black=1][Piece: P, N, B, R, Q, K]
    Bitboard occupancy[3];  // White, Black, and Both
    
    // Example: Check if a square is occupied
    bool isOccupied(int square) {
        return (occupancy[2] >> square) & 1ULL;
    }
};