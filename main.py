import pygame as pg
from ctypes import *

engine = CDLL("./engine.so")

engine.initializeBoard.argtypes = [c_char_p]
engine.getMoves.argtypes = [c_int]
engine.getMoves.restype = c_uint64
engine.getBoard.restype = POINTER(c_uint64)
engine.getLastMove.restype = c_uint64
engine.getWhiteInfluence.restype = c_uint64
engine.getBlackInfluence.restype = c_uint64

FENString = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0'
engine.initializeBoard(FENString.encode('utf-8'))
board = engine.getBoard()
lastMove = engine.getLastMove()

pg.init()
windowSize = 960 / 2
squareSize = windowSize / 8
window = pg.display.set_mode((windowSize, windowSize))

# sprite initialization
spriteDictionary = {}
pieceNames = ['bRooks', 'bKnights', 'bBishops', 'bQueens', 'bKing', 'bPawns', 'wRooks', 'wKnights', 'wBishops', 'wQueens', 'wKing', 'wPawns']
for pieceName in pieceNames:
    img = pg.image.load('./assets/' + pieceName + '.png')
    img = pg.transform.smoothscale(img, (squareSize, squareSize))
    spriteDictionary[pieceName] = img

def drawBoard():
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
        if (lastMove >> i) & 1:
            x = (i % 8) * squareSize
            y = (i // 8) * squareSize
            window.blit(highlight, (x, y))

    # drawing the pieces
    for piece in pieceNames:
        bitBoard = board[pieceNames.index(piece)]
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

selectedMoves = 0
selectedPiece = 0
drawBoard()
pg.display.flip()
running = True
while running:
    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False

        if event.type == pg.MOUSEBUTTONDOWN:
            if event.button == 2:
                print('WHITE INFLUENCE')
                drawBoard()
                selectedMoves = engine.getWhiteInfluence()
                drawSelectedMoves(selectedMoves)
            elif event.button == 3:
                print('BLACK INFLUENCE')
                drawBoard()
                selectedMoves = engine.getBlackInfluence()
                drawSelectedMoves(selectedMoves)
            else:
                clickedSquare = int((event.pos[0] // squareSize) + (event.pos[1] // squareSize) * 8)
                if(1 << clickedSquare) & selectedMoves:
                    engine.move(selectedPiece, clickedSquare)
                    board = engine.getBoard()
                    selectedPiece = 0
                    selectedMoves = 0
                    lastMove = engine.getLastMove()
                    drawBoard()
                else:
                    drawBoard()
                    selectedPiece = clickedSquare
                    selectedMoves = engine.getMoves(clickedSquare)
                    drawSelectedMoves(selectedMoves)
            pg.display.flip()

pg.quit()
