enum pieces {bRooks, bKnights, bBishops, bQueens, bKing, bPawns, wRooks, wKnights, wBishops, wQueens, wKing, wPawns};
enum castlingRights {wKingSide, wQueenSide, bKingSide, bQueenSide};

typedef struct {
  uint64_t fromBit;
  uint64_t toBit;
  int movedPiece;
  int capturedPiece;
  bool changedCastlingRights[4];
  bool isPromoting;
} Move;

typedef struct {
  Move moves[100];
  int top;
} Stack;

void push(Stack *pStack, uint64_t value);
void pop(Stack *pStack);

typedef struct {
  uint64_t pieces[12];
  uint64_t whitePieces, blackPieces;
  uint64_t whiteInfluence, blackInfluence;
  bool castlingRights[4];
  Stack 
  bool isWhiteTurn;
} BoardState;

void initializeBoard(BoardState *boardState, char *FENString);
void initializePieceMaps(uint64_t rookMaps[64][4], uint64_t bishopMaps[64][4], uint64_t knightMaps[64], uint64_t kingMaps[64], uint64_t pawnMaps[64][4]);

uint64_t getBeamInfluence(BoardState boardState, int square, uint64_t pieceMaps[square][4]);
uint64_t getInfluence(BoardState boardState, int square, int pieceType);
void updateInfluence(BoardState *pBoardState);
void move(BoardState *pBoardState, int fromSquare, int toSquare);
uint64_t getMoves(BoardState boardState, int square);

int evaluate(BoardState boardState);

