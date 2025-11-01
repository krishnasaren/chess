#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>
#include <limits>
#include <cstdlib>
#include <android/log.h>

#define LOG_TAG "ChessNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Thread-safe random number generation
class RandomGenerator {
private:
    std::mt19937 rng;
    
public:
    RandomGenerator() : rng(static_cast<unsigned int>(time(nullptr))) {}
    
    int getSmallNoise() {
        std::uniform_int_distribution<int> dist(0, 5);
        return dist(rng);
    }
    
    int getCoinFlip() {
        std::uniform_int_distribution<int> dist(0, 1);
        return dist(rng);
    }
};

static RandomGenerator* g_random = nullptr;

void ensureRandomInit() {
    if (!g_random) {
        g_random = new RandomGenerator();
    }
}

enum PieceType { 
    EMPTY = 0, 
    PAWN = 1, 
    KNIGHT = 2, 
    BISHOP = 3, 
    ROOK = 4, 
    QUEEN = 5, 
    KING = 6 
};

enum Color { 
    NONE = 0, 
    WHITE = 1, 
    BLACK = 2 
};

struct Piece {
    PieceType type;
    Color color;
    bool hasMoved;
    
    Piece() : type(EMPTY), color(NONE), hasMoved(false) {}
    Piece(PieceType t, Color c, bool m) : type(t), color(c), hasMoved(m) {}
};

struct Move {
    int fromRow, fromCol, toRow, toCol;
    bool isCapture;
    bool isCastling;
    bool isEnPassant;
    bool isPromotion;
    int score;
    Piece capturedPiece;
    
    Move() : fromRow(-1), fromCol(-1), toRow(-1), toCol(-1), 
             isCapture(false), isCastling(false), isEnPassant(false), 
             isPromotion(false), score(0) {}
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
    bool initialized;

    // Piece-square tables
    static const int pawnTableWhite[8][8];
    static const int knightTable[8][8];
    static const int bishopTable[8][8];
    static const int rookTable[8][8];
    static const int queenTable[8][8];
    static const int kingTableMiddle[8][8];
    static const int kingTableEnd[8][8];

public:
    ChessGame() : initialized(false) {
        LOGD("ChessGame constructor called");
        ensureRandomInit();
        initializeBoard();
    }

    ~ChessGame() {
        LOGD("ChessGame destructor called");
    }

    void initializeBoard() {
        try {
            LOGD("Initializing board...");
            
            // Clear board
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    board[i][j] = Piece(EMPTY, NONE, false);
                }
            }

            // Setup black pieces
            board[0][0] = Piece(ROOK, BLACK, false);
            board[0][1] = Piece(KNIGHT, BLACK, false);
            board[0][2] = Piece(BISHOP, BLACK, false);
            board[0][3] = Piece(QUEEN, BLACK, false);
            board[0][4] = Piece(KING, BLACK, false);
            board[0][5] = Piece(BISHOP, BLACK, false);
            board[0][6] = Piece(KNIGHT, BLACK, false);
            board[0][7] = Piece(ROOK, BLACK, false);

            for (int i = 0; i < 8; i++) {
                board[1][i] = Piece(PAWN, BLACK, false);
            }

            // Setup white pieces
            board[7][0] = Piece(ROOK, WHITE, false);
            board[7][1] = Piece(KNIGHT, WHITE, false);
            board[7][2] = Piece(BISHOP, WHITE, false);
            board[7][3] = Piece(QUEEN, WHITE, false);
            board[7][4] = Piece(KING, WHITE, false);
            board[7][5] = Piece(BISHOP, WHITE, false);
            board[7][6] = Piece(KNIGHT, WHITE, false);
            board[7][7] = Piece(ROOK, WHITE, false);

            for (int i = 0; i < 8; i++) {
                board[6][i] = Piece(PAWN, WHITE, false);
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
            initialized = true;
            
            LOGD("Board initialized successfully");
        } catch (const std::exception& e) {
            LOGE("Exception in initializeBoard: %s", e.what());
            initialized = false;
        } catch (...) {
            LOGE("Unknown exception in initializeBoard");
            initialized = false;
        }
    }

    bool isInitialized() const {
        return initialized;
    }

    int getPiece(int row, int col) {
        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            LOGE("Invalid board access: %d, %d", row, col);
            return 0;
        }
        
        try {
            Piece p = board[row][col];
            if (p.type == EMPTY) return 0;
            return (p.color * 10) + p.type;
        } catch (...) {
            LOGE("Exception in getPiece");
            return 0;
        }
    }

    bool isValidPosition(int row, int col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    std::vector<Move> getLegalMoves(int row, int col) {
        std::vector<Move> moves;
        try {
            if (!initialized || !isValidPosition(row, col)) return moves;

            Piece piece = board[row][col];
            if (piece.type == EMPTY || piece.color != currentPlayer) return moves;

            std::vector<Move> pseudoMoves = getPseudoLegalMoves(row, col);

            for (const Move& move : pseudoMoves) {
                if (isLegalMove(move)) {
                    moves.push_back(move);
                }
            }
        } catch (const std::exception& e) {
            LOGE("Exception in getLegalMoves: %s", e.what());
        } catch (...) {
            LOGE("Unknown exception in getLegalMoves");
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
            default:
                break;
        }

        return moves;
    }

    void getPawnMoves(int row, int col, std::vector<Move>& moves) {
        Piece piece = board[row][col];
        int direction = (piece.color == WHITE) ? -1 : 1;
        int startRow = (piece.color == WHITE) ? 6 : 1;

        // Forward move
        if (isValidPosition(row + direction, col) && board[row + direction][col].type == EMPTY) {
            bool isPromotion = (row + direction == 0 || row + direction == 7);
            Move move;
            move.fromRow = row;
            move.fromCol = col;
            move.toRow = row + direction;
            move.toCol = col;
            move.isPromotion = isPromotion;
            moves.push_back(move);

            // Double forward
            if (row == startRow && board[row + 2 * direction][col].type == EMPTY) {
                Move doubleMove;
                doubleMove.fromRow = row;
                doubleMove.fromCol = col;
                doubleMove.toRow = row + 2 * direction;
                doubleMove.toCol = col;
                moves.push_back(doubleMove);
            }
        }

        // Captures
        for (int dcol : {-1, 1}) {
            int newRow = row + direction;
            int newCol = col + dcol;
            if (isValidPosition(newRow, newCol)) {
                Piece target = board[newRow][newCol];
                if (target.type != EMPTY && target.color != piece.color) {
                    bool isPromotion = (newRow == 0 || newRow == 7);
                    Move move;
                    move.fromRow = row;
                    move.fromCol = col;
                    move.toRow = newRow;
                    move.toCol = newCol;
                    move.isCapture = true;
                    move.isPromotion = isPromotion;
                    move.capturedPiece = target;
                    moves.push_back(move);
                }
                // En passant
                if (newCol == enPassantCol && newRow == enPassantRow) {
                    Move move;
                    move.fromRow = row;
                    move.fromCol = col;
                    move.toRow = newRow;
                    move.toCol = newCol;
                    move.isCapture = true;
                    move.isEnPassant = true;
                    move.capturedPiece = Piece(PAWN, (piece.color == WHITE ? BLACK : WHITE), true);
                    moves.push_back(move);
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
                    Move move;
                    move.fromRow = row;
                    move.fromCol = col;
                    move.toRow = newRow;
                    move.toCol = newCol;
                    move.isCapture = (target.type != EMPTY);
                    move.capturedPiece = target;
                    moves.push_back(move);
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
                Move move;
                move.fromRow = row;
                move.fromCol = col;
                move.toRow = newRow;
                move.toCol = newCol;
                
                if (target.type == EMPTY) {
                    moves.push_back(move);
                } else {
                    if (target.color != piece.color) {
                        move.isCapture = true;
                        move.capturedPiece = target;
                        moves.push_back(move);
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
                    Move move;
                    move.fromRow = row;
                    move.fromCol = col;
                    move.toRow = newRow;
                    move.toCol = newCol;
                    move.isCapture = (target.type != EMPTY);
                    move.capturedPiece = target;
                    moves.push_back(move);
                }
            }
        }

        // Castling
        if (!isInCheck(piece.color)) {
            if (piece.color == WHITE && !whiteKingMoved) {
                // Kingside castling
                if (!whiteRookHMoved && board[7][5].type == EMPTY && board[7][6].type == EMPTY &&
                    !isSquareAttacked(7, 5, BLACK) && !isSquareAttacked(7, 6, BLACK)) {
                    Move move;
                    move.fromRow = 7;
                    move.fromCol = 4;
                    move.toRow = 7;
                    move.toCol = 6;
                    move.isCastling = true;
                    moves.push_back(move);
                }
                // Queenside castling
                if (!whiteRookAMoved && board[7][1].type == EMPTY && board[7][2].type == EMPTY &&
                    board[7][3].type == EMPTY && !isSquareAttacked(7, 3, BLACK) && 
                    !isSquareAttacked(7, 2, BLACK)) {
                    Move move;
                    move.fromRow = 7;
                    move.fromCol = 4;
                    move.toRow = 7;
                    move.toCol = 2;
                    move.isCastling = true;
                    moves.push_back(move);
                }
            } else if (piece.color == BLACK && !blackKingMoved) {
                // Kingside castling
                if (!blackRookHMoved && board[0][5].type == EMPTY && board[0][6].type == EMPTY &&
                    !isSquareAttacked(0, 5, WHITE) && !isSquareAttacked(0, 6, WHITE)) {
                    Move move;
                    move.fromRow = 0;
                    move.fromCol = 4;
                    move.toRow = 0;
                    move.toCol = 6;
                    move.isCastling = true;
                    moves.push_back(move);
                }
                // Queenside castling
                if (!blackRookAMoved && board[0][1].type == EMPTY && board[0][2].type == EMPTY &&
                    board[0][3].type == EMPTY && !isSquareAttacked(0, 3, WHITE) && 
                    !isSquareAttacked(0, 2, WHITE)) {
                    Move move;
                    move.fromRow = 0;
                    move.fromCol = 4;
                    move.toRow = 0;
                    move.toCol = 2;
                    move.isCastling = true;
                    moves.push_back(move);
                }
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
                            if (move.toRow == row && move.toCol == col && !move.isCastling) {
                                return true;
                            }
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
        // Save state
        Piece tempPiece = board[move.toRow][move.toCol];
        Piece movingPiece = board[move.fromRow][move.fromCol];
        int tempEnPassantCol = enPassantCol;
        int tempEnPassantRow = enPassantRow;
        Color savedPlayer = currentPlayer;

        // Make move
        board[move.toRow][move.toCol] = movingPiece;
        board[move.fromRow][move.fromCol] = Piece(EMPTY, NONE, false);

        Piece capturedPawn;
        if (move.isEnPassant) {
            int captureRow = (movingPiece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            capturedPawn = board[captureRow][move.toCol];
            board[captureRow][move.toCol] = Piece(EMPTY, NONE, false);
        }

        Piece savedRook;
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
            board[row][rookFromCol] = Piece(EMPTY, NONE, false);
        }

        bool legal = !isInCheck(savedPlayer);

        // Restore state
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
        try {
            if (!initialized) {
                LOGE("Game not initialized");
                return false;
            }

            std::vector<Move> legalMoves = getLegalMoves(fromRow, fromCol);

            for (const Move& move : legalMoves) {
                if (move.toRow == toRow && move.toCol == toCol) {
                    executeMoveInternal(move, QUEEN);
                int score = minimax(depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), currentPlayer == BLACK);

                // Restore state
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
                    score += g_random->getSmallNoise() * 20;
                } else if (difficulty == 2) {
                    score += g_random->getSmallNoise() * 10;
                }

                if ((prevPlayer == WHITE && (score > bestScore || (score == bestScore && g_random->getCoinFlip()))) ||
                    (prevPlayer == BLACK && (score < bestScore || (score == bestScore && g_random->getCoinFlip())))) {
                    bestScore = score;
                    bestMove = move;
                }
            }

            return bestMove;
        } catch (const std::exception& e) {
            LOGE("Exception in getBestMove: %s", e.what());
            return Move();
        } catch (...) {
            LOGE("Unknown exception in getBestMove");
            return Move();
        }
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

        return score;
    }

    int minimax(int depth, int alpha, int beta, bool maximizing) {
        if (depth == 0) return evaluateBoard();

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
                Piece tempEnPassant;
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
                Piece tempEnPassant;
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
};

// Initialize static members
const int ChessGame::pawnTableWhite[8][8] = {
    {0,   0,   0,   0,   0,   0,   0,   0},
    {50,  50,  50,  50,  50,  50,  50,  50},
    {10,  10,  20,  30,  30,  20,  10,  10},
    {5,   5,   10,  27,  27,  10,  5,   5},
    {0,   0,   0,   25,  25,  0,   0,   0},
    {5,   -5,  -10, 0,   0,   -10, -5,  5},
    {5,   10,  10,  -25, -25, 10,  10,  5},
    {0,   0,   0,   0,   0,   0,   0,   0}
};

const int ChessGame::knightTable[8][8] = {
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0,   5,   5,   0,   -20, -40},
    {-30, 5,   10,  15,  15,  10,  5,   -30},
    {-30, 0,   15,  20,  20,  15,  0,   -30},
    {-30, 5,   15,  20,  20,  15,  5,   -30},
    {-30, 0,   10,  15,  15,  10,  0,   -30},
    {-40, -20, 0,   0,   0,   0,   -20, -40},
    {-50, -40, -20, -30, -30, -20, -40, -50}
};

const int ChessGame::bishopTable[8][8] = {
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10, 5,   0,   0,   0,   0,   5,   -10},
    {-10, 10,  10,  10,  10,  10,  10,  -10},
    {-10, 0,   10,  10,  10,  10,  0,   -10},
    {-10, 5,   5,   10,  10,  5,   5,   -10},
    {-10, 0,   5,   10,  10,  5,   0,   -10},
    {-10, 0,   0,   0,   0,   0,   0,   -10},
    {-20, -10, -40, -10, -10, -40, -10, -20}
};

const int ChessGame::rookTable[8][8] = {
    {0,  0,  0,  5,  5,  0,  0,  0},
    {-5, 0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  -5},
    {5,  10, 10, 10, 10, 10, 10, 5},
    {0,  0,  0,  0,  0,  0,  0,  0}
};

const int ChessGame::queenTable[8][8] = {
    {-20, -10, -10, -5, -5, -10, -10, -20},
    {-10, 0,   5,   0,  0,  0,   0,   -10},
    {-10, 5,   5,   5,  5,  5,   0,   -10},
    {0,   0,   5,   5,  5,  5,   0,   -5},
    {-5,  0,   5,   5,  5,  5,   0,   -5},
    {-10, 0,   5,   5,  5,  5,   0,   -10},
    {-10, 0,   0,   0,  0,  0,   0,   -10},
    {-20, -10, -10, -5, -5, -10, -10, -20}
};

const int ChessGame::kingTableMiddle[8][8] = {
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    {20,  20,  0,   0,   0,   0,   20,  20},
    {20,  30,  10,  0,   0,   10,  30,  20}
};

const int ChessGame::kingTableEnd[8][8] = {
    {-50, -30, -30, -30, -30, -30, -30, -50},
    {-30, -30, 0,   0,   0,   0,   -30, -30},
    {-30, -10, 20,  30,  30,  20,  -10, -30},
    {-30, -10, 30,  40,  40,  30,  -10, -30},
    {-30, -10, 30,  40,  40,  30,  -10, -30},
    {-30, -10, 20,  30,  30,  20,  -10, -30},
    {-30, -20, -10, 0,   0,   -10, -20, -30},
    {-50, -40, -30, -20, -20, -30, -40, -50}
};

ChessGame* game = nullptr;
int aiDifficulty = 2;

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_initGame(JNIEnv* env, jobject thiz) {
    try {
        LOGD("initGame called");
        
        if (game) {
            LOGD("Deleting existing game");
            delete game;
            game = nullptr;
        }
        
        LOGD("Creating new game");
        game = new ChessGame();
        
        if (game && game->isInitialized()) {
            LOGD("Game initialized successfully");
        } else {
            LOGE("Game initialization failed");
            if (game) {
                delete game;
                game = nullptr;
            }
        }
    } catch (const std::exception& e) {
        LOGE("Exception in initGame: %s", e.what());
        if (game) {
            delete game;
            game = nullptr;
        }
    } catch (...) {
        LOGE("Unknown exception in initGame");
        if (game) {
            delete game;
            game = nullptr;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_setDifficulty(JNIEnv* env, jobject, jint difficulty) {
    try {
        aiDifficulty = difficulty;
        LOGD("Difficulty set to %d", difficulty);
    } catch (...) {
        LOGE("Exception in setDifficulty");
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getPiece(JNIEnv* env, jobject, jint row, jint col) {
    try {
        if (!game) {
            LOGE("Game is null in getPiece");
            return 0;
        }
        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            LOGE("Invalid coordinates: %d, %d", row, col);
            return 0;
        }
        return game->getPiece(row, col);
    } catch (const std::exception& e) {
        LOGE("Exception in getPiece: %s", e.what());
        return 0;
    } catch (...) {
        LOGE("Unknown exception in getPiece");
        return 0;
    }
}

extern "C" JNIEXPORT jintArray JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getLegalMoves(JNIEnv* env, jobject, jint row, jint col) {
    try {
        if (!game) {
            LOGE("Game is null in getLegalMoves");
            return env->NewIntArray(0);
        }

        if (row < 0 || row >= 8 || col < 0 || col >= 8) {
            LOGE("Invalid coordinates in getLegalMoves: %d, %d", row, col);
            return env->NewIntArray(0);
        }

        std::vector<Move> moves = game->getLegalMoves(row, col);
        jintArray result = env->NewIntArray(moves.size() * 2);

        if (result == nullptr) {
            LOGE("Failed to create int array");
            return env->NewIntArray(0);
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
    } catch (const std::exception& e) {
        LOGE("Exception in getLegalMoves: %s", e.what());
        return env->NewIntArray(0);
    } catch (...) {
        LOGE("Unknown exception in getLegalMoves");
        return env->NewIntArray(0);
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_makeMove(JNIEnv* env, jobject, jint fromRow, jint fromCol, jint toRow, jint toCol) {
    try {
        if (!game) {
            LOGE("Game is null in makeMove");
            return false;
        }
        return game->makeMove(fromRow, fromCol, toRow, toCol);
    } catch (const std::exception& e) {
        LOGE("Exception in makeMove: %s", e.what());
        return false;
    } catch (...) {
        LOGE("Unknown exception in makeMove");
        return false;
    }
}

extern "C" JNIEXPORT jintArray JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getComputerMove(JNIEnv* env, jobject) {
    try {
        if (!game) {
            LOGE("Game is null in getComputerMove");
            jintArray result = env->NewIntArray(4);
            jint moveData[4] = {-1, -1, -1, -1};
            env->SetIntArrayRegion(result, 0, 4, moveData);
            return result;
        }

        int depth = aiDifficulty + 1;
        if (aiDifficulty == 4) depth = 5;

        Move bestMove = game->getBestMove(depth, aiDifficulty);
        jintArray result = env->NewIntArray(4);
        jint moveData[4] = {bestMove.fromRow, bestMove.fromCol, bestMove.toRow, bestMove.toCol};
        env->SetIntArrayRegion(result, 0, 4, moveData);
        return result;
    } catch (const std::exception& e) {
        LOGE("Exception in getComputerMove: %s", e.what());
        jintArray result = env->NewIntArray(4);
        jint moveData[4] = {-1, -1, -1, -1};
        env->SetIntArrayRegion(result, 0, 4, moveData);
        return result;
    } catch (...) {
        LOGE("Unknown exception in getComputerMove");
        jintArray result = env->NewIntArray(4);
        jint moveData[4] = {-1, -1, -1, -1};
        env->SetIntArrayRegion(result, 0, 4, moveData);
        return result;
    }
}

extern "C" JNIEXPORT jint JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getCurrentPlayer(JNIEnv* env, jobject) {
    try {
        if (!game) return 1;
        return game->getCurrentPlayer();
    } catch (...) {
        LOGE("Exception in getCurrentPlayer");
        return 1;
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_isGameOver(JNIEnv* env, jobject) {
    try {
        if (!game) return false;
        return game->isGameOver();
    } catch (...) {
        LOGE("Exception in isGameOver");
        return false;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_cleanupGame(JNIEnv* env, jobject thiz) {
    try {
        LOGD("cleanupGame called");
        if (game) {
            delete game;
            game = nullptr;
            LOGD("Game cleaned up successfully");
        }
        if (g_random) {
            delete g_random;
            g_random = nullptr;
        }
    } catch (const std::exception& e) {
        LOGE("Exception in cleanupGame: %s", e.what());
    } catch (...) {
        LOGE("Unknown exception in cleanupGame");
    }
}ernal(move, promotionPiece);
                    return true;
                }
            }
        } catch (const std::exception& e) {
            LOGE("Exception in makeMove: %s", e.what());
        } catch (...) {
            LOGE("Unknown exception in makeMove");
        }
        return false;
    }

    void executeMoveInternal(const Move& move, int promotionPiece) {
        Piece piece = board[move.fromRow][move.fromCol];

        // Handle en passant capture
        if (move.isEnPassant) {
            int captureRow = (piece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            board[captureRow][move.toCol] = Piece(EMPTY, NONE, false);
        }

        // Handle castling
        if (move.isCastling) {
            if (move.toCol == 6) {
                board[move.fromRow][5] = board[move.fromRow][7];
                board[move.fromRow][7] = Piece(EMPTY, NONE, false);
                board[move.fromRow][5].hasMoved = true;
            } else {
                board[move.fromRow][3] = board[move.fromRow][0];
                board[move.fromRow][0] = Piece(EMPTY, NONE, false);
                board[move.fromRow][3].hasMoved = true;
            }
        }

        // Update en passant
        enPassantCol = -1;
        enPassantRow = -1;
        if (piece.type == PAWN && abs(move.fromRow - move.toRow) == 2) {
            enPassantCol = move.fromCol;
            enPassantRow = (move.fromRow + move.toRow) / 2;
        }

        // Update half-move clock
        if (piece.type == PAWN || move.isCapture) {
            halfMoveClock = 0;
        } else {
            halfMoveClock++;
        }

        // Move piece
        board[move.toRow][move.toCol] = piece;
        board[move.fromRow][move.fromCol] = Piece(EMPTY, NONE, false);
        board[move.toRow][move.toCol].hasMoved = true;

        // Handle promotion
        if (move.isPromotion) {
            board[move.toRow][move.toCol].type = static_cast<PieceType>(promotionPiece);
        }

        // Update castling rights
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

        // Update move counter
        if (currentPlayer == BLACK) fullMoveNumber++;
        
        // Switch player
        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
        
        // Add to history
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

                    if (piece.color == WHITE) score += value;
                    else score -= value;
                }
            }
        }

        return score;
    }

    Move getBestMove(int depth, int difficulty) {
        ensureRandomInit();
        
        try {
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
                return Move();
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
                // Save state
                Piece temp = board[move.toRow][move.toCol];
                Piece tempEnPassant;
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

                executeMoveInt
