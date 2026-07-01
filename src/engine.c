#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "utility.h"

int getPieceValue(int pieceType) {
  if(pieceType == bRooks) return -500;
  if(pieceType == bKnights) return -250;
  if(pieceType == bBishops) return -300;
  if(pieceType == bQueens) return -950;
  if(pieceType == bKing) return -999999;
  if(pieceType == bPawns) return -100;
  if(pieceType == wRooks) return 500;
  if(pieceType == wKnights) return 250;
  if(pieceType == wBishops) return 300;
  if(pieceType == wQueens) return 950;
  if(pieceType == wKing) return 999999;
  if(pieceType == wPawns) return 100;
  return 0;
}

int evaluate(BoardState boardState) {
  int output = 0;

  uint64_t currentPieces;
  for(int pieceType = 0; pieceType < 12; pieceType++) {
    currentPieces = boardState.pieces[pieceType];
    while(currentPieces) {
      __builtin_ctzll(currentPieces);
      currentPieces &= currentPieces - 1;
      output += getPieceValue(pieceType);
    }
  }
  return output;
}

uint64_t *getAllMoves(BoardState boardState) {
  uint64_t allMoves[64];
  for(int square = 0; square < 64; square++) {
    allMoves[square] = getMoves(boardState, square);
  }
  return allMoves;
}

int negaMax(BoardState boardState, int depth) {
  if(depth == 0) return evaluate(boardState);

  int max = INT_MIN;
  uint64_t allMoves[64] = getAllMoves(boardState);
  int fromSquare, toSquare;
  for(fromSquare = 0; fromSquare < 64; fromSquare++) {
    while(allMoves[fromSquare]) {
      toSquare = __builtin_ctzll(allMoves[square]);
      allMoves[square] &= allMoves[square] - 1;

      move(&boardState, fromSquare, toSquare);
      score = -negaMax(boardState, depth - 1);
      if(score > max) {
        max = score;
      }
    }
  }
  return max;
}
