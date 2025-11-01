#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>
#include <limits>
#include <android/log.h>

// Remove these problematic lines:
// static std::mt19937 rng(std::random_device{}());
// static std::uniform_int_distribution<int> smallNoise(0, 5);
// static std::uniform_int_distribution<int> coinFlip(0, 1);

// Replace with thread-safe function-local statics:
std::mt19937& getRNG() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

int getSmallNoise() {
    static std::uniform_int_distribution<int> dist(0, 5);
    return dist(getRNG());
}

int getCoinFlip() {
    static std::uniform_int_distribution<int> dist(0, 1);
    return dist(getRNG());
}

enum PieceType { EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6 };
enum Color { NONE = 0, WHITE = 1, BLACK = 2 };

struct Piece {
    PieceType type;
    Color color;
    bool hasMoved;
};

struct Move {
    int fromRow, fromCol, toRow, toCol;
    bool isCapture;
    bool isCastling;
    bool isEnPassant;
    bool isPromotion;
    int score;
    Piece capturedPiece;
};

class ChessGame {
private:
    Piece board[8][8];
    Color currentPlayer;
    bool whiteKingMoved, blackKingMoved;
    bool whiteRookAMoved, whiteRookHMoved;
    bool blackRookAMoved, blackRookHMoved;
    int enPassantCol;
    int enPassantRow;
    std::vector<Move> moveHistory;
    int halfMoveClock;
    int fullMoveNumber;

    // Improved piece-square tables
    int pawnTableWhite[8][8] = {
            {0,   0,   0,   0,   0,   0,   0,   0},
            {50,  50,  50,  50,  50,  50,  50,  50},
            {10,  10,  20,  30,  30,  20,  10,  10},
            {5,   5,   10,  27,  27,  10,  5,   5},
            {0,   0,   0,   25,  25,  0,   0,   0},
            {5,   -5,  -10, 0,   0,   -10, -5,  5},
            {5,   10,  10,  -25, -25, 10,  10,  5},
            {0,   0,   0,   0,   0,   0,   0,   0}
    };

    int knightTable[8][8] = {
            {-50, -40, -30, -30, -30, -30, -40, -50},
            {-40, -20, 0,   5,   5,   0,   -20, -40},
            {-30, 5,   10,  15,  15,  10,  5,   -30},
            {-30, 0,   15,  20,  20,  15,  0,   -30},
            {-30, 5,   15,  20,  20,  15,  5,   -30},
            {-30, 0,   10,  15,  15,  10,  0,   -30},
            {-40, -20, 0,   0,   0,   0,   -20, -40},
            {-50, -40, -20, -30, -30, -20, -40, -50}
    };

    int bishopTable[8][8] = {
            {-20, -10, -10, -10, -10, -10, -10, -20},
            {-10, 5,   0,   0,   0,   0,   5,   -10},
            {-10, 10,  10,  10,  10,  10,  10,  -10},
            {-10, 0,   10,  10,  10,  10,  0,   -10},
            {-10, 5,   5,   10,  10,  5,   5,   -10},
            {-10, 0,   5,   10,  10,  5,   0,   -10},
            {-10, 0,   0,   0,   0,   0,   0,   -10},
            {-20, -10, -40, -10, -10, -40, -10, -20}
    };

    int rookTable[8][8] = {
            {0,  0,  0,  5,  5,  0,  0,  0},
            {-5, 0,  0,  0,  0,  0,  0,  -5},
            {-5, 0,  0,  0,  0,  0,  0,  -5},
            {-5, 0,  0,  0,  0,  0,  0,  -5},
            {-5, 0,  0,  0,  0,  0,  0,  -5},
            {-5, 0,  0,  0,  0,  0,  0,  -5},
            {5,  10, 10, 10, 10, 10, 10, 5},
            {0,  0,  0,  0,  0,  0,  0,  0}
    };

    int queenTable[8][8] = {
            {-20, -10, -10, -5, -5, -10, -10, -20},
            {-10, 0,   5,   0,  0,  0,   0,   -10},
            {-10, 5,   5,   5,  5,  5,   0,   -10},
            {0,   0,   5,   5,  5,  5,   0,   -5},
            {-5,  0,   5,   5,  5,  5,   0,   -5},
            {-10, 0,   5,   5,  5,  5,   0,   -10},
            {-10, 0,   0,   0,  0,  0,   0,   -10},
            {-20, -10, -10, -5, -5, -10, -10, -20}
    };

    int kingTableMiddle[8][8] = {
            {-30, -40, -40, -50, -50, -40, -40, -30},
            {-30, -40, -40, -50, -50, -40, -40, -30},
            {-30, -40, -40, -50, -50, -40, -40, -30},
            {-30, -40, -40, -50, -50, -40, -40, -30},
            {-20, -30, -30, -40, -40, -30, -30, -20},
            {-10, -20, -20, -20, -20, -20, -20, -10},
            {20,  20,  0,   0,   0,   0,   20,  20},
            {20,  30,  10,  0,   0,   10,  30,  20}
    };

    int kingTableEnd[8][8] = {
            {-50, -30, -30, -30, -30, -30, -30, -50},
            {-30, -30, 0,   0,   0,   0,   -30, -30},
            {-30, -10, 20,  30,  30,  20,  -10, -30},
            {-30, -10, 30,  40,  40,  30,  -10, -30},
            {-30, -10, 30,  40,  40,  30,  -10, -30},
            {-30, -10, 20,  30,  30,  20,  -10, -30},
            {-30, -20, -10, 0,   0,   -10, -20, -30},
            {-50, -40, -30, -20, -20, -30, -40, -50}
    };

public:
    ChessGame() {
        initializeBoard();
    }

    void initializeBoard() {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board[i][j] = {EMPTY, NONE, false};
            }
        }

        board[0][0] = {ROOK, BLACK, false};
        board[0][1] = {KNIGHT, BLACK, false};
        board[0][2] = {BISHOP, BLACK, false};
        board[0][3] = {QUEEN, BLACK, false};
        board[0][4] = {KING, BLACK, false};
        board[0][5] = {BISHOP, BLACK, false};
        board[0][6] = {KNIGHT, BLACK, false};
        board[0][7] = {ROOK, BLACK, false};

        for (int i = 0; i < 8; i++) {
            board[1][i] = {PAWN, BLACK, false};
        }

        board[7][0] = {ROOK, WHITE, false};
        board[7][1] = {KNIGHT, WHITE, false};
        board[7][2] = {BISHOP, WHITE, false};
        board[7][3] = {QUEEN, WHITE, false};
        board[7][4] = {KING, WHITE, false};
        board[7][5] = {BISHOP, WHITE, false};
        board[7][6] = {KNIGHT, WHITE, false};
        board[7][7] = {ROOK, WHITE, false};

        for (int i = 0; i < 8; i++) {
            board[6][i] = {PAWN, WHITE, false};
        }

        currentPlayer = WHITE;
        whiteKingMoved = blackKingMoved = false;
        whiteRookAMoved = whiteRookHMoved = false;
        blackRookAMoved = blackRookHMoved = false;
        enPassantCol = -1;
        enPassantRow = -1;
        halfMoveClock = 0;
        fullMoveNumber = 1;
        moveHistory.clear();
    }

    int getPiece(int row, int col) {
        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Invalid board access: %d, %d", row, col);
            return 0;
        }
        Piece p = board[row][col];
        if (p.type == EMPTY) return 0;
        return (p.color * 10) + p.type;
    }

    bool isValidPosition(int row, int col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    std::vector<Move> getLegalMoves(int row, int col) {
        std::vector<Move> moves;
        if (!isValidPosition(row, col)) return moves;

        Piece piece = board[row][col];
        if (piece.type == EMPTY || piece.color != currentPlayer) return moves;

        std::vector<Move> pseudoMoves = getPseudoLegalMoves(row, col);

        for (const Move& move : pseudoMoves) {
            if (isLegalMove(move)) {
                moves.push_back(move);
            }
        }

        return moves;
    }

    std::vector<Move> getPseudoLegalMoves(int row, int col) {
        std::vector<Move> moves;
        Piece piece = board[row][col];

        switch (piece.type) {
            case PAWN:
                getPawnMoves(row, col, moves);
                break;
            case KNIGHT:
                getKnightMoves(row, col, moves);
                break;
            case BISHOP:
                getBishopMoves(row, col, moves);
                break;
            case ROOK:
                getRookMoves(row, col, moves);
                break;
            case QUEEN:
                getQueenMoves(row, col, moves);
                break;
            case KING:
                getKingMoves(row, col, moves);
                break;
        }

        return moves;
    }

    void getPawnMoves(int row, int col, std::vector<Move>& moves) {
        Piece piece = board[row][col];
        int direction = (piece.color == WHITE) ? -1 : 1;
        int startRow = (piece.color == WHITE) ? 6 : 1;

        if (isValidPosition(row + direction, col) && board[row + direction][col].type == EMPTY) {
            bool isPromotion = (row + direction == 0 || row + direction == 7);
            moves.push_back({row, col, row + direction, col, false, false, false, isPromotion, 0, {EMPTY, NONE, false}});

            if (row == startRow && board[row + 2 * direction][col].type == EMPTY) {
                moves.push_back({row, col, row + 2 * direction, col, false, false, false, false, 0, {EMPTY, NONE, false}});
            }
        }

        for (int dcol : {-1, 1}) {
            int newRow = row + direction;
            int newCol = col + dcol;
            if (isValidPosition(newRow, newCol)) {
                Piece target = board[newRow][newCol];
                if (target.type != EMPTY && target.color != piece.color) {
                    bool isPromotion = (newRow == 0 || newRow == 7);
                    moves.push_back({row, col, newRow, newCol, true, false, false, isPromotion, 0, target});
                }
                if (newCol == enPassantCol && newRow == enPassantRow) {
                    Piece capturedPawn = {PAWN, (piece.color == WHITE ? BLACK : WHITE), true};
                    moves.push_back({row, col, newRow, newCol, true, false, true, false, 0, capturedPawn});
                }
            }
        }
    }

    void getKnightMoves(int row, int col, std::vector<Move>& moves) {
        Piece piece = board[row][col];
        int offsets[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};

        for (auto& offset : offsets) {
            int newRow = row + offset[0];
            int newCol = col + offset[1];
            if (isValidPosition(newRow, newCol)) {
                Piece target = board[newRow][newCol];
                if (target.type == EMPTY || target.color != piece.color) {
                    moves.push_back({row, col, newRow, newCol, target.type != EMPTY, false, false, false, 0, target});
                }
            }
        }
    }

    void getBishopMoves(int row, int col, std::vector<Move>& moves) {
        int directions[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
        getSlidingMoves(row, col, directions, 4, moves);
    }

    void getRookMoves(int row, int col, std::vector<Move>& moves) {
        int directions[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        getSlidingMoves(row, col, directions, 4, moves);
    }

    void getQueenMoves(int row, int col, std::vector<Move>& moves) {
        int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
        getSlidingMoves(row, col, directions, 8, moves);
    }

    void getSlidingMoves(int row, int col, int directions[][2], int numDirs, std::vector<Move>& moves) {
        Piece piece = board[row][col];
        for (int d = 0; d < numDirs; d++) {
            for (int dist = 1; dist < 8; dist++) {
                int newRow = row + directions[d][0] * dist;
                int newCol = col + directions[d][1] * dist;
                if (!isValidPosition(newRow, newCol)) break;

                Piece target = board[newRow][newCol];
                if (target.type == EMPTY) {
                    moves.push_back({row, col, newRow, newCol, false, false, false, false, 0, {EMPTY, NONE, false}});
                } else {
                    if (target.color != piece.color) {
                        moves.push_back({row, col, newRow, newCol, true, false, false, false, 0, target});
                    }
                    break;
                }
            }
        }
    }

    void getKingMoves(int row, int col, std::vector<Move>& moves) {
        Piece piece = board[row][col];
        int directions[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};

        for (auto& dir : directions) {
            int newRow = row + dir[0];
            int newCol = col + dir[1];
            if (isValidPosition(newRow, newCol)) {
                Piece target = board[newRow][newCol];
                if (target.type == EMPTY || target.color != piece.color) {
                    moves.push_back({row, col, newRow, newCol, target.type != EMPTY, false, false, false, 0, target});
                }
            }
        }

        // Castling
        if (piece.color == WHITE && !whiteKingMoved && !isInCheck(WHITE)) {
            if (!whiteRookHMoved && board[7][5].type == EMPTY && board[7][6].type == EMPTY &&
                !isSquareAttacked(7, 5, BLACK) && !isSquareAttacked(7, 6, BLACK)) {
                moves.push_back({7, 4, 7, 6, false, true, false, false, 0, {EMPTY, NONE, false}});
            }
            if (!whiteRookAMoved && board[7][1].type == EMPTY && board[7][2].type == EMPTY &&
                board[7][3].type == EMPTY && !isSquareAttacked(7, 3, BLACK) && !isSquareAttacked(7, 2, BLACK)) {
                moves.push_back({7, 4, 7, 2, false, true, false, false, 0, {EMPTY, NONE, false}});
            }
        } else if (piece.color == BLACK && !blackKingMoved && !isInCheck(BLACK)) {
            if (!blackRookHMoved && board[0][5].type == EMPTY && board[0][6].type == EMPTY &&
                !isSquareAttacked(0, 5, WHITE) && !isSquareAttacked(0, 6, WHITE)) {
                moves.push_back({0, 4, 0, 6, false, true, false, false, 0, {EMPTY, NONE, false}});
            }
            if (!blackRookAMoved && board[0][1].type == EMPTY && board[0][2].type == EMPTY &&
                board[0][3].type == EMPTY && !isSquareAttacked(0, 3, WHITE) && !isSquareAttacked(0, 2, WHITE)) {
                moves.push_back({0, 4, 0, 2, false, true, false, false, 0, {EMPTY, NONE, false}});
            }
        }
    }

    bool isSquareAttacked(int row, int col, Color attackerColor) {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece piece = board[r][c];
                if (piece.type != EMPTY && piece.color == attackerColor) {
                    if (piece.type == PAWN) {
                        int direction = (attackerColor == WHITE) ? -1 : 1;
                        for (int dc : {-1, 1}) {
                            if (r + direction == row && c + dc == col) return true;
                        }
                    } else {
                        std::vector<Move> moves = getPseudoLegalMoves(r, c);
                        for (const Move& move : moves) {
                            if (move.toRow == row && move.toCol == col && !move.isCastling) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool isInCheck(Color color) {
        int kingRow = -1, kingCol = -1;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].type == KING && board[r][c].color == color) {
                    kingRow = r;
                    kingCol = c;
                    break;
                }
            }
            if (kingRow != -1) break;
        }

        if (kingRow == -1) return false;
        return isSquareAttacked(kingRow, kingCol, (color == WHITE) ? BLACK : WHITE);
    }

    bool isLegalMove(const Move& move) {
        Piece tempPiece = board[move.toRow][move.toCol];
        Piece movingPiece = board[move.fromRow][move.fromCol];
        int tempEnPassantCol = enPassantCol;
        int tempEnPassantRow = enPassantRow;
        Color savedPlayer = currentPlayer;

        board[move.toRow][move.toCol] = movingPiece;
        board[move.fromRow][move.fromCol] = {EMPTY, NONE, false};

        Piece capturedPawn = {EMPTY, NONE, false};
        if (move.isEnPassant) {
            int captureRow = (movingPiece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            capturedPawn = board[captureRow][move.toCol];
            board[captureRow][move.toCol] = {EMPTY, NONE, false};
        }

        Piece savedRook = {EMPTY, NONE, false};
        int rookFromCol = -1, rookToCol = -1;
        if (move.isCastling) {
            int row = move.fromRow;
            if (move.toCol == 6) {
                rookFromCol = 7;
                rookToCol = 5;
            } else {
                rookFromCol = 0;
                rookToCol = 3;
            }
            savedRook = board[row][rookToCol];
            board[row][rookToCol] = board[row][rookFromCol];
            board[row][rookFromCol] = {EMPTY, NONE, false};
        }

        bool legal = !isInCheck(savedPlayer);

        board[move.fromRow][move.fromCol] = movingPiece;
        board[move.toRow][move.toCol] = tempPiece;
        enPassantCol = tempEnPassantCol;
        enPassantRow = tempEnPassantRow;
        currentPlayer = savedPlayer;

        if (move.isEnPassant) {
            int captureRow = (movingPiece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            board[captureRow][move.toCol] = capturedPawn;
        }

        if (move.isCastling) {
            int row = move.fromRow;
            board[row][rookFromCol] = board[row][rookToCol];
            board[row][rookToCol] = savedRook;
        }

        return legal;
    }

    bool makeMove(int fromRow, int fromCol, int toRow, int toCol, int promotionPiece = QUEEN) {
        std::vector<Move> legalMoves = getLegalMoves(fromRow, fromCol);

        for (const Move& move : legalMoves) {
            if (move.toRow == toRow && move.toCol == toCol) {
                executeMoveInternal(move, promotionPiece);
                return true;
            }
        }
        return false;
    }

    void executeMoveInternal(const Move& move, int promotionPiece) {
        Piece piece = board[move.fromRow][move.fromCol];

        if (move.isEnPassant) {
            int captureRow = (piece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            board[captureRow][move.toCol] = {EMPTY, NONE, false};
        }

        if (move.isCastling) {
            if (move.toCol == 6) {
                board[move.fromRow][5] = board[move.fromRow][7];
                board[move.fromRow][7] = {EMPTY, NONE, false};
                board[move.fromRow][5].hasMoved = true;
            } else {
                board[move.fromRow][3] = board[move.fromRow][0];
                board[move.fromRow][0] = {EMPTY, NONE, false};
                board[move.fromRow][3].hasMoved = true;
            }
        }

        enPassantCol = -1;
        enPassantRow = -1;
        if (piece.type == PAWN && abs(move.fromRow - move.toRow) == 2) {
            enPassantCol = move.fromCol;
            enPassantRow = (move.fromRow + move.toRow) / 2;
        }

        if (piece.type == PAWN || move.isCapture) {
            halfMoveClock = 0;
        } else {
            halfMoveClock++;
        }

        board[move.toRow][move.toCol] = piece;
        board[move.fromRow][move.fromCol] = {EMPTY, NONE, false};
        board[move.toRow][move.toCol].hasMoved = true;

        if (move.isPromotion) {
            board[move.toRow][move.toCol].type = static_cast<PieceType>(promotionPiece);
        }

        if (piece.type == KING) {
            if (piece.color == WHITE) whiteKingMoved = true;
            else blackKingMoved = true;
        }
        if (piece.type == ROOK) {
            if (piece.color == WHITE) {
                if (move.fromCol == 0) whiteRookAMoved = true;
                if (move.fromCol == 7) whiteRookHMoved = true;
            } else {
                if (move.fromCol == 0) blackRookAMoved = true;
                if (move.fromCol == 7) blackRookHMoved = true;
            }
        }

        if (currentPlayer == BLACK) fullMoveNumber++;
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        moveHistory.push_back(move);
    }

    int evaluateBoard() {
        int score = 0;
        int pieceValues[7] = {0, 100, 320, 330, 500, 900, 20000};

        int materialCount = 0;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].type != EMPTY && board[r][c].type != KING) {
                    materialCount++;
                }
            }
        }
        bool isEndgame = materialCount < 12;

        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece piece = board[r][c];
                if (piece.type != EMPTY) {
                    int value = pieceValues[piece.type];
                    int posValue = 0;

                    if (piece.type == PAWN) {
                        posValue = (piece.color == WHITE) ? pawnTableWhite[r][c] : pawnTableWhite[7-r][c];
                    } else if (piece.type == KNIGHT) {
                        posValue = (piece.color == WHITE) ? knightTable[r][c] : knightTable[7-r][c];
                    } else if (piece.type == BISHOP) {
                        posValue = (piece.color == WHITE) ? bishopTable[r][c] : bishopTable[7-r][c];
                    } else if (piece.type == ROOK) {
                        posValue = (piece.color == WHITE) ? rookTable[r][c] : rookTable[7-r][c];
                    } else if (piece.type == QUEEN) {
                        posValue = (piece.color == WHITE) ? queenTable[r][c] : queenTable[7-r][c];
                    } else if (piece.type == KING) {
                        if (isEndgame) {
                            posValue = (piece.color == WHITE) ? kingTableEnd[r][c] : kingTableEnd[7-r][c];
                        } else {
                            posValue = (piece.color == WHITE) ? kingTableMiddle[r][c] : kingTableMiddle[7-r][c];
                        }
                    }

                    value += posValue;

                    // Mobility bonus
                    Color savedPlayer = currentPlayer;
                    currentPlayer = piece.color;
                    std::vector<Move> moves = getLegalMoves(r, c);
                    currentPlayer = savedPlayer;
                    value += moves.size() * 2;

                    if (piece.color == WHITE) score += value;
                    else score -= value;
                }
            }
        }

        // King safety
        if (!isEndgame) {
            if (isInCheck(WHITE)) score -= 50;
            if (isInCheck(BLACK)) score += 50;
        }

        return score;
    }

    Move getBestMove(int depth, int difficulty) {
        std::vector<Move> allMoves;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color == currentPlayer) {
                    std::vector<Move> moves = getLegalMoves(r, c);
                    allMoves.insert(allMoves.end(), moves.begin(), moves.end());
                }
            }
        }

        if (allMoves.empty()) {
            return {-1, -1, -1, -1, false, false, false, false, 0, {EMPTY, NONE, false}};
        }

        // Score and sort moves
        for (Move& move : allMoves) {
            move.score = scoreMoveOrdering(move);
        }

        std::sort(allMoves.begin(), allMoves.end(), [](const Move& a, const Move& b) {
            return a.score > b.score;
        });

        Move bestMove = allMoves[0];
        int bestScore = (currentPlayer == WHITE) ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

        for (Move& move : allMoves) {
            Piece temp = board[move.toRow][move.toCol];
            Piece tempEnPassant = {EMPTY, NONE, false};
            int savedEnPassantCol = enPassantCol;
            int savedEnPassantRow = enPassantRow;
            Color prevPlayer = currentPlayer;
            bool savedWhiteKing = whiteKingMoved, savedBlackKing = blackKingMoved;
            bool savedWRA = whiteRookAMoved, savedWRH = whiteRookHMoved;
            bool savedBRA = blackRookAMoved, savedBRH = blackRookHMoved;

            if (move.isEnPassant) {
                int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                tempEnPassant = board[captureRow][move.toCol];
            }

            executeMoveInternal(move, QUEEN);
            int score = minimax(depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), currentPlayer == BLACK);

            currentPlayer = prevPlayer;
            board[move.fromRow][move.fromCol] = board[move.toRow][move.toCol];
            board[move.toRow][move.toCol] = temp;
            enPassantCol = savedEnPassantCol;
            enPassantRow = savedEnPassantRow;
            whiteKingMoved = savedWhiteKing;
            blackKingMoved = savedBlackKing;
            whiteRookAMoved = savedWRA;
            whiteRookHMoved = savedWRH;
            blackRookAMoved = savedBRA;
            blackRookHMoved = savedBRH;

            if (move.isEnPassant) {
                int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                board[captureRow][move.toCol] = tempEnPassant;
            }

            // Add randomness for lower difficulties
            if (difficulty == 1) {
                score += getSmallNoise() * 20;
            } else if (difficulty == 2) {
                score += getSmallNoise() * 10;
            }

            if ((prevPlayer == WHITE && (score > bestScore || (score == bestScore && getCoinFlip()))) ||
                (prevPlayer == BLACK && (score < bestScore || (score == bestScore && getCoinFlip())))) {
                bestScore = score;
                bestMove = move;
            }
        }

        return bestMove;
    }

    int scoreMoveOrdering(const Move& move) {
        int score = 0;

        if (move.capturedPiece.type != EMPTY) {
            int pieceValues[7] = {0, 100, 320, 330, 500, 900, 20000};
            Piece attacker = board[move.fromRow][move.fromCol];
            score = 10 * pieceValues[move.capturedPiece.type] - pieceValues[attacker.type];
        }

        if (move.isPromotion) score += 800;

        if (move.toRow >= 3 && move.toRow <= 4 && move.toCol >= 3 && move.toCol <= 4) {
            score += 20;
        }

        Piece tempPiece = board[move.toRow][move.toCol];
        Piece movingPiece = board[move.fromRow][move.fromCol];
        board[move.toRow][move.toCol] = movingPiece;
        board[move.fromRow][move.fromCol] = {EMPTY, NONE, false};

        Color opponent = (currentPlayer == WHITE) ? BLACK : WHITE;
        if (isSquareAttacked(move.toRow, move.toCol, opponent)) {
            score -= 50;
        }

        board[move.fromRow][move.fromCol] = movingPiece;
        board[move.toRow][move.toCol] = tempPiece;

        return score;
    }

    int minimax(int depth, int alpha, int beta, bool maximizing) {
        if (depth == 0) return quiescence(alpha, beta, 2);

        std::vector<Move> allMoves;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color == currentPlayer) {
                    std::vector<Move> moves = getLegalMoves(r, c);
                    allMoves.insert(allMoves.end(), moves.begin(), moves.end());
                }
            }
        }

        if (allMoves.empty()) {
            if (isInCheck(currentPlayer)) return maximizing ? -999999 : 999999;
            return 0;
        }

        for (Move& move : allMoves) {
            move.score = scoreMoveOrdering(move);
        }

        std::sort(allMoves.begin(), allMoves.end(), [](const Move& a, const Move& b) {
            return a.score > b.score;
        });

        if (maximizing) {
            int maxEval = std::numeric_limits<int>::min();
            for (const Move& move : allMoves) {
                Piece temp = board[move.toRow][move.toCol];
                Piece tempEnPassant = {EMPTY, NONE, false};
                int savedEnPassantCol = enPassantCol;
                int savedEnPassantRow = enPassantRow;
                Color prevPlayer = currentPlayer;
                bool savedWhiteKing = whiteKingMoved, savedBlackKing = blackKingMoved;
                bool savedWRA = whiteRookAMoved, savedWRH = whiteRookHMoved;
                bool savedBRA = blackRookAMoved, savedBRH = blackRookHMoved;

                if (move.isEnPassant) {
                    int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                    tempEnPassant = board[captureRow][move.toCol];
                }

                executeMoveInternal(move, QUEEN);
                int eval = minimax(depth - 1, alpha, beta, false);

                currentPlayer = prevPlayer;
                board[move.fromRow][move.fromCol] = board[move.toRow][move.toCol];
                board[move.toRow][move.toCol] = temp;
                enPassantCol = savedEnPassantCol;
                enPassantRow = savedEnPassantRow;
                whiteKingMoved = savedWhiteKing;
                blackKingMoved = savedBlackKing;
                whiteRookAMoved = savedWRA;
                whiteRookHMoved = savedWRH;
                blackRookAMoved = savedBRA;
                blackRookHMoved = savedBRH;

                if (move.isEnPassant) {
                    int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                    board[captureRow][move.toCol] = tempEnPassant;
                }

                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        } else {
            int minEval = std::numeric_limits<int>::max();
            for (const Move& move : allMoves) {
                Piece temp = board[move.toRow][move.toCol];
                Piece tempEnPassant = {EMPTY, NONE, false};
                int savedEnPassantCol = enPassantCol;
                int savedEnPassantRow = enPassantRow;
                Color prevPlayer = currentPlayer;
                bool savedWhiteKing = whiteKingMoved, savedBlackKing = blackKingMoved;
                bool savedWRA = whiteRookAMoved, savedWRH = whiteRookHMoved;
                bool savedBRA = blackRookAMoved, savedBRH = blackRookHMoved;

                if (move.isEnPassant) {
                    int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                    tempEnPassant = board[captureRow][move.toCol];
                }

                executeMoveInternal(move, QUEEN);
                int eval = minimax(depth - 1, alpha, beta, true);

                currentPlayer = prevPlayer;
                board[move.fromRow][move.fromCol] = board[move.toRow][move.toCol];
                board[move.toRow][move.toCol] = temp;
                enPassantCol = savedEnPassantCol;
                enPassantRow = savedEnPassantRow;
                whiteKingMoved = savedWhiteKing;
                blackKingMoved = savedBlackKing;
                whiteRookAMoved = savedWRA;
                whiteRookHMoved = savedWRH;
                blackRookAMoved = savedBRA;
                blackRookHMoved = savedBRH;

                if (move.isEnPassant) {
                    int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                    board[captureRow][move.toCol] = tempEnPassant;
                }

                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                if (beta <= alpha) break;
            }
            return minEval;
        }
    }

    int quiescence(int alpha, int beta, int depth) {
        int standPat = evaluateBoard();

        if (depth == 0) return standPat;

        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;

        std::vector<Move> captureMoves;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color == currentPlayer) {
                    std::vector<Move> moves = getLegalMoves(r, c);
                    for (const Move& move : moves) {
                        if (move.isCapture || move.isPromotion) {
                            captureMoves.push_back(move);
                        }
                    }
                }
            }
        }

        for (Move& move : captureMoves) {
            move.score = scoreMoveOrdering(move);
        }

        std::sort(captureMoves.begin(), captureMoves.end(), [](const Move& a, const Move& b) {
            return a.score > b.score;
        });

        for (const Move& move : captureMoves) {
            Piece temp = board[move.toRow][move.toCol];
            Color prevPlayer = currentPlayer;
            int savedEnPassantCol = enPassantCol;
            int savedEnPassantRow = enPassantRow;

            executeMoveInternal(move, QUEEN);
            int score = -quiescence(-beta, -alpha, depth - 1);

            currentPlayer = prevPlayer;
            board[move.fromRow][move.fromCol] = board[move.toRow][move.toCol];
            board[move.toRow][move.toCol] = temp;
            enPassantCol = savedEnPassantCol;
            enPassantRow = savedEnPassantRow;

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    int getCurrentPlayer() { return currentPlayer; }

    bool isGameOver() {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c].color == currentPlayer) {
                    if (!getLegalMoves(r, c).empty()) return false;
                }
            }
        }
        return true;
    }

    bool isCheckmate() {
        return isGameOver() && isInCheck(currentPlayer);
    }

    bool isStalemate() {
        return isGameOver() && !isInCheck(currentPlayer);
    }
};

ChessGame* game = nullptr;
int aiDifficulty = 2;

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_initGame(JNIEnv* env, jobject thiz) {
    if (game) {
        delete game;
        game = nullptr;
    }
    game = new ChessGame();
    __android_log_print(ANDROID_LOG_INFO, "ChessGame", "Game initialized successfully");
}

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_setDifficulty(JNIEnv* env, jobject, jint difficulty) {
    aiDifficulty = difficulty;
}

extern "C" JNIEXPORT jint JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getPiece(JNIEnv* env, jobject, jint row, jint col) {
    if (!game) {
        __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Game is null in getPiece");
        return 0;
    }
    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Invalid coordinates: %d, %d", row, col);
        return 0;
    }
    return game->getPiece(row, col);
}

extern "C" JNIEXPORT jintArray JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getLegalMoves(JNIEnv* env, jobject, jint row, jint col) {
    if (!game) {
        __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Game is null in getLegalMoves");
        return env->NewIntArray(0);
    }

    if (row < 0 || row >= 8 || col < 0 || col >= 8) {
        __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Invalid coordinates in getLegalMoves: %d, %d", row, col);
        return env->NewIntArray(0);
    }

    std::vector<Move> moves = game->getLegalMoves(row, col);
    jintArray result = env->NewIntArray(moves.size() * 2);

    if (result == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "ChessGame", "Failed to create int array");
        return nullptr;
    }

    std::vector<jint> positions;
    for (const Move& move : moves) {
        positions.push_back(move.toRow);
        positions.push_back(move.toCol);
    }

    if (!positions.empty()) {
        env->SetIntArrayRegion(result, 0, positions.size(), positions.data());
    }

    return result;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_makeMove(JNIEnv* env, jobject, jint fromRow, jint fromCol, jint toRow, jint toCol) {
    if (!game) return false;
    return game->makeMove(fromRow, fromCol, toRow, toCol);
}

extern "C" JNIEXPORT jintArray JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getComputerMove(JNIEnv* env, jobject) {
    if (!game) return env->NewIntArray(0);

    int depth = aiDifficulty + 1;
    if (aiDifficulty == 4) depth = 5;

    Move bestMove = game->getBestMove(depth, aiDifficulty);
    jintArray result = env->NewIntArray(4);
    jint moveData[4] = {bestMove.fromRow, bestMove.fromCol, bestMove.toRow, bestMove.toCol};
    env->SetIntArrayRegion(result, 0, 4, moveData);
    return result;
}

extern "C" JNIEXPORT jint JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getCurrentPlayer(JNIEnv* env, jobject) {
    if (!game) return 1;
    return game->getCurrentPlayer();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_isGameOver(JNIEnv* env, jobject) {
    if (!game) return false;
    return game->isGameOver();
}

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_cleanupGame(JNIEnv* env, jobject thiz) {
    if (game) {
        delete game;
        game = nullptr;
    }
    __android_log_print(ANDROID_LOG_INFO, "ChessGame", "Game cleaned up");
}