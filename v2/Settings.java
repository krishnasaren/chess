package elrasseo.syreao.checkmate;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class Settings extends AppCompatActivity {
    private SharedPreferences prefs;
    private RadioGroup themeGroup;
    private Switch soundSwitch;
    private Switch vibrationSwitch;
    private Switch animationSwitch;
    private SeekBar volumeSeekBar;
    private TextView volumeText;
    private TextView versionText;
    private Button saveButton;
    private Button aboutButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_settings);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.settings), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        prefs = getSharedPreferences("ChessPrefs", MODE_PRIVATE);

        themeGroup = findViewById(R.id.themeGroup);
        soundSwitch = findViewById(R.id.soundSwitch);
        vibrationSwitch = findViewById(R.id.vibrationSwitch);
        animationSwitch = findViewById(R.id.animationSwitch);
        volumeSeekBar = findViewById(R.id.volumeSeekBar);
        volumeText = findViewById(R.id.volumeText);
        versionText = findViewById(R.id.versionText);
        saveButton = findViewById(R.id.saveButton);
        aboutButton = findViewById(R.id.aboutButton);

        loadSettings();
        setupListeners();

    }

    private void loadSettings() {
        // Load theme
        String theme = prefs.getString("theme", "classic");
        switch (theme) {
            case "dark":
                themeGroup.check(R.id.themeDark);
                break;
            case "blue":
                themeGroup.check(R.id.themeBlue);
                break;
            case "wood":
                themeGroup.check(R.id.themeWood);
                break;
            default:
                themeGroup.check(R.id.themeClassic);
                break;
        }

        // Load other settings
        soundSwitch.setChecked(prefs.getBoolean("sound", true));
        vibrationSwitch.setChecked(prefs.getBoolean("vibration", true));
        animationSwitch.setChecked(prefs.getBoolean("animation", true));

        int volume = prefs.getInt("volume", 50);
        volumeSeekBar.setProgress(volume);
        volumeText.setText("Volume: " + volume + "%");

        versionText.setText("Version 2.0.0");
    }

    private void setupListeners() {
        volumeSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                volumeText.setText("Volume: " + progress + "%");
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        saveButton.setOnClickListener(v -> saveSettings());

        aboutButton.setOnClickListener(v -> {
            android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
            builder.setTitle("About Chess Master");
            builder.setMessage("Chess Master v2.0.0\n\n" +
                    "A feature-rich chess game with:\n" +
                    "• Advanced AI with multiple difficulty levels\n" +
                    "• Beautiful themes and animations\n" +
                    "• Offline and online multiplayer\n" +
                    "• Complete chess rules implementation\n\n" +
                    "Developed by: Your Team\n" +
                    "© 2025 All rights reserved");
            builder.setPositiveButton("OK", null);
            builder.show();
        });
    }

    private void saveSettings() {
        SharedPreferences.Editor editor = prefs.edit();

        // Save theme
        int selectedThemeId = themeGroup.getCheckedRadioButtonId();
        String theme = "classic";
        if (selectedThemeId == R.id.themeDark) {
            theme = "dark";
        } else if (selectedThemeId == R.id.themeBlue) {
            theme = "blue";
        } else if (selectedThemeId == R.id.themeWood) {
            theme = "wood";
        }
        editor.putString("theme", theme);

        // Save other settings
        editor.putBoolean("sound", soundSwitch.isChecked());
        editor.putBoolean("vibration", vibrationSwitch.isChecked());
        editor.putBoolean("animation", animationSwitch.isChecked());
        editor.putInt("volume", volumeSeekBar.getProgress());

        editor.apply();

        setResult(RESULT_OK);
        finish();
    }
}