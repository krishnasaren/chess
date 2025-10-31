package elrasseo.syreao.checkmate;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

public class ChessBoardCustomView extends View {
    private int selectedRow = -1;
    private int selectedCol = -1;
    private int[] highlightedMoves = new int[0];
    private MainActivity activity;

    private static final String[] PIECE_UNICODE = {
            "",     // 0 = EMPTY
            "♙",    // 1 = WHITE PAWN (not used directly because getPiece encodes color*10+type)
            "♘",    // 2 = WHITE KNIGHT
            "♗",    // 3 = WHITE BISHOP
            "♖",    // 4 = WHITE ROOK
            "♕",    // 5 = WHITE QUEEN
            "♔",    // 6 = WHITE KING
            "",     // 7
            "",     // 8
            "",     // 9
            "",     // 10
            "♙",    // 11 = WHITE PAWN
            "♘",    // 12 = WHITE KNIGHT
            "♗",    // 13 = WHITE BISHOP
            "♖",    // 14 = WHITE ROOK
            "♕",    // 15 = WHITE QUEEN
            "♔",    // 16 = WHITE KING
            "",     // 17
            "",     // 18
            "",     // 19
            "",     // 20
            "♟",    // 21 = BLACK PAWN
            "♞",    // 22 = BLACK KNIGHT
            "♝",    // 23 = BLACK BISHOP
            "♜",    // 24 = BLACK ROOK
            "♛",    // 25 = BLACK QUEEN
            "♚"     // 26 = BLACK KING
    };

    public ChessBoardCustomView(Context context) {
        super(context);
        init();
    }

    public ChessBoardCustomView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public ChessBoardCustomView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        setOnTouchListener((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                handleTouch(event.getX(), event.getY());
                return true;
            }
            return false;
        });
    }

    public void setActivity(MainActivity activity) {
        this.activity = activity;
    }

    public void reset() {
        selectedRow = -1;
        selectedCol = -1;
        highlightedMoves = new int[0];
        invalidate();
    }

    private void handleTouch(float x, float y) {
        if (activity == null) return;

        int boardSize = Math.min(getWidth(), getHeight());
        int squareSize = boardSize / 8;
        int offsetX = (getWidth() - boardSize) / 2;
        int offsetY = (getHeight() - boardSize) / 2;

        int col = (int)((x - offsetX) / squareSize);
        int row = (int)((y - offsetY) / squareSize);

        if (row < 0 || row >= 8 || col < 0 || col >= 8) return;

        // Check if game is over
        if (activity.isGameOver()) {
            Toast.makeText(getContext(), "Game is over! Start a new game.", Toast.LENGTH_SHORT).show();
            return;
        }

        // Check if it's computer's turn
        if (activity.isVsComputer() && activity.getCurrentPlayer() == 2) {
            return;
        }

        int currentPlayer = activity.getCurrentPlayer();
        int piece = activity.getPiece(row, col);

        // FIXED: get piece color from encoded value color*10 + type => color = piece / 10
        int pieceColor = (piece == 0) ? 0 : (piece / 10);

        if (selectedRow == -1) {
            // First selection
            if (piece != 0 && pieceColor == currentPlayer) {
                selectedRow = row;
                selectedCol = col;
                highlightedMoves = activity.getLegalMoves(row, col);
                invalidate();
            }
        } else {
            // Check if clicking highlighted square
            boolean isValidMove = false;
            for (int i = 0; i < highlightedMoves.length; i += 2) {
                if (highlightedMoves[i] == row && highlightedMoves[i + 1] == col) {
                    isValidMove = true;
                    break;
                }
            }

            if (isValidMove) {
                // Make move
                if (activity.makeMove(selectedRow, selectedCol, row, col)) {
                    selectedRow = -1;
                    selectedCol = -1;
                    highlightedMoves = new int[0];
                    invalidate();
                    activity.updateStatus();
                }
            } else if (piece != 0 && pieceColor == currentPlayer) {
                // Select different piece
                selectedRow = row;
                selectedCol = col;
                highlightedMoves = activity.getLegalMoves(row, col);
                invalidate();
            } else {
                // Deselect
                selectedRow = -1;
                selectedCol = -1;
                highlightedMoves = new int[0];
                invalidate();
            }
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        if (activity == null) return;

        int boardSize = Math.min(getWidth(), getHeight());
        int squareSize = boardSize / 8;
        int offsetX = (getWidth() - boardSize) / 2;
        int offsetY = (getHeight() - boardSize) / 2;

        Paint paint = new Paint();
        paint.setAntiAlias(true);

        // Draw board
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int x = offsetX + col * squareSize;
                int y = offsetY + row * squareSize;

                // Square color
                if ((row + col) % 2 == 0) {
                    paint.setColor(Color.parseColor("#f0d9b5"));
                } else {
                    paint.setColor(Color.parseColor("#b58863"));
                }

                // Highlight selected square
                if (row == selectedRow && col == selectedCol) {
                    paint.setColor(Color.parseColor("#00d4ff"));
                    paint.setAlpha(200);
                }

                canvas.drawRect(x, y, x + squareSize, y + squareSize, paint);
                paint.setAlpha(255);

                // Draw piece
                int piece = activity.getPiece(row, col);
                if (piece > 0 && piece < PIECE_UNICODE.length) {
                    // determine color from encoded piece (color*10 + type)
                    int colorCode = (piece == 0) ? 0 : (piece / 10);
                    // set piece text color for visibility (black for black pieces, white for white pieces)
                    if (colorCode == 1) {
                        paint.setColor(Color.WHITE);
                    } else {
                        paint.setColor(Color.BLACK);
                    }

                    paint.setTextSize(squareSize * 0.7f);
                    paint.setTextAlign(Paint.Align.CENTER);

                    String pieceChar = PIECE_UNICODE[piece];
                    float textX = x + squareSize / 2f;
                    float textY = y + squareSize / 2f + squareSize * 0.25f;

                    canvas.drawText(pieceChar, textX, textY, paint);
                }
            }
        }

        // Draw highlighted moves
        paint.setColor(Color.parseColor("#7dff00"));
        paint.setAlpha(150);
        for (int i = 0; i < highlightedMoves.length; i += 2) {
            int row = highlightedMoves[i];
            int col = highlightedMoves[i + 1];
            int x = offsetX + col * squareSize;
            int y = offsetY + row * squareSize;

            canvas.drawCircle(x + squareSize / 2f, y + squareSize / 2f, squareSize / 6f, paint);
        }

        // Draw border
        paint.setAlpha(255);
        paint.setColor(Color.parseColor("#16213e"));
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(8);
        canvas.drawRect(offsetX, offsetY, offsetX + boardSize, offsetY + boardSize, paint);
    }
}
