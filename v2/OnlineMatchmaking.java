package elrasseo.syreao.checkmate;

import android.os.Bundle;
import android.os.Handler;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import java.util.Random;

public class OnlineMatchmaking extends AppCompatActivity {
    private TextView statusText;
    private ProgressBar progressBar;
    private Button findMatchButton;
    private Button cancelButton;
    private Handler handler = new Handler();
    private boolean isSearching = false;
    private Random random = new Random();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_online_matchmaking);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.matchmaking), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        statusText = findViewById(R.id.statusText);
        progressBar = findViewById(R.id.progressBar);
        findMatchButton = findViewById(R.id.findMatchButton);
        cancelButton = findViewById(R.id.cancelButton);

        setupListeners();
    }
    private void setupListeners() {
        findMatchButton.setOnClickListener(v -> startMatchmaking());
        cancelButton.setOnClickListener(v -> cancelMatchmaking());
    }

    private void startMatchmaking() {
        isSearching = true;
        findMatchButton.setEnabled(false);
        cancelButton.setEnabled(true);
        progressBar.setVisibility(ProgressBar.VISIBLE);
        statusText.setText("Searching for opponent...");

        // Simulate matchmaking (replace with real server connection)
        handler.postDelayed(() -> {
            if (isSearching) {
                simulateMatchFound();
            }
        }, 3000 + random.nextInt(3000)); // 3-6 seconds
    }

    private void simulateMatchFound() {
        statusText.setText("Match found! Connecting...");

        handler.postDelayed(() -> {
            Toast.makeText(this, "Connected to opponent!", Toast.LENGTH_SHORT).show();
            setResult(RESULT_OK);
            finish();
        }, 1000);
    }

    private void cancelMatchmaking() {
        isSearching = false;
        findMatchButton.setEnabled(true);
        cancelButton.setEnabled(false);
        progressBar.setVisibility(ProgressBar.GONE);
        statusText.setText("Matchmaking cancelled");
        handler.removeCallbacksAndMessages(null);

        setResult(RESULT_CANCELED);
        finish();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacksAndMessages(null);
    }

    // TODO: Implement real online matchmaking with Firebase or custom server
    /*
     * For real implementation:
     * 1. Use Firebase Realtime Database or Firestore
     * 2. Create matchmaking queue
     * 3. Implement move synchronization
     * 4. Add chat functionality
     * 5. Implement ELO rating system
     * 6. Add friend system
     */
}