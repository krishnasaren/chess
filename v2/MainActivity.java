package elrasseo.syreao.checkmate;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.graphics.Color;
import android.os.Handler;
import android.widget.Toast;

import elrasseo.syreao.checkmate.databinding.ActivityMainBinding;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("checkmate");
    }

    public native void initGame();
    public native void setDifficulty(int difficulty);
    public native void cleanupGame();
    public native int getPiece(int row, int col);
    public native int[] getLegalMoves(int row, int col);
    public native boolean makeMove(int fromRow, int fromCol, int toRow, int toCol);
    public native int[] getComputerMove();
    public native int getCurrentPlayer();
    public native boolean isGameOver();

    private ChessBoardCustomView chessBoardCustomView;
    private TextView statusText;
    private TextView moveHistoryText;
    private Button newGameButton;
    private Button undoButton;
    private Button hintButton;
    private ImageButton settingsButton;
    private RadioGroup gameModeGroup;
    private RadioGroup difficultyGroup;
    private boolean isVsComputer = true;
    private boolean isOnlineMode = false;
    private int aiDifficulty = 2;
    private Handler handler = new Handler();
    private SharedPreferences prefs;

    private static final int REQUEST_SETTINGS = 1;
    private static final int REQUEST_ONLINE = 2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        prefs = getSharedPreferences("ChessPrefs", MODE_PRIVATE);

        chessBoardCustomView = binding.chessBoardView;
        statusText = binding.statusText;
        moveHistoryText = binding.moveHistoryText;
        newGameButton = binding.newGameButton;
        undoButton = binding.undoButton;
        hintButton = binding.hintButton;
        settingsButton = binding.settingsButton;
        gameModeGroup = binding.gameModeGroup;
        difficultyGroup = binding.difficultyGroup;

        chessBoardCustomView.setActivity(this);
        loadSettings();
        setupListeners();
        newGame();
    }

    private void loadSettings() {
        aiDifficulty = prefs.getInt("difficulty", 2);
        String theme = prefs.getString("theme", "classic");
        chessBoardCustomView.setTheme(theme);
    }

    private void setupListeners() {
        gameModeGroup.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.vsComputerBtn) {
                isVsComputer = true;
                isOnlineMode = false;
                difficultyGroup.setVisibility(View.VISIBLE);
            } else if (checkedId == R.id.vsPlayerBtn) {
                isVsComputer = false;
                isOnlineMode = false;
                difficultyGroup.setVisibility(View.GONE);
            } else if (checkedId == R.id.onlineBtn) {
                isOnlineMode = true;
                isVsComputer = false;
                difficultyGroup.setVisibility(View.GONE);
                startOnlineMode();
            }
            newGame();
        });

        difficultyGroup.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.easyBtn) {
                aiDifficulty = 1;
            } else if (checkedId == R.id.mediumBtn) {
                aiDifficulty = 2;
            } else if (checkedId == R.id.hardBtn) {
                aiDifficulty = 3;
            } else if (checkedId == R.id.expertBtn) {
                aiDifficulty = 4;
            }
            setDifficulty(aiDifficulty);
            prefs.edit().putInt("difficulty", aiDifficulty).apply();
        });

        newGameButton.setOnClickListener(v -> newGame());

        undoButton.setOnClickListener(v -> {
            Toast.makeText(this, "Undo feature coming soon!", Toast.LENGTH_SHORT).show();
        });

        hintButton.setOnClickListener(v -> {
            if (!isGameOver() && !isVsComputer) {
                Toast.makeText(this, "Hint: Look for tactical opportunities!", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "Hints not available in this mode", Toast.LENGTH_SHORT).show();
            }
        });

        settingsButton.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, Settings.class);
            startActivityForResult(intent, REQUEST_SETTINGS);
        });
    }

    private void startOnlineMode() {
        Intent intent = new Intent(MainActivity.this, OnlineMatchmaking.class);
        startActivityForResult(intent, REQUEST_ONLINE);
    }

    private void newGame() {
        initGame();
        setDifficulty(aiDifficulty);
        chessBoardCustomView.reset();
        moveHistoryText.setText("Move History:\n");
        updateStatus();
    }

    public void updateStatus() {
        int currentPlayer = getCurrentPlayer();
        String playerName = (currentPlayer == 1) ? "White" : "Black";

        if (isGameOver()) {
            String winner = ((currentPlayer == 1) ? "Black" : "White");
            statusText.setText("🏆 Game Over! " + winner + " Wins!");
            statusText.setTextColor(Color.parseColor("#FFD700"));
        } else {
            if (isOnlineMode) {
                statusText.setText("🌐 " + playerName + "'s Turn (Online)");
            } else {
                statusText.setText("♟️ " + playerName + "'s Turn");
            }
            statusText.setTextColor(Color.parseColor("#00d4ff"));

            if (isVsComputer && currentPlayer == 2 && !isGameOver()) {
                statusText.setText("🤖 Computer is thinking...");
                handler.postDelayed(() -> makeComputerMove(), 500);
            }
        }
    }

    private void makeComputerMove() {
        int[] move = getComputerMove();
        if (move.length == 4 && move[0] >= 0) {
            makeMove(move[0], move[1], move[2], move[3]);
            updateMoveHistory(move[0], move[1], move[2], move[3]);
            chessBoardCustomView.invalidate();
            updateStatus();
        }
    }

    private void updateMoveHistory(int fromRow, int fromCol, int toRow, int toCol) {
        String moveNotation = convertToChessNotation(fromRow, fromCol, toRow, toCol);
        String currentText = moveHistoryText.getText().toString();
        moveHistoryText.setText(currentText + moveNotation + "\n");
    }

    private String convertToChessNotation(int fromRow, int fromCol, int toRow, int toCol) {
        char fromFile = (char) ('a' + fromCol);
        char toFile = (char) ('a' + toCol);
        int fromRank = 8 - fromRow;
        int toRank = 8 - toRow;
        return "" + fromFile + fromRank + toFile + toRank;
    }

    public boolean isVsComputer() {
        return isVsComputer;
    }

    public boolean isOnlineMode() {
        return isOnlineMode;
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        cleanupGame();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_SETTINGS && resultCode == RESULT_OK) {
            loadSettings();
            chessBoardCustomView.invalidate();
        } else if (requestCode == REQUEST_ONLINE) {
            if (resultCode == RESULT_OK) {
                isOnlineMode = true;
                Toast.makeText(this, "Connected to online match!", Toast.LENGTH_SHORT).show();
            } else {
                isOnlineMode = false;
                findViewById(R.id.vsPlayerBtn).performClick();
            }
        }
    }
}



