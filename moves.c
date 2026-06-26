#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "moves.h"

void initializePieceMaps(uint64_t rookMaps[64][4], uint64_t bishopMaps[64][4], uint64_t knightMaps[64], uint64_t kingMaps[64], uint64_t pawnMaps[64][4]) {
  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;

    uint64_t currentRookMap = 0;
    for(int j = i + 1; j % 8 != 0; j++) {
      currentRookMap |= (uint64_t)1 << j;
    }
    rookMaps[i][0] = currentRookMap;
    currentRookMap = 0;
    for(int j = i + 8; j < 64; j += 8) {
      currentRookMap |= (uint64_t)1 << j;
    }
    rookMaps[i][1] = currentRookMap;
    currentRookMap = 0;
    for(int j = i - 1; j % 8 != 7 && j >= 0; j--) {
      currentRookMap |= (uint64_t)1 << j;
    }
    rookMaps[i][2] = currentRookMap;
    currentRookMap = 0;
    for(int j = i - 8; j >= 0; j -= 8) {
      currentRookMap |= (uint64_t)1 << j;
    }
    rookMaps[i][3] = currentRookMap;
  }

  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentKnightMap = 0;

    if(i % 8 < 7 && i / 8 < 6) currentKnightMap |= (uint64_t)1 << (i + 17);
    if(i % 8 < 7 && i / 8 > 1) currentKnightMap |= (uint64_t)1 << (i - 15);
    if(i % 8 > 0 && i / 8 < 6) currentKnightMap |= (uint64_t)1 << (i + 15);
    if(i % 8 > 0 && i / 8 > 1) currentKnightMap |= (uint64_t)1 << (i - 17);
    if(i % 8 < 6 && i / 8 < 7) currentKnightMap |= (uint64_t)1 << (i + 10);
    if(i % 8 > 1 && i / 8 < 7) currentKnightMap |= (uint64_t)1 << (i + 6);
    if(i % 8 < 6 && i / 8 > 0) currentKnightMap |= (uint64_t)1 << (i - 6);
    if(i % 8 > 1 && i / 8 > 0) currentKnightMap |= (uint64_t)1 << (i - 10);

    knightMaps[i] = currentKnightMap;
  }

  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;

    uint64_t currentBishopMap = 0;
    for(int j = i + 9; j < 64 && j % 8 != 0; j += 9) {
      currentBishopMap |= (uint64_t)1 << j;
    }
    bishopMaps[i][0] = currentBishopMap;
    currentBishopMap = 0;
    for(int j = i + 7; j < 64 && j % 8 != 7; j += 7) {
      currentBishopMap |= (uint64_t)1 << j;
    }
    bishopMaps[i][1] = currentBishopMap;
    currentBishopMap = 0;
    for(int j = i - 7; j >= 0 && j % 8 != 0; j -= 7) {
      currentBishopMap |= (uint64_t)1 << j;
    }
    bishopMaps[i][2] = currentBishopMap;
    currentBishopMap = 0;
    for(int j = i - 9; j >= 0 && j % 8 < 7; j -= 9) {
      currentBishopMap |= (uint64_t)1 << j;
    }
    bishopMaps[i][3] = currentBishopMap;
    currentBishopMap = 0;
  }

  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentKingMap = 0;

    if(i % 8 < 7) currentKingMap |= (uint64_t)1 << (i + 1);
    if(i % 8 > 0) currentKingMap |= (uint64_t)1 << (i - 1);
    if(i / 8 < 7) currentKingMap |= (uint64_t)1 << (i + 8);
    if(i / 8 > 0) currentKingMap |= (uint64_t)1 << (i - 8);
    if(i % 8 < 7 && i / 8 < 7) currentKingMap |= (uint64_t)1 << (i + 9);
    if(i % 8 > 0 && i / 8 < 7) currentKingMap |= (uint64_t)1 << (i + 7);
    if(i % 8 < 7 && i / 8 > 0) currentKingMap |= (uint64_t)1 << (i - 7);
    if(i % 8 > 0 && i / 8 > 0) currentKingMap |= (uint64_t)1 << (i - 9);

    kingMaps[i] = currentKingMap;
  }

  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentPawnMap = 0;

    if(i % 8 < 7 && i / 8 < 7) currentPawnMap |= (uint64_t)1 << (i + 9);
    if(i % 8 > 0 && i / 8 < 7) currentPawnMap |= (uint64_t)1 << (i + 7);

    pawnMaps[i][0] = currentPawnMap;
  }
  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentPawnMap = 0;

    if(i % 8 < 7 && i / 8 > 0) currentPawnMap |= (uint64_t)1 << (i - 7);
    if(i % 8 > 0 && i / 8 > 0) currentPawnMap |= (uint64_t)1 << (i - 9);

    pawnMaps[i][1] = currentPawnMap;
  }
  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentPawnMap = 0;
    if(i / 8 == 1) {
      currentPawnMap |= (uint64_t)1 << (i + 16);
    }
    currentPawnMap |= (uint64_t)1 << (i + 8);
    pawnMaps[i][2] |= currentPawnMap;
  }
  for(int i = 0; i < 64; i++) {
    uint64_t currentBit = (uint64_t)1 << i;
    uint64_t currentPawnMap = 0;
    if(i / 8 == 6) {
      currentPawnMap |= (uint64_t)1 << (i - 16);
    }
    currentPawnMap |= (uint64_t)1 << (i - 8);
    pawnMaps[i][3] |= currentPawnMap;
  }
}
