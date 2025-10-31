#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <random>



static std::mt19937 rng(std::random_device{}()); // High-quality random generator
static std::uniform_int_distribution<int> smallNoise(0, 2); // Used instead of rand()%3
static std::uniform_int_distribution<int> coinFlip(0, 1);   // Used for tie-breaking


// Chess piece types
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

public:
    ChessGame() {
        initializeBoard();
    }

    void initializeBoard() {
        // Clear board
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                board[i][j] = {EMPTY, NONE, false};
            }
        }

        // Setup black pieces
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

        // Setup white pieces
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
    }

    int getPiece(int row, int col) {
        if (row < 0 || row >= 8 || col < 0 || col >= 8) return 0;
        Piece p = board[row][col];
        if (p.type == EMPTY) return 0;
        // Encode as: (color * 10) + type
        // WHITE (1): 11=pawn, 12=knight, 13=bishop, 14=rook, 15=queen, 16=king
        // BLACK (2): 21=pawn, 22=knight, 23=bishop, 24=rook, 25=queen, 26=king
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

        // Forward move
        if (isValidPosition(row + direction, col) && board[row + direction][col].type == EMPTY) {
            moves.push_back({row, col, row + direction, col, false, false, false,
                             (row + direction == 0 || row + direction == 7), 0});

            // Double move from start
            if (row == startRow && board[row + 2 * direction][col].type == EMPTY) {
                moves.push_back({row, col, row + 2 * direction, col, false, false, false, false, 0});
            }
        }

        // Captures
        for (int dcol : {-1, 1}) {
            int newRow = row + direction;
            int newCol = col + dcol;
            if (isValidPosition(newRow, newCol)) {
                Piece target = board[newRow][newCol];
                if (target.type != EMPTY && target.color != piece.color) {
                    moves.push_back({row, col, newRow, newCol, true, false, false,
                                     (newRow == 0 || newRow == 7), 0});
                }
                // En passant
                if (newCol == enPassantCol && newRow == enPassantRow) {
                    moves.push_back({row, col, newRow, newCol, true, false, true, false, 0});
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
                    moves.push_back({row, col, newRow, newCol, target.type != EMPTY, false, false, false, 0});
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
                    moves.push_back({row, col, newRow, newCol, false, false, false, false, 0});
                } else {
                    if (target.color != piece.color) {
                        moves.push_back({row, col, newRow, newCol, true, false, false, false, 0});
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
                    moves.push_back({row, col, newRow, newCol, target.type != EMPTY, false, false, false, 0});
                }
            }
        }

        // Castling
        if (piece.color == WHITE && !whiteKingMoved) {
            // Kingside
            if (!whiteRookHMoved && board[7][5].type == EMPTY && board[7][6].type == EMPTY) {
                moves.push_back({7, 4, 7, 6, false, true, false, false, 0});
            }
            // Queenside
            if (!whiteRookAMoved && board[7][1].type == EMPTY && board[7][2].type == EMPTY && board[7][3].type == EMPTY) {
                moves.push_back({7, 4, 7, 2, false, true, false, false, 0});
            }
        } else if (piece.color == BLACK && !blackKingMoved) {
            // Kingside
            if (!blackRookHMoved && board[0][5].type == EMPTY && board[0][6].type == EMPTY) {
                moves.push_back({0, 4, 0, 6, false, true, false, false, 0});
            }
            // Queenside
            if (!blackRookAMoved && board[0][1].type == EMPTY && board[0][2].type == EMPTY && board[0][3].type == EMPTY) {
                moves.push_back({0, 4, 0, 2, false, true, false, false, 0});
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
                            int rr = r + direction;
                            int cc = c + dc;
                            if (rr == row && cc == col)
                                return true;
                        }
                    } else {
                        std::vector<Move> moves = getPseudoLegalMoves(r, c);
                        for (const Move& move : moves) {
                            if (move.toRow == row && move.toCol == col && !move.isCastling)
                                return true;
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
        // Save game state
        Piece tempPiece = board[move.toRow][move.toCol];
        Piece movingPiece = board[move.fromRow][move.fromCol];
        int tempEnPassantCol = enPassantCol;
        int tempEnPassantRow = enPassantRow;
        Color savedPlayer = currentPlayer;

        // Make the move temporarily
        board[move.toRow][move.toCol] = movingPiece;
        board[move.fromRow][move.fromCol] = {EMPTY, NONE, false};

        // Handle en passant capture
        Piece capturedPawn = {EMPTY, NONE, false};
        if (move.isEnPassant) {
            int captureRow = (movingPiece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            capturedPawn = board[captureRow][move.toCol];
            board[captureRow][move.toCol] = {EMPTY, NONE, false};
        }

        // Handle castling
        Piece savedRook = {EMPTY, NONE, false};
        int rookFromCol = -1, rookToCol = -1;
        if (move.isCastling) {
            int row = move.fromRow;
            if (move.toCol == 6) { // Kingside
                rookFromCol = 7;
                rookToCol = 5;
            } else { // Queenside
                rookFromCol = 0;
                rookToCol = 3;
            }
            savedRook = board[row][rookToCol];
            board[row][rookToCol] = board[row][rookFromCol];
            board[row][rookFromCol] = {EMPTY, NONE, false};
        }

        // ✅ Do NOT switch player here.
        // Just check if this move leaves our own king in check
        bool legal = !isInCheck(savedPlayer);

        // Restore board state
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

        // Handle en passant
        if (move.isEnPassant) {
            int captureRow = (piece.color == WHITE) ? move.toRow + 1 : move.toRow - 1;
            board[captureRow][move.toCol] = {EMPTY, NONE, false};
        }

        // Handle castling
        if (move.isCastling) {
            if (move.toCol == 6) { // Kingside
                board[move.fromRow][5] = board[move.fromRow][7];
                board[move.fromRow][7] = {EMPTY, NONE, false};
            } else { // Queenside
                board[move.fromRow][3] = board[move.fromRow][0];
                board[move.fromRow][0] = {EMPTY, NONE, false};
            }
        }

        // Update en passant
        enPassantCol = -1;
        if (piece.type == PAWN && abs(move.fromRow - move.toRow) == 2) {
            enPassantCol = move.fromCol;
            enPassantRow = (move.fromRow + move.toRow) / 2;
        }

        // Execute move
        board[move.toRow][move.toCol] = piece;
        board[move.fromRow][move.fromCol] = {EMPTY, NONE, false};
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

        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
    }

    int evaluateBoard() {
        int score = 0;
        int pieceValues[7] = {0, 100, 320, 330, 500, 900, 20000};

        // Position tables for better evaluation
        int pawnTable[8][8] = {
                {0,  0,  0,  0,  0,  0,  0,  0},
                {50, 50, 50, 50, 50, 50, 50, 50},
                {10, 10, 20, 30, 30, 20, 10, 10},
                {5,  5, 10, 25, 25, 10,  5,  5},
                {0,  0,  0, 20, 20,  0,  0,  0},
                {5, -5,-10,  0,  0,-10, -5,  5},
                {5, 10, 10,-20,-20, 10, 10,  5},
                {0,  0,  0,  0,  0,  0,  0,  0}
        };

        int knightTable[8][8] = {
                {-50,-40,-30,-30,-30,-30,-40,-50},
                {-40,-20,  0,  0,  0,  0,-20,-40},
                {-30,  0, 10, 15, 15, 10,  0,-30},
                {-30,  5, 15, 20, 20, 15,  5,-30},
                {-30,  0, 15, 20, 20, 15,  0,-30},
                {-30,  5, 10, 15, 15, 10,  5,-30},
                {-40,-20,  0,  5,  5,  0,-20,-40},
                {-50,-40,-30,-30,-30,-30,-40,-50}
        };

        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                Piece piece = board[r][c];
                if (piece.type != EMPTY) {
                    int value = pieceValues[piece.type];

                    // Add positional bonus
                    if (piece.type == PAWN) {
                        value += (piece.color == WHITE) ? pawnTable[r][c] : pawnTable[7-r][c];
                    } else if (piece.type == KNIGHT) {
                        value += (piece.color == WHITE) ? knightTable[r][c] : knightTable[7-r][c];
                    }

                    // Center control bonus
                    if ((r >= 3 && r <= 4) && (c >= 3 && c <= 4)) {
                        value += 10;
                    }

                    if (piece.color == WHITE) score += value;
                    else score -= value;
                }
            }
        }

        return score;
    }

    Move getBestMove(int depth) {
        // --- Random seed once (if not already in constructor) ---
        /*static bool seeded = false;
        if (!seeded) {
            srand(static_cast<unsigned int>(time(nullptr)));
            seeded = true;
        }*/

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
            return {-1, -1, -1, -1, false, false, false, false, 0};
        }

        // --- Quick evaluation for move ordering ---
        for (Move& move : allMoves) {
            move.score = 0;

            Piece captured = board[move.toRow][move.toCol];
            if (captured.type != EMPTY) {
                move.score = 100; // Prioritize captures
            }

            if (move.toRow >= 3 && move.toRow <= 4 && move.toCol >= 3 && move.toCol <= 4) {
                move.score += 10; // Center control
            }

            // Small random factor to make AI less predictable
            //move.score += rand() % 3;
            move.score += smallNoise(rng);

            // --- Reward moves that put opponent in check (safe version) ---
            Piece tempPiece = board[move.toRow][move.toCol];
            Piece movingPiece = board[move.fromRow][move.fromCol];
            Color prevPlayer = currentPlayer;
            int savedEnPassantCol = enPassantCol;
            int savedEnPassantRow = enPassantRow;

            executeMoveInternal(move, QUEEN);
            bool givesCheck = isInCheck((prevPlayer == WHITE) ? BLACK : WHITE);

            // Restore state manually
            currentPlayer = prevPlayer;
            board[move.fromRow][move.fromCol] = movingPiece;
            board[move.toRow][move.toCol] = tempPiece;
            enPassantCol = savedEnPassantCol;
            enPassantRow = savedEnPassantRow;

            if (move.isEnPassant) {
                int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                board[captureRow][move.toCol] = {PAWN, (prevPlayer == WHITE) ? BLACK : WHITE, false};
            }

            if (givesCheck) move.score += 20;

            // Slight bonus for pawn advancement
            if (board[move.fromRow][move.fromCol].type == PAWN) {
                int advance = (currentPlayer == WHITE) ? (7 - move.toRow) : move.toRow;
                move.score += advance / 2;
            }

            // Big bonus if promotion
            if (move.isPromotion) move.score += 80;
        }

        // --- Sort moves by score (best first for better pruning) ---
        std::sort(allMoves.begin(), allMoves.end(), [](const Move& a, const Move& b) {
            return a.score > b.score;
        });

        Move bestMove = allMoves[0];
        int bestScore = (currentPlayer == WHITE) ? -999999 : 999999;

        // --- Evaluate each move ---
        for (Move& move : allMoves) {
            // Save state
            Piece temp = board[move.toRow][move.toCol];
            Piece tempEnPassant = {EMPTY, NONE, false};
            int savedEnPassantCol = enPassantCol;
            int savedEnPassantRow = enPassantRow;
            Color prevPlayer = currentPlayer;
            bool savedWhiteKing = whiteKingMoved, savedBlackKing = blackKingMoved;
            bool savedWRA = whiteRookAMoved, savedWRH = whiteRookHMoved;
            bool savedBRA = blackRookAMoved, savedBRH = blackRookHMoved;

            // Handle en passant
            if (move.isEnPassant) {
                int captureRow = (prevPlayer == WHITE) ? move.toRow + 1 : move.toRow - 1;
                tempEnPassant = board[captureRow][move.toCol];
            }

            // Execute move temporarily
            executeMoveInternal(move, QUEEN);

            // Evaluate with minimax (negamax style)
            int score = minimax(depth - 1, -999999, 999999, currentPlayer == BLACK);

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

            // --- Updated tie-breaking logic ---
            if ((prevPlayer == WHITE && (score > bestScore || (score == bestScore && coinFlip(rng)))) ||
                (prevPlayer == BLACK && (score < bestScore || (score == bestScore && coinFlip(rng))))) {
                bestScore = score;
                bestMove = move;
            }
        }

        return bestMove;
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

        if (maximizing) {
            int maxEval = -999999;
            for (const Move& move : allMoves) {
                // Save state
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

                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                if (beta <= alpha) break;
            }
            return maxEval;
        } else {
            int minEval = 999999;
            for (const Move& move : allMoves) {
                // Save state
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

ChessGame* game = nullptr;
int aiDifficulty = 2; // 1=Easy, 2=Medium, 3=Hard

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_initGame(JNIEnv* env, jobject) {
    if (game) delete game;
    game = new ChessGame();
}

extern "C" JNIEXPORT void JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_setDifficulty(JNIEnv* env, jobject, jint difficulty) {
    aiDifficulty = difficulty;
}

extern "C" JNIEXPORT jint JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getPiece(JNIEnv* env, jobject, jint row, jint col) {
    if (!game) return 0;
    return game->getPiece(row, col);
}

extern "C" JNIEXPORT jintArray JNICALL
Java_elrasseo_syreao_checkmate_MainActivity_getLegalMoves(JNIEnv* env, jobject, jint row, jint col) {
    if (!game) return env->NewIntArray(0);

    std::vector<Move> moves = game->getLegalMoves(row, col);
    jintArray result = env->NewIntArray(moves.size() * 2);

    std::vector<jint> positions;
    for (const Move& move : moves) {
        positions.push_back(move.toRow);
        positions.push_back(move.toCol);
    }

    env->SetIntArrayRegion(result, 0, positions.size(), positions.data());
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

    // Adjust depth based on difficulty
    int depth = aiDifficulty; // 1=Easy (depth 1), 2=Medium (depth 2), 3=Hard (depth 3)

    Move bestMove = game->getBestMove(depth);
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