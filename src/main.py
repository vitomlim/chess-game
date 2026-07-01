import pygame as pg
from ctypes import *
import numpy as np

class BoardState(Structure):
    _fields_ = [
        ("pieces", c_uint64 * 12),
        ("whitePieces", c_uint64),
        ("blackPieces", c_uint64),
        ("whiteInfluence", c_uint64),
        ("blackInfluence", c_uint64),
        ("castlingRights", c_bool * 4),
        ("lastMove", c_uint64),
        ("isWhiteTurn", c_bool),
    ]

def drawBoard(BoardState: boardState):
    # drawing the background
    for y in range(8):
        for x in range(8):
            if (x + y) % 2 == 0:
                color = (229, 230, 203)
            else:
                color = (112, 146, 80)

            square = pg.Rect(x * squareSize, y * squareSize, squareSize, squareSize)
            pg.draw.rect(window, color, square)

    # highlighting the last move
    highlight = pg.Surface((squareSize, squareSize), pg.SRCALPHA)
    highlight.fill((255, 255, 0, 100))
    for i in range(64):
        if (boardState.lastMove >> i) & 1:
            x = (i % 8) * squareSize
            y = (i // 8) * squareSize
            window.blit(highlight, (x, y))

    # drawing the pieces
    for piece in pieceNames:
        bitBoard = boardState.pieces[pieceNames.index(piece)]
        for square in range(64):
            if(bitBoard >> square) & 1:
                x = (square % 8) * squareSize
                y = (square // 8) * squareSize
                window.blit(spriteDictionary[piece], (x, y))

def drawSelectedMoves(selectedMoves):
    highlight = pg.Surface((squareSize, squareSize), pg.SRCALPHA)
    highlight.fill((255, 255, 0, 100))
    for i in range(64):
        if (selectedMoves >> i) & 1:
            x = (i % 8) * squareSize
            y = (i // 8) * squareSize
            window.blit(highlight, (x, y))

def drawEvaluation(evaluation):
    bar = pg.Surface((squareSize / 2, 7 * squareSize))
    bar.fill((255, 255, 255))
    window.blit(bar, (8.5 * squareSize, 0.5 * squareSize))
    blackEval = pg.Surface((squareSize / 2, evalFunction(evaluation / 500) * 7 * squareSize))
    blackEval.fill((64, 61, 57))
    window.blit(blackEval, (8.5 * squareSize, 0.5 * squareSize))
    font = pg.font.Font(None, 14)
    text = font.render(str(evaluation), True, (0, 0, 255))
    textRect = text.get_rect(center=(8.5 * squareSize + (bar.get_width() // 2), 7 * squareSize))
    window.blit(text, textRect)

def evalFunction(x):
    return 1 / (1 + np.exp(x))

game = CDLL("./game.so")
game.initializeGame.argtypes = [POINTER(BoardState), c_char_p]
game.getMoves.argtypes = [BoardState, c_int]
game.getMoves.restype = c_uint64
game.move.argtypes = [POINTER(BoardState), c_int, c_int]

engine = CDLL("./engine.so")
engine.evaluate.argtypes = [BoardState]
engine.evaluate.restype = c_int

FENString = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
boardState = BoardState()
selectedMoves = 0
selectedPiece = 0
game.initializeGame(byref(boardState), FENString.encode('utf-8'))

pg.init()
squareSize = 60
window = pg.display.set_mode((9.5 * squareSize, 8 * squareSize))
window.fill((48, 46, 43))

# sprite initialization
spriteDictionary = {}
pieceNames = ['bRooks', 'bKnights', 'bBishops', 'bQueens', 'bKing', 'bPawns', 'wRooks', 'wKnights', 'wBishops', 'wQueens', 'wKing', 'wPawns']
for pieceName in pieceNames:
    img = pg.image.load('../assets/' + pieceName + '.png')
    img = pg.transform.smoothscale(img, (squareSize, squareSize))
    spriteDictionary[pieceName] = img

drawBoard(boardState)
drawEvaluation(0)
pg.display.flip()
running = True
while running:
    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False

        if event.type == pg.MOUSEBUTTONDOWN:
            clickedSquare = int((event.pos[0] // squareSize) + (event.pos[1] // squareSize) * 8)
            if(1 << clickedSquare) & selectedMoves:
                game.move(byref(boardState), selectedPiece, clickedSquare)
                selectedPiece = 0
                selectedMoves = 0
                drawBoard(boardState)
                evaluation = engine.evaluate(boardState)
                drawEvaluation(evaluation)
            else:
                drawBoard(boardState)
                selectedPiece = clickedSquare
                selectedMoves = game.getMoves(boardState, clickedSquare)
                drawSelectedMoves(selectedMoves)
            pg.display.flip()

pg.quit()
