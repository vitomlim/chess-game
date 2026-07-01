#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "utility.h"

// auxiliary variables
uint64_t firstRank = (uint64_t)0b11111111 << 56, lastRank = (uint64_t)0b11111111;

uint64_t knightMaps[64], kingMaps[64];
uint64_t rookMaps[64][4], bishopMaps[64][4];
uint64_t pawnMaps[64][4];

void initializeGame(BoardState *pBoardState, char* FENString) {
  initializeBoard(pBoardState, FENString);
  initializePieceMaps(rookMaps, bishopMaps, knightMaps, kingMaps, pawnMaps);
}

// algorithm to calculate collisions for rooks and bishops
uint64_t getBeamInfluence(BoardState boardState, int square, uint64_t pieceMaps[64][4]) {
  uint64_t influence = 0;
  for(int i = 0; i < 2; i++) {
    uint64_t crescentCollisions = pieceMaps[square][i + 2] & (boardState.whitePieces | boardState.blackPieces);
    crescentCollisions |= crescentCollisions >> 1;
    crescentCollisions |= crescentCollisions >> 2;
    crescentCollisions |= crescentCollisions >> 4;
    crescentCollisions |= crescentCollisions >> 8;
    crescentCollisions |= crescentCollisions >> 16;
    crescentCollisions |= crescentCollisions >> 32;
    crescentCollisions = crescentCollisions >> 1;

    influence |= (~crescentCollisions & pieceMaps[square][i + 2]);

    uint64_t decrescentCollisions = pieceMaps[square][i] & (boardState.whitePieces | boardState.blackPieces);
    decrescentCollisions |= decrescentCollisions << 1;
    decrescentCollisions |= decrescentCollisions << 2;
    decrescentCollisions |= decrescentCollisions << 4;
    decrescentCollisions |= decrescentCollisions << 8;
    decrescentCollisions |= decrescentCollisions << 16;
    decrescentCollisions |= decrescentCollisions << 32;
    decrescentCollisions = decrescentCollisions << 1;
    influence |= (~decrescentCollisions & pieceMaps[square][i]);
  }
  return influence;
}

uint64_t getInfluence(BoardState boardState, int square, int pieceType) {
  if(pieceType == -1)                                         return 0;
  else if(pieceType == bRooks || pieceType == wRooks)         return getBeamInfluence(boardState, square, rookMaps);
  else if(pieceType == bKnights || pieceType == wKnights)     return knightMaps[square];
  else if(pieceType == bBishops || pieceType == wBishops)     return getBeamInfluence(boardState, square, bishopMaps);
  else if(pieceType == bQueens || pieceType == wQueens)       return getBeamInfluence(boardState, square, rookMaps) | getBeamInfluence(boardState, square, bishopMaps);
  else if(pieceType == bKing || pieceType == wKing)           return kingMaps[square];
  else if(pieceType == wPawns)                                return pawnMaps[square][1];
  else if(pieceType == bPawns)                                return pawnMaps[square][0];
}

void updateInfluence(BoardState *pBoardState) {
  pBoardState->whiteInfluence = 0;
  pBoardState->blackInfluence = 0;

  uint64_t currentPieces;
  int square;

  for(int blackPieceType = 0; blackPieceType < 6; blackPieceType++) {
    currentPieces = pBoardState->pieces[blackPieceType];
    while(currentPieces) {
      square = __builtin_ctzll(currentPieces);
      currentPieces &= currentPieces - 1;
      pBoardState->blackInfluence |= getInfluence(*pBoardState, square, blackPieceType);
    }
  }
  for(int whitePieceType = 6; whitePieceType < 12; whitePieceType++) {
    currentPieces = pBoardState->pieces[whitePieceType];
    while(currentPieces) {
      square = __builtin_ctzll(currentPieces);
      currentPieces &= currentPieces - 1;
      pBoardState->whiteInfluence |= getInfluence(*pBoardState, square, whitePieceType);
    }
  }
}

void move(BoardState *pBoardState, int fromSquare, int toSquare) {

  uint64_t fromBit = (uint64_t)1 << fromSquare;
  uint64_t toBit = (uint64_t)1 << toSquare;

  int movedPiece = -1;
  for(movedPiece = pBoardState->isWhiteTurn ? 6 : 0; movedPiece < pBoardState->isWhiteTurn ? 12 : 6; movedPiece++) {
    if(pBoardState->pieces[movedPiece] & fromBit) {
      break;
    }
  }

  // handling promotion move (pawn to last rank)
  // if pawn moves to last rank, the move is ended without updating current turn
  // getMoves function checks if lastMove has the same color as current turn
  // if it does, it returns 0b1111, which is used to choose the promoted piece
  if(movedPiece == wPawns && lastRank & toBit || movedPiece == bPawns && firstRank & toBit) {
    pBoardState->lastMove = fromBit | toBit;
    return;
  }

  // handling promotion choice (choice between queen, rook, knight and bishop)
  if(pBoardState->isWhiteTurn && pBoardState->lastMove & pBoardState->whitePieces) {
    fromBit = pBoardState->lastMove & pBoardState->whitePieces;
    toBit = pBoardState->lastMove & ~fromBit;
    pBoardState->pieces[wPawns] &= ~fromBit;
    if(toSquare == 0)      movedPiece = wQueens;
    else if(toSquare == 1) movedPiece = wRooks;
    else if(toSquare == 2) movedPiece = wKnights;
    else if(toSquare == 3) movedPiece = wBishops;
  }
  if(!pBoardState->isWhiteTurn && pBoardState->lastMove & pBoardState->blackPieces) {
    fromBit = pBoardState->lastMove & pBoardState->blackPieces;
    toBit = pBoardState->lastMove & ~fromBit;
    pBoardState->pieces[bPawns] &= ~fromBit;
    if(toSquare == 0)      movedPiece = bQueens;
    else if(toSquare == 1) movedPiece = bRooks;
    else if(toSquare == 2) movedPiece = bKnights;
    else if(toSquare == 3) movedPiece = bBishops;
  }


  // handle enpassant capture
  // if pawn is making capture move and there are no pieces in the target square
  if(movedPiece == bPawns && toBit & ~pBoardState->whitePieces && toBit & pawnMaps[fromSquare][0]) {
    pBoardState->pieces[wPawns] &= ~pBoardState->lastMove;
    pBoardState->whitePieces &= ~pBoardState->lastMove;
  }
  if(movedPiece == wPawns && toBit & ~pBoardState->blackPieces && toBit & pawnMaps[fromSquare][1]) {
    pBoardState->pieces[bPawns] &= ~pBoardState->lastMove;
    pBoardState->blackPieces &= ~pBoardState->lastMove;
  }

  // handling rook movement in case of castling
  if(fromSquare == 60 && toSquare == 62) {
    pBoardState->pieces[wRooks] &= ~((uint64_t)1 << 63);
    pBoardState->pieces[wRooks] |= (uint64_t)1 << 61;
    pBoardState->whitePieces &= ~((uint64_t)1 << 63);
    pBoardState->whitePieces |= (uint64_t)1 << 61;
  }
  if(fromSquare == 60 && toSquare == 58) {
    pBoardState->pieces[wRooks] &= ~((uint64_t)1 << 56);
    pBoardState->pieces[wRooks] |= (uint64_t)1 << 59;
    pBoardState->whitePieces &= ~((uint64_t)1 << 56);
    pBoardState->whitePieces |= (uint64_t)1 << 59;
  }
  if(fromSquare == 4 && toSquare == 6) {
    pBoardState->pieces[bRooks] &= ~((uint64_t)1 << 7);
    pBoardState->pieces[bRooks] |= (uint64_t)1 << 5;
    pBoardState->blackPieces &= ~((uint64_t)1 << 7);
    pBoardState->blackPieces |= (uint64_t)1 << 5;
  }
  if(fromSquare == 4 && toSquare == 2) {
    pBoardState->pieces[bRooks] &= ~((uint64_t)1 << 0);
    pBoardState->pieces[bRooks] |= (uint64_t)1 << 3;
    pBoardState->blackPieces &= ~((uint64_t)1 << 0);
    pBoardState->blackPieces |= (uint64_t)1 << 3;
  }

  // updating castling rights
  if(fromSquare == 4) {
    pBoardState->castlingRights[bQueenSide] = false;
    pBoardState->castlingRights[bKingSide] = false;
  }
  if(fromSquare == 60) {
    pBoardState->castlingRights[wQueenSide] = false;
    pBoardState->castlingRights[wKingSide] = false;
  }
  if(fromSquare == 0 || toSquare == 0) pBoardState->castlingRights[bQueenSide] = false;
  if(fromSquare == 7 || toSquare == 7) pBoardState->castlingRights[bKingSide] = false;
  if(fromSquare == 56 || toSquare == 56) pBoardState->castlingRights[wQueenSide] = false;
  if(fromSquare == 63 || toSquare == 63) pBoardState->castlingRights[wKingSide] = false;

  // updating board state
  pBoardState->pieces[movedPiece] &= ~fromBit;
  pBoardState->pieces[movedPiece] |= toBit;
  for(int capturedPiece = pBoardState->isWhiteTurn ? 0 : 6; capturedPiece < (pBoardState->isWhiteTurn ? 6 : 12); capturedPiece++) {
    if(pBoardState->pieces[capturedPiece] & toBit) {
      pBoardState->pieces[capturedPiece] &= ~toBit;
      break;
    }
  }
  if(pBoardState->isWhiteTurn) {
    pBoardState->whitePieces &= ~fromBit;
    pBoardState->whitePieces |= toBit;
    pBoardState->blackPieces &= ~toBit;
  } else {
    pBoardState->blackPieces &= ~fromBit;
    pBoardState->blackPieces |= toBit;
    pBoardState->whitePieces &= ~toBit;
  }
  pBoardState->lastMove = fromBit | toBit;
  pBoardState->isWhiteTurn = !pBoardState->isWhiteTurn;
  updateInfluence(pBoardState);
}

uint64_t getMoves(BoardState boardState, int square) {

  // handling promotion
  // move function doesn't update current turn if last move was promotion
  uint64_t alliedPieces = boardState.isWhiteTurn ? boardState.whitePieces : boardState.blackPieces;
  if(boardState.lastMove & alliedPieces) {
    return (uint64_t)0b1111;
  }

  // Moves without considering king safety
  int pieceType = -1;
  uint64_t bit = (uint64_t)1 << square;
  for(int currentPieceType = (boardState.isWhiteTurn ? 6 : 0); currentPieceType < (boardState.isWhiteTurn ? 12 : 6); currentPieceType++) {
    if(boardState.pieces[currentPieceType] & bit) {
      pieceType = currentPieceType;
      break;
    }
  }

  uint64_t moves = 0;
  moves |= getInfluence(boardState, square, pieceType) & ~alliedPieces;

  // handling castling moves
  if(pieceType == bKing && square == 4) {
    if(boardState.castlingRights[bKingSide] && !((boardState.blackPieces | boardState.whitePieces) & (uint64_t)3 << 5) && !(boardState.whiteInfluence & (uint64_t)7 << 4)) {
      moves |= (uint64_t)1 << 6;
    }
    if(boardState.castlingRights[bQueenSide] && !((boardState.blackPieces | boardState.whitePieces) & (uint64_t)7 << 1) && !(boardState.whiteInfluence & (uint64_t)15 << 1)) {
      moves |= (uint64_t)1 << 2;
    }
  }
  if(pieceType == wKing && square == 60) {
    if(boardState.castlingRights[wKingSide] && !((boardState.whitePieces | boardState.blackPieces) & (uint64_t)3 << 61) && !(boardState.blackInfluence & (uint64_t)7 << 60)) {
      moves |= (uint64_t)1 << 62;
    }
    if(boardState.castlingRights[wQueenSide] && !((boardState.whitePieces | boardState.blackPieces) & (uint64_t)7 << 57) && !(boardState.blackInfluence & (uint64_t)15 << 57)) {
      moves |= (uint64_t)1 << 58;
    }
  }

  if(pieceType == bPawns) {
    // pawn's influence is only a valid move if it captures something
    moves &= boardState.whitePieces;
    // handling non capture pawn moves
    moves |= pawnMaps[square][2] & ~(boardState.whitePieces | boardState.blackPieces);
    if(8 <= square && square < 16 && (boardState.whitePieces | boardState.blackPieces) & ((uint64_t)1 << square + 8)) {
      moves &= ~pawnMaps[square][2];
    }
    // handling enpassant moves
    if(boardState.lastMove & boardState.pieces[wPawns] && (boardState.lastMove << 8) & pawnMaps[square][0] && (boardState.lastMove >> 8) & pawnMaps[square][0]) {
      moves |= (boardState.lastMove >> 8) & pawnMaps[square][0];
    }
  }
  // same logic as bPawns
  if(pieceType == wPawns) {
    moves &= boardState.blackPieces;
    moves |= pawnMaps[square][3] & ~(boardState.whitePieces | boardState.blackPieces);
    if(48 <= square && square < 56 && (boardState.whitePieces | boardState.blackPieces) & ((uint64_t)1 << square - 8)) {
      moves &= ~pawnMaps[square][3];
    }
    if(boardState.lastMove & boardState.pieces[bPawns] && (boardState.lastMove >> 8) & pawnMaps[square][1] && (boardState.lastMove << 8) & pawnMaps[square][1]) {
      moves |= (boardState.lastMove << 8) & pawnMaps[square][1];
    }
  }

  // handling king safety
  uint64_t rawMoves = moves, rawBit;
  int rawSquare;
  while(rawMoves) {
    rawSquare = __builtin_ctzll(rawMoves);
    rawMoves &= rawMoves - 1;

    rawBit = (uint64_t)1 << rawSquare;
    if(0 <= pieceType && pieceType < 6) {
      boardState.blackPieces |= rawBit;
      boardState.blackPieces &= ~bit;
      int capturedPiece = -1;
      for(int enemyPiece = 6; enemyPiece < 12; enemyPiece++) {
        if(boardState.pieces[enemyPiece] & rawBit) {
          boardState.pieces[enemyPiece] &= ~rawBit;
          capturedPiece = enemyPiece;
          break;
        }
      }
      updateInfluence(&boardState);
      if((boardState.whiteInfluence & boardState.pieces[bKing] && pieceType != bKing)
      || (pieceType == bKing && boardState.whiteInfluence & rawBit)) {
        moves &= ~rawBit;
      }
      boardState.blackPieces &= ~rawBit;
      boardState.blackPieces |= bit;
      if(capturedPiece != -1) {
        boardState.pieces[capturedPiece] |= rawBit;
      }
      updateInfluence(&boardState);
    }
    else if(6 <= pieceType && pieceType < 12) {
      boardState.whitePieces |= rawBit;
      boardState.whitePieces &= ~bit;
      int capturedPiece = -1;
      for(int enemyPiece = 0; enemyPiece < 6; enemyPiece++) {
        if(boardState.pieces[enemyPiece] & rawBit) {
          boardState.pieces[enemyPiece] &= ~rawBit;
          capturedPiece = enemyPiece;
          break;
        }
      }
      updateInfluence(&boardState);
      if((boardState.blackInfluence & boardState.pieces[wKing] && pieceType != wKing)
      || (pieceType == wKing && boardState.blackInfluence & rawBit)) {
        moves &= ~rawBit;
      }
      boardState.whitePieces &= ~rawBit;
      boardState.whitePieces |= bit;
      if(capturedPiece != -1) {
        boardState.pieces[capturedPiece] |= rawBit;
      }
      updateInfluence(&boardState);
    }
  }

  return moves;
}
