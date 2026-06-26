#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "types.h"
#include "moves.h"

uint64_t rookMaps[64][4], bishopMaps[64][4], knightMaps[64], kingMaps[64], pawnMaps[64][4];
uint64_t board[12], whitePieces, blackPieces; bool castlingRights[4];
uint64_t lastMove; bool isWhiteTurn;
uint64_t whiteInfluence, blackInfluence;
uint64_t firstRank = (uint64_t)0b11111111 << 56, lastRank = (uint64_t)0b11111111, secondRank = (uint64_t)0b11111111 << 48, seventhRank = (uint64_t)0b11111111 << 8, thirdRank = (uint64_t)0b11111111 << 40, sixthRank = (uint64_t)0b11111111 << 16, fourthRank = (uint64_t)0b11111111 << 32, fifthRank = (uint64_t)0b11111111 << 24;

uint64_t getBeamInfluence(int square, uint64_t pieceMaps[64][4]) {
  uint64_t influence = 0;
  for(int i = 0; i < 2; i++) {
    uint64_t crescentCollisions = pieceMaps[square][i + 2] & (whitePieces | blackPieces);
    crescentCollisions |= crescentCollisions >> 1;
    crescentCollisions |= crescentCollisions >> 2;
    crescentCollisions |= crescentCollisions >> 4;
    crescentCollisions |= crescentCollisions >> 8;
    crescentCollisions |= crescentCollisions >> 16;
    crescentCollisions |= crescentCollisions >> 32;
    crescentCollisions = crescentCollisions >> 1;

    influence |= (~crescentCollisions & pieceMaps[square][i + 2]);

    uint64_t decrescentCollisions = pieceMaps[square][i] & (whitePieces | blackPieces);
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

uint64_t getInfluence(int square, int pieceType) {
  if(pieceType == -1)                                         return 0;
  else if(pieceType == bRooks || pieceType == wRooks)         return getBeamInfluence(square, rookMaps);
  else if(pieceType == bKnights || pieceType == wKnights)     return knightMaps[square];
  else if(pieceType == bBishops || pieceType == wBishops)     return getBeamInfluence(square, bishopMaps);
  else if(pieceType == bQueens || pieceType == wQueens)       return getBeamInfluence(square, rookMaps) | getBeamInfluence(square, bishopMaps);
  else if(pieceType == bKing || pieceType == wKing)           return kingMaps[square];
  else if(pieceType == wPawns)                                return pawnMaps[square][1];
  else if(pieceType == bPawns)                                return pawnMaps[square][0];
}

void updateInfluence() {
  whiteInfluence = 0;
  blackInfluence = 0;

  uint64_t currentPieces;
  int square;

  for(int blackPieceType = 0; blackPieceType < 6; blackPieceType++) {
    currentPieces = board[blackPieceType];
    while(currentPieces) {
      square = __builtin_ctzll(currentPieces);
      currentPieces &= currentPieces - 1;
      blackInfluence |= getInfluence(square, blackPieceType);
    }
  }
  for(int whitePieceType = 6; whitePieceType < 12; whitePieceType++) {
    currentPieces = board[whitePieceType];
    while(currentPieces) {
      square = __builtin_ctzll(currentPieces);
      currentPieces &= currentPieces - 1;
      whiteInfluence |= getInfluence(square, whitePieceType);
    }
  }
}

void move(int fromSquare, int toSquare) {

  uint64_t fromBit = (uint64_t)1 << fromSquare;
  uint64_t toBit = (uint64_t)1 << toSquare;

  // handling promotion move (pawn to last rank)
  // if pawn moves to last rank, the move is ended without updating current turn
  // getMoves function checks if lastMove has the same color as current turn
  // if it does, it returns 0b1111, which is used to choose the promoted piece
  if(board[wPawns] & fromBit && lastRank & toBit || board[bPawns] & fromBit && firstRank & toBit) {
    lastMove = fromBit | toBit;
    return;
  }

  int movedPiece = -1;
  // handling promotion choice (choice between queen, rook, knight and bishop)
  if(isWhiteTurn && lastMove & whitePieces) {
    fromBit = lastMove & whitePieces;
    toBit = lastMove & ~fromBit;
    board[wPawns] &= ~fromBit;
    switch(toSquare) {
      case 0: movedPiece = wQueens; break;
      case 1: movedPiece = wRooks; break;
      case 2: movedPiece = wKnights; break;
      case 3: movedPiece = wBishops; break;
    }
  }
  if(!isWhiteTurn && lastMove & blackPieces) {
    fromBit = lastMove & blackPieces;
    toBit = lastMove & ~fromBit;
    board[bPawns] &= ~fromBit;
    switch(toSquare) {
      case 0: movedPiece = bQueens; break;
      case 1: movedPiece = bRooks; break;
      case 2: movedPiece = bKnights; break;
      case 3: movedPiece = bBishops; break;
    }
  }

  // handle enpassant capture
  // if pawn is making capture move and there are no pieces in the target square
  if(board[wPawns] & fromBit
  && !(pawnMaps[fromSquare][1] & blackPieces)
  && pawnMaps[fromSquare][1] & toBit) {
    movedPiece = wPawns;
    board[bPawns] &= ~lastMove;
    blackPieces &= ~lastMove;
  }
  if(board[bPawns] & fromBit
  && !(pawnMaps[fromSquare][0] & whitePieces)
  && pawnMaps[fromSquare][0] & toBit) {
    movedPiece = bPawns;
    board[wPawns] &= ~lastMove;
    whitePieces &= ~lastMove;
  }

  // handle castling rights and rook movement in case of castling
  if(board[wKing] & fromBit) {
    movedPiece = wKing;
    castlingRights[wKingSide] = false;
    castlingRights[wQueenSide] = false;
    if(fromSquare == 60 && toSquare == 62) {
      board[wRooks] &= ~((uint64_t)1 << 63);
      board[wRooks] |= (uint64_t)1 << 61;
      whitePieces &= ~((uint64_t)1 << 63);
      whitePieces |= (uint64_t)1 << 61;
    }
    if(fromSquare == 60 && toSquare == 58) {
      board[wRooks] &= ~((uint64_t)1 << 56);
      board[wRooks] |= (uint64_t)1 << 59;
      whitePieces &= ~((uint64_t)1 << 56);
      whitePieces |= (uint64_t)1 << 59;
    }
  }
  if(board[bKing] & fromBit) {
    movedPiece = bKing;

    castlingRights[bKingSide] = false;
    castlingRights[bQueenSide] = false;
    if(fromSquare == 4 && toSquare == 6) {
      board[bRooks] &= ~((uint64_t)1 << 7);
      board[bRooks] |= (uint64_t)1 << 5;
      blackPieces &= ~((uint64_t)1 << 7);
      blackPieces |= (uint64_t)1 << 5;
    }
    if(fromSquare == 4 && toSquare == 2) {
      board[bRooks] &= ~((uint64_t)1 << 0);
      board[bRooks] |= (uint64_t)1 << 3;
      blackPieces &= ~((uint64_t)1 << 0);
      blackPieces |= (uint64_t)1 << 3;
    }
  }
  if(board[wRooks] & fromBit) {
    movedPiece = wRooks;
    if(fromSquare == 56) {
      castlingRights[wQueenSide] = false;
    }
    if(fromSquare == 63) {
      castlingRights[wKingSide] = false;
    }
  }
  if(board[bRooks] & fromBit) {
    movedPiece = bRooks;
    if(fromSquare == 0) {
      castlingRights[bQueenSide] = false;
    }
    if(fromSquare == 7) {
      castlingRights[bKingSide] = false;
    }
  }

  // updating board state
  if(movedPiece == -1) {
    for(movedPiece = isWhiteTurn ? 6 : 0; movedPiece < isWhiteTurn ? 12 : 6; movedPiece++) {
      if(board[movedPiece] & fromBit) {
        break;
      }
    }
  }
  board[movedPiece] &= ~fromBit;
  board[movedPiece] |= toBit;
  for(int capturedPiece = isWhiteTurn ? 0 : 6; capturedPiece < (isWhiteTurn ? 6 : 12); capturedPiece++) {
    if(board[capturedPiece] & toBit) {
      board[capturedPiece] &= ~toBit;
      break;
    }
  }
  if(isWhiteTurn) {
    whitePieces &= ~fromBit;
    whitePieces |= toBit;
    blackPieces &= ~toBit;
  } else {
    blackPieces &= ~fromBit;
    blackPieces |= toBit;
    whitePieces &= ~toBit;
  }
  lastMove = fromBit | toBit;
  isWhiteTurn = !isWhiteTurn;
  updateInfluence();
}

uint64_t getMoves(int square) {

  // handling promotion
  // move function doesn't update current turn if last move was promotion
  uint64_t alliedPieces = isWhiteTurn ? whitePieces : blackPieces;
  if(lastMove & alliedPieces) {
    return (uint64_t)0b1111;
  }

  // Moves without considering king safety
  int pieceType = -1;
  uint64_t bit = (uint64_t)1 << square;
  for(int currentPieceType = (isWhiteTurn ? 6 : 0); currentPieceType < (isWhiteTurn ? 12 : 6); currentPieceType++) {
    if(board[currentPieceType] & bit) {
      pieceType = currentPieceType;
      break;
    }
  }

  uint64_t moves = 0;
  moves |= getInfluence(square, pieceType) & ~alliedPieces;

  // handling castling moves
  if(pieceType == bKing && square == 4) {
    if(castlingRights[bKingSide] && !((blackPieces | whitePieces) & (uint64_t)3 << 5) && !(whiteInfluence & (uint64_t)7 << 4)) {
      moves |= (uint64_t)1 << 6;
    }
    if(castlingRights[bQueenSide] && !((blackPieces | whitePieces) & (uint64_t)7 << 1) && !(whiteInfluence & (uint64_t)15 << 1)) {
      moves |= (uint64_t)1 << 2;
    }
  }
  if(pieceType == wKing && square == 60) {
    if(castlingRights[wKingSide] && !((whitePieces | blackPieces) & (uint64_t)3 << 61) && !(blackInfluence & (uint64_t)7 << 60)) {
      moves |= (uint64_t)1 << 62;
    }
    if(castlingRights[wQueenSide] && !((whitePieces | blackPieces) & (uint64_t)7 << 57) && !(blackInfluence & (uint64_t)15 << 57)) {
      moves |= (uint64_t)1 << 58;
    }
  }

  if(pieceType == bPawns) {
    // pawn's influence is only a valid move if it captures something
    moves &= whitePieces;
    // handling non capture pawn moves
    moves |= pawnMaps[square][2] & ~(whitePieces | blackPieces);
    if(8 <= square && square < 16 && (whitePieces | blackPieces) & ((uint64_t)1 << square + 8)) {
      moves &= ~pawnMaps[square][2];
    }
    // handling enpassant moves
    if(lastMove & board[wPawns] && (lastMove << 8) & pawnMaps[square][0] && (lastMove >> 8) & pawnMaps[square][0]) {
      moves |= (lastMove >> 8) & pawnMaps[square][0];
    }
  }
  // same logic as bPawns
  if(pieceType == wPawns) {
    moves &= blackPieces;
    moves |= pawnMaps[square][3] & ~(whitePieces | blackPieces);
    if(48 <= square && square < 56 && (whitePieces | blackPieces) & ((uint64_t)1 << square - 8)) {
      moves &= ~pawnMaps[square][3];
    }
    if(lastMove & board[bPawns] && (lastMove >> 8) & pawnMaps[square][1] && (lastMove << 8) & pawnMaps[square][1]) {
      moves |= (lastMove << 8) & pawnMaps[square][1];
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
      blackPieces |= rawBit;
      blackPieces &= ~bit;
      updateInfluence();
      if((whiteInfluence & board[bKing] && pieceType != bKing)
      || (pieceType == bKing && whiteInfluence & rawBit)) {
        moves &= ~rawBit;
      }
      blackPieces &= ~rawBit;
      blackPieces |= bit;
      updateInfluence();
    }
    else if(6 <= pieceType && pieceType < 12) {
      whitePieces |= rawBit;
      whitePieces &= ~bit;
      updateInfluence();
      if((blackInfluence & board[wKing] && pieceType != wKing)
      || (pieceType == wKing && blackInfluence & rawBit)) {
        moves &= ~rawBit;
      }
      whitePieces &= ~rawBit;
      whitePieces |= bit;
      updateInfluence();
    }
  }

  return moves;
}

// getting board state from FEN code
void initializeBoard(char* FENString) {

  initializePieceMaps(rookMaps, bishopMaps, knightMaps, kingMaps, pawnMaps);

  // first field: pieces position
  int x = 0, y = 0;
  while(*FENString && *FENString != ' ') {
    char currentChar = *FENString++;

    if('1' <= currentChar && currentChar <= '8') {
      x += currentChar - '0';
      continue;
    }

    if(currentChar == '/') {
      y++;
      x = 0;
      continue;
    }

    int currentSquare = x + 8*y;
    uint64_t currentBit = (uint64_t)1 << currentSquare;
    switch(currentChar) {
      case 'r': board[bRooks] |= currentBit; break;
      case 'n': board[bKnights] |= currentBit; break;
      case 'b': board[bBishops] |= currentBit; break;
      case 'q': board[bQueens] |= currentBit; break;
      case 'k': board[bKing] |= currentBit; break;
      case 'p': board[bPawns] |= currentBit; break;
      case 'R': board[wRooks] |= currentBit; break;
      case 'N': board[wKnights] |= currentBit; break;
      case 'B': board[wBishops] |= currentBit; break;
      case 'Q': board[wQueens] |= currentBit; break;
      case 'K': board[wKing] |= currentBit; break;
      case 'P': board[wPawns] |= currentBit; break;
    }
    x++;
  }
  whitePieces = board[wPawns] | board[wKnights] | board[wBishops] | board[wRooks] | board[wQueens] | board[wKing];
  blackPieces = board[bPawns] | board[bKnights] | board[bBishops] | board[bRooks] | board[bQueens] | board[bKing];
  updateInfluence();

  FENString++;
  // second field: current turn
  isWhiteTurn = (*FENString++ == 'w');

  // third field: castling rights
  castlingRights[0] = strchr(FENString, 'K') != NULL;
  castlingRights[1] = strchr(FENString, 'Q') != NULL;
  castlingRights[2] = strchr(FENString, 'k') != NULL;
  castlingRights[3] = strchr(FENString, 'q') != NULL;

}

uint64_t* getBoard() {
  return board;
}

uint64_t getLastMove() {
  return lastMove;
}

uint64_t getWhiteInfluence() {
  return whiteInfluence;
}

uint64_t getBlackInfluence() {
  return blackInfluence;
}
