package elrasseo.syreao.checkmate;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.graphics.Color;
import android.os.Handler;

import elrasseo.syreao.checkmate.databinding.ActivityMainBinding;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("checkmate");
    }

    // Native methods
    // Native methods
    public native void initGame();
    public native void setDifficulty(int difficulty);
    public native int getPiece(int row, int col);
    public native int[] getLegalMoves(int row, int col);
    public native boolean makeMove(int fromRow, int fromCol, int toRow, int toCol);
    public native int[] getComputerMove();
    public native int getCurrentPlayer();
    public native boolean isGameOver();

    private ChessBoardCustomView chessBoardCustomView;
    private TextView statusText;
    private Button newGameButton;
    private RadioGroup gameModeGroup;
    private RadioButton vsComputerBtn;
    private RadioButton vsPlayerBtn;
    private RadioGroup difficultyGroup;
    private boolean isVsComputer = true;
    private int aiDifficulty = 2; // 1=Easy, 2=Medium, 3=Hard
    private Handler handler = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Initialize views
        chessBoardCustomView = binding.chessBoardView;
        statusText = binding.statusText;
        newGameButton = binding.newGameButton;
        gameModeGroup = binding.gameModeGroup;
        vsComputerBtn = binding.vsComputerBtn;
        vsPlayerBtn = binding.vsPlayerBtn;
        difficultyGroup = binding.difficultyGroup;

        // Set activity reference for ChessBoardView
        chessBoardCustomView.setActivity(this);

        // Setup listeners
        gameModeGroup.setOnCheckedChangeListener((group, checkedId) -> {
            if (checkedId == R.id.vsComputerBtn) {
                isVsComputer = true;
                difficultyGroup.setVisibility(View.VISIBLE);
            } else {
                isVsComputer = false;
                difficultyGroup.setVisibility(View.GONE);
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
            }
            setDifficulty(aiDifficulty);
        });

        newGameButton.setOnClickListener(v -> newGame());

        // Initialize game
        newGame();
    }

    private void newGame() {
        initGame();
        setDifficulty(aiDifficulty);
        chessBoardCustomView.reset();
        updateStatus();
    }

    public void updateStatus() {
        int currentPlayer = getCurrentPlayer();
        String playerName = (currentPlayer == 1) ? "White" : "Black";

        if (isGameOver()) {
            statusText.setText("Game Over! " + ((currentPlayer == 1) ? "Black" : "White") + " Wins!");
            statusText.setTextColor(Color.parseColor("#ff4757"));
        } else {
            statusText.setText(playerName + "'s Turn");
            statusText.setTextColor(Color.parseColor("#00d4ff"));

            if (isVsComputer && currentPlayer == 2) {
                statusText.setText("Computer is thinking...");
                handler.postDelayed(() -> makeComputerMove(), 300);
            }
        }
    }

    private void makeComputerMove() {
        int[] move = getComputerMove();
        if (move.length == 4 && move[0] >= 0) {
            makeMove(move[0], move[1], move[2], move[3]);
            chessBoardCustomView.invalidate();
            updateStatus();
        }
    }

    public boolean isVsComputer() {
        return isVsComputer;
    }
}



