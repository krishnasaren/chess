package elrasseo.syreao.checkmate;

import android.animation.ValueAnimator;
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
    private String currentTheme = "classic";

    private int lightSquareColor;
    private int darkSquareColor;
    private int highlightColor;
    private int moveIndicatorColor;
    private int borderColor;

    private float animationProgress = 0f;
    private ValueAnimator pulseAnimator;

    private static final String[] PIECE_UNICODE = {
            "",     // 0
            "♙",    // 1
            "♘",    // 2
            "♗",    // 3
            "♖",    // 4
            "♕",    // 5
            "♔",    // 6
            "", "", "", "",
            "♙",    // 11
            "♘",    // 12
            "♗",    // 13
            "♖",    // 14
            "♕",    // 15
            "♔",    // 16
            "", "", "", "",
            "♟",    // 21
            "♞",    // 22
            "♝",    // 23
            "♜",    // 24
            "♛",    // 25
            "♚"     // 26
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
        setTheme("classic");
        // --- Setup animations here but DON'T start it ---
        pulseAnimator = ValueAnimator.ofFloat(0f, 1f);
        pulseAnimator.setDuration(1000);
        pulseAnimator.setRepeatCount(ValueAnimator.INFINITE);
        pulseAnimator.setRepeatMode(ValueAnimator.REVERSE);
        pulseAnimator.addUpdateListener(animation -> {
            animationProgress = (float) animation.getAnimatedValue();
            invalidate();
        });
        // --- End of animation setup ---

        setOnTouchListener((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                handleTouch(event.getX(), event.getY());
                return true;
            }
            return false;
        });
    }

    public void setTheme(String theme) {
        currentTheme = theme;
        switch (theme) {
            case "dark":
                lightSquareColor = Color.parseColor("#4a4a4a");
                darkSquareColor = Color.parseColor("#2b2b2b");
                highlightColor = Color.parseColor("#00d4ff");
                moveIndicatorColor = Color.parseColor("#7dff00");
                borderColor = Color.parseColor("#000000");
                break;
            case "blue":
                lightSquareColor = Color.parseColor("#dee3e6");
                darkSquareColor = Color.parseColor("#8ca2ad");
                highlightColor = Color.parseColor("#0088ff");
                moveIndicatorColor = Color.parseColor("#00ff88");
                borderColor = Color.parseColor("#1a5490");
                break;
            case "wood":
                lightSquareColor = Color.parseColor("#f0d9b5");
                darkSquareColor = Color.parseColor("#b58863");
                highlightColor = Color.parseColor("#ff6b6b");
                moveIndicatorColor = Color.parseColor("#51cf66");
                borderColor = Color.parseColor("#654321");
                break;
            default: // classic
                lightSquareColor = Color.parseColor("#f0d9b5");
                darkSquareColor = Color.parseColor("#b58863");
                highlightColor = Color.parseColor("#00d4ff");
                moveIndicatorColor = Color.parseColor("#7dff00");
                borderColor = Color.parseColor("#16213e");
                break;
        }
        invalidate();
    }

    public void setActivity(MainActivity activity) {
        this.activity = activity;
    }

    public void reset() {
        if (pulseAnimator != null && pulseAnimator.isRunning()) {
            pulseAnimator.cancel();
        }
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

        if (activity.isGameOver()) {
            Toast.makeText(getContext(), "Game is over! Start a new game.", Toast.LENGTH_SHORT).show();
            return;
        }

        if (activity.isVsComputer() && activity.getCurrentPlayer() == 2) {
            return;
        }

        int currentPlayer = activity.getCurrentPlayer();
        int piece = activity.getPiece(row, col);
        int pieceColor = (piece == 0) ? 0 : (piece / 10);

        if (selectedRow == -1) {
            if (piece != 0 && pieceColor == currentPlayer) {
                selectedRow = row;
                selectedCol = col;
                highlightedMoves = activity.getLegalMoves(row, col);
                if (!pulseAnimator.isRunning()) { // <-- START ANIMATION
                    pulseAnimator.start();
                }
                invalidate();
            }
        } else {
            boolean isValidMove = false;
            for (int i = 0; i < highlightedMoves.length; i += 2) {
                if (highlightedMoves[i] == row && highlightedMoves[i + 1] == col) {
                    isValidMove = true;
                    break;
                }
            }

            if (isValidMove) {
                if (activity.makeMove(selectedRow, selectedCol, row, col)) {
                    if (pulseAnimator.isRunning()) { // <-- STOP ANIMATION
                        pulseAnimator.cancel();
                    }
                    selectedRow = -1;
                    selectedCol = -1;
                    highlightedMoves = new int[0];
                    invalidate();
                    activity.updateStatus();
                }
            } else if (piece != 0 && pieceColor == currentPlayer) {
                selectedRow = row;
                selectedCol = col;
                highlightedMoves = activity.getLegalMoves(row, col);
                if (!pulseAnimator.isRunning()) { // <-- START ANIMATION (for re-selection)
                    pulseAnimator.start();
                }
                invalidate();
            } else {
                if (pulseAnimator.isRunning()) { // <-- STOP ANIMATION
                    pulseAnimator.cancel();
                }
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

        // Draw coordinates
        drawCoordinates(canvas, paint, offsetX, offsetY, squareSize, boardSize);

        // Draw board squares
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                int x = offsetX + col * squareSize;
                int y = offsetY + row * squareSize;

                // Square color
                if ((row + col) % 2 == 0) {
                    paint.setColor(lightSquareColor);
                } else {
                    paint.setColor(darkSquareColor);
                }

                // Highlight selected square with pulsing effect
                if (row == selectedRow && col == selectedCol) {
                    paint.setColor(highlightColor);
                    int alpha = (int) (150 + 105 * animationProgress);
                    paint.setAlpha(alpha);
                }

                canvas.drawRect(x, y, x + squareSize, y + squareSize, paint);
                paint.setAlpha(255);

                // Draw piece
                int piece = activity.getPiece(row, col);
                if (piece > 0 && piece < PIECE_UNICODE.length) {
                    int colorCode = (piece == 0) ? 0 : (piece / 10);

                    // Add shadow for better visibility
                    paint.setColor(Color.BLACK);
                    paint.setAlpha(80);
                    paint.setTextSize(squareSize * 0.7f);
                    paint.setTextAlign(Paint.Align.CENTER);

                    String pieceChar = PIECE_UNICODE[piece];
                    float textX = x + squareSize / 2f;
                    float textY = y + squareSize / 2f + squareSize * 0.25f;

                    canvas.drawText(pieceChar, textX + 2, textY + 2, paint);

                    // Draw piece
                    paint.setAlpha(255);
                    if (colorCode == 1) {
                        paint.setColor(Color.WHITE);
                    } else {
                        paint.setColor(Color.BLACK);
                    }
                    canvas.drawText(pieceChar, textX, textY, paint);
                }
            }
        }

        // Draw highlighted moves with animation
        paint.setColor(moveIndicatorColor);
        int indicatorAlpha = (int) (120 + 60 * animationProgress);
        paint.setAlpha(indicatorAlpha);

        for (int i = 0; i < highlightedMoves.length; i += 2) {
            int row = highlightedMoves[i];
            int col = highlightedMoves[i + 1];
            int x = offsetX + col * squareSize;
            int y = offsetY + row * squareSize;

            int targetPiece = activity.getPiece(row, col);
            if (targetPiece != 0) {
                // Draw ring for capture
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(squareSize * 0.1f);
                canvas.drawCircle(x + squareSize / 2f, y + squareSize / 2f, squareSize * 0.4f, paint);
                paint.setStyle(Paint.Style.FILL);
            } else {
                // Draw dot for move
                float radius = squareSize / 5f + (squareSize / 20f) * animationProgress;
                canvas.drawCircle(x + squareSize / 2f, y + squareSize / 2f, radius, paint);
            }
        }

        // Draw border
        paint.setAlpha(255);
        paint.setColor(borderColor);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(10);
        canvas.drawRect(offsetX, offsetY, offsetX + boardSize, offsetY + boardSize, paint);
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawCoordinates(Canvas canvas, Paint paint, int offsetX, int offsetY, int squareSize, int boardSize) {
        paint.setTextSize(squareSize * 0.2f);
        paint.setColor(Color.parseColor("#888888"));
        paint.setTextAlign(Paint.Align.CENTER);

        // Files (a-h)
        for (int col = 0; col < 8; col++) {
            char file = (char) ('a' + col);
            float x = offsetX + col * squareSize + squareSize / 2f;
            float y = offsetY - squareSize * 0.1f;
            canvas.drawText(String.valueOf(file), x, y, paint);
            canvas.drawText(String.valueOf(file), x, offsetY + boardSize + squareSize * 0.3f, paint);
        }

        // Ranks (1-8)
        paint.setTextAlign(Paint.Align.LEFT);
        for (int row = 0; row < 8; row++) {
            int rank = 8 - row;
            float x = offsetX - squareSize * 0.3f;
            float y = offsetY + row * squareSize + squareSize * 0.6f;
            canvas.drawText(String.valueOf(rank), x, y, paint);
            canvas.drawText(String.valueOf(rank), offsetX + boardSize + squareSize * 0.1f, y, paint);
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (pulseAnimator != null) {
            pulseAnimator.cancel();
        }
    }
}
