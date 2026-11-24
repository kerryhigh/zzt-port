package com.kerry.zzt;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.text.InputType;
import android.text.InputFilter;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.TextPaint;
import android.text.method.LinkMovementMethod;
import android.text.style.ClickableSpan;
import android.text.style.ForegroundColorSpan;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.content.Context;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.NumberPicker;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ArrayAdapter;
import android.view.Choreographer;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CodingErrorAction;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public final class MainActivity extends Activity implements Choreographer.FrameCallback {
    private static final Charset CP437 = Charset.forName("IBM437");
    private static final int OPEN_WORLD = 1;
    private static final int EXPORT_SAVE = 2;
    private static final int EXPORT_WORLD = 3;
    private static final int EXPORT_HIGHSCORES = 4;
    private static final int OPEN_BOARD = 5;
    private static final int EXPORT_BOARD = 6;
    private static final String PREF_MONOCHROME_PLAYFIELD = "monochrome_playfield";
    private static final int MAX_BRD_RECORD_LENGTH = 0xFFFF;
    private static final int KEY_UP = 200;
    private static final int KEY_LEFT = 203;
    private static final int KEY_RIGHT = 205;
    private static final int KEY_DOWN = 208;
    private static final int KEY_ENTER = 13;
    private static final int KEY_ESCAPE = 27;
    private static final int ACTION_SHOOT = 6;
    private static final int GAME_STATE_TITLE = 1;
    private static final int GAME_STATE_PLAY = 2;
    private static final int CATEGORY_ITEM = 1;
    private static final int CATEGORY_CREATURE = 2;
    private static final int CATEGORY_TERRAIN = 3;
    private static final int ELEMENT_PLAYER = 4;
    private static final int[] EDITOR_PATTERN_ELEMENTS = {21, 22, 23, 0, 31};
    private static final String[] EDITOR_PATTERN_NAMES = {
            "Solid", "Normal", "Breakable", "Empty", "Line", "Copied tile"
    };

    private static final class HelpLink {
        final int start;
        final int end;
        final String target;

        HelpLink(int start, int end, String target) {
            this.start = start;
            this.end = end;
            this.target = target;
        }
    }

    private static final class HelpDocument {
        final SpannableStringBuilder text = new SpannableStringBuilder();
        final ArrayList<HelpLink> links = new ArrayList<>();
        final Map<String, Integer> anchors = new HashMap<>();
    }

    private BoardView boardView;
    private TextView boardTitle;
    private TextView statusText;
    private TextView worldTitle;
    private LinearLayout controls;
    private LinearLayout tools;
    private LinearLayout editorTools;
    private Button playTool;
    private Button saveTool;
    private byte[] frameBytes = new byte[BoardView.COLS * BoardView.ROWS * 2];
    private final byte[] transitionFrom = new byte[BoardView.COLS * BoardView.ROWS * 2];
    private final byte[] transitionTo = new byte[BoardView.COLS * BoardView.ROWS * 2];
    private final short[] transitionOrder = new short[BoardView.COLS * BoardView.ROWS];
    private int[] lastStatus;
    private File activeDirectory;
    private String activeFilename = "DEMO.ZZT";
    private File pendingExport;
    private byte[] pendingBoardExport;
    private boolean framePosted;
    private boolean pickerSaveOnly;
    private boolean editorActive;
    private boolean editorModified;
    private boolean editorCopyMode;
    private boolean editorPasteMode;
    private boolean editorClipboardValid;
    private boolean editorFloodFillMode;
    private boolean editorFloodFillUsesClipboard;
    private boolean editorTextMode;
    private boolean editorDrawMode;
    private boolean editorShiftHeld;
    private boolean editorShiftPainted;
    private boolean editorClipboardPattern;
    private final boolean[] handledGameplayKeyUps = new boolean[512];
    private final HighScores highScores = new HighScores();
    private int editorElement = 21;
    private int editorColor = 14;
    private int editorPatternIndex;
    private int editorTextCharacter = 'A';
    private int editorCellX = -1;
    private int editorCellY = -1;
    private String editorElementName = "Solid";
    private volatile boolean audioRunning;
    private AudioTrack audioTrack;
    private Thread audioThread;
    private final int[] gameplayNumberKeyUps = new int[512];
    private final Set<Integer> heldNativeKeys = new java.util.HashSet<>();
    private final Set<Integer> heldNativeActions = new java.util.HashSet<>();
    private final ArrayList<Runnable> activeTouchReleaseCallbacks = new ArrayList<>();
    private int modalPromptBlockDepth;
    private final boolean[] externalActivityBlocks = new boolean[EXPORT_BOARD + 1];

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setStatusBarColor(Color.rgb(18, 18, 18));
        getWindow().setNavigationBarColor(Color.rgb(18, 18, 18));
        getWindow().getDecorView().setSystemUiVisibility(0);

        File worlds = new File(getFilesDir(), "worlds");
        activeDirectory = new File(worlds, "demo");
        activeDirectory.mkdirs();
        try {
            File demo = new File(activeDirectory, "DEMO.ZZT");
            if (!demo.exists()) copyAsset("DEMO.ZZT", demo);
        } catch (Exception e) {
            Toast.makeText(this, "Could not install DEMO.ZZT: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }

        int result = ZztNative.init(activeDirectory.getAbsolutePath(), "DEMO.ZZT");
        if (result != 0) toastError("Load DEMO.ZZT", result);
        highScores.loadInto(highScoreFile());
        buildScreen();
        updateDisplay(System.nanoTime() / 1_000_000L);
    }

    private void buildScreen() {
        int pad = dp(8);
        LinearLayout page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setMotionEventSplittingEnabled(true);
        page.setPadding(pad, dp(4), pad, dp(6));
        page.setBackgroundColor(Color.rgb(18, 18, 18));

        LinearLayout top = new LinearLayout(this);
        top.setGravity(Gravity.CENTER_VERTICAL);
        worldTitle = label("ZZT", 21, Color.rgb(255, 224, 102));
        worldTitle.setTypeface(android.graphics.Typeface.MONOSPACE, android.graphics.Typeface.BOLD);
        top.addView(worldTitle, new LinearLayout.LayoutParams(0, dp(42), 1));
        Button menu = button("Menu");
        menu.setOnClickListener(v -> showMenu());
        top.addView(menu, new LinearLayout.LayoutParams(dp(86), dp(42)));
        page.addView(top);

        boardView = new BoardView(this, ZztNative.font8x14());
        boolean monochrome = getPreferences(MODE_PRIVATE)
                .getBoolean(PREF_MONOCHROME_PLAYFIELD, false);
        boardView.setMonochrome(monochrome);
        ZztNative.setMonochrome(monochrome);
        LinearLayout.LayoutParams boardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        boardParams.setMargins(0, dp(2), 0, dp(2));
        page.addView(boardView, boardParams);

        boardTitle = label("Title screen", 14, Color.rgb(85, 255, 255));
        boardTitle.setTypeface(android.graphics.Typeface.MONOSPACE, android.graphics.Typeface.BOLD);
        boardTitle.setPadding(dp(3), dp(2), dp(3), dp(2));
        page.addView(boardTitle);

        statusText = label("", 14, Color.WHITE);
        statusText.setTypeface(android.graphics.Typeface.MONOSPACE);
        statusText.setPadding(dp(3), dp(2), dp(3), dp(4));
        page.addView(statusText);

        controls = new LinearLayout(this);
        controls.setGravity(Gravity.CENTER);
        controls.setMotionEventSplittingEnabled(true);
        GridLayout dpad = new GridLayout(this);
        dpad.setColumnCount(3);
        dpad.setRowCount(3);
        dpad.setMotionEventSplittingEnabled(true);
        addDirection(dpad, "▲", 0, 1, KEY_UP);
        addDirection(dpad, "◀", 1, 0, KEY_LEFT);
        Button confirm = button("OK");
        confirm.setContentDescription("Confirm");
        bindKey(confirm, KEY_ENTER);
        addGridButton(dpad, confirm, 1, 1);
        addDirection(dpad, "▶", 1, 2, KEY_RIGHT);
        addDirection(dpad, "▼", 2, 1, KEY_DOWN);
        controls.addView(dpad, new LinearLayout.LayoutParams(dp(174), dp(174)));

        LinearLayout actionButtons = new LinearLayout(this);
        actionButtons.setOrientation(LinearLayout.VERTICAL);
        actionButtons.setGravity(Gravity.CENTER);
        actionButtons.setMotionEventSplittingEnabled(true);
        Button fire = button("Fire");
        fire.setContentDescription("Tap to fire; hold with a direction to keep firing");
        bindHold(fire,
                () -> {
                    ZztNative.shootTap();
                    setNativeAction(ACTION_SHOOT, true);
                },
                () -> setNativeAction(ACTION_SHOOT, false));
        actionButtons.addView(fire, new LinearLayout.LayoutParams(dp(98), dp(62)));
        Button torch = button("Torch");
        bindKey(torch, 't');
        LinearLayout.LayoutParams torchParams = new LinearLayout.LayoutParams(dp(98), dp(54));
        torchParams.topMargin = dp(8);
        actionButtons.addView(torch, torchParams);
        controls.addView(actionButtons);
        page.addView(controls);

        tools = new LinearLayout(this);
        tools.setGravity(Gravity.CENTER);
        addTool(tools, "Menu", v -> showMenu());
        playTool = addTool(tools, "Play", v -> {
            int code = startGameFromTitle();
            if (code != 0) toastError("Start game", code);
            refreshStatus();
        });
        saveTool = addTool(tools, "Save", v -> saveAndExport());
        saveTool.setVisibility(View.GONE);
        addTool(tools, "Esc", v -> handleEscapeAction());
        LinearLayout.LayoutParams toolsParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(48));
        toolsParams.topMargin = dp(3);
        page.addView(tools, toolsParams);

        editorTools = new LinearLayout(this);
        editorTools.setOrientation(LinearLayout.VERTICAL);
        LinearLayout editorToolsTop = new LinearLayout(this);
        editorToolsTop.setGravity(Gravity.CENTER);
        addTool(editorToolsTop, "Items", v -> showEditorPalette(CATEGORY_ITEM, "Items"));
        addTool(editorToolsTop, "Creatures", v -> showEditorPalette(CATEGORY_CREATURE, "Creatures"));
        addTool(editorToolsTop, "Terrain", v -> showEditorPalette(CATEGORY_TERRAIN, "Terrain"));
        addTool(editorToolsTop, "Patterns", v -> showEditorPatterns());
        addTool(editorToolsTop, "Boards", v -> showEditorBoards());
        editorTools.addView(editorToolsTop, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(48)));
        LinearLayout editorToolsBottom = new LinearLayout(this);
        editorToolsBottom.setGravity(Gravity.CENTER);
        addTool(editorToolsBottom, "More", v -> showEditorMore());
        addTool(editorToolsBottom, "Save", v -> saveWorldToApp());
        addTool(editorToolsBottom, "Export", v -> saveAndExportWorld());
        addTool(editorToolsBottom, "Done", v -> requestEditorExit());
        editorTools.addView(editorToolsBottom, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(48)));
        editorTools.setVisibility(View.GONE);
        page.addView(editorTools);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setMotionEventSplittingEnabled(true);
        scroll.setBackgroundColor(Color.rgb(18, 18, 18));
        scroll.setOnApplyWindowInsetsListener((view, insets) -> {
            int topInset;
            int bottom;
            if (Build.VERSION.SDK_INT >= 30) {
                android.graphics.Insets bars = insets.getInsets(WindowInsets.Type.systemBars());
                topInset = bars.top;
                bottom = bars.bottom;
            } else {
                topInset = insets.getSystemWindowInsetTop();
                bottom = insets.getSystemWindowInsetBottom();
            }
            view.setPadding(0, topInset, 0, bottom);
            return insets;
        });
        scroll.addView(page);
        setContentView(scroll);
    }

    private void addDirection(GridLayout grid, String glyph, int row, int column, int key) {
        Button direction = button(glyph);
        direction.setContentDescription("Move " + glyph);
        bindKey(direction, key);
        addGridButton(grid, direction, row, column);
    }

    private void addGridButton(GridLayout grid, Button button, int row, int column) {
        GridLayout.LayoutParams params = new GridLayout.LayoutParams(
                GridLayout.spec(row), GridLayout.spec(column));
        params.width = dp(54);
        params.height = dp(54);
        params.setMargins(dp(2), dp(2), dp(2), dp(2));
        grid.addView(button, params);
    }

    private Button addTool(LinearLayout row, String text, View.OnClickListener click) {
        Button b = button(text);
        b.setOnClickListener(click);
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(0, dp(46), 1);
        p.setMargins(dp(2), 0, dp(2), 0);
        row.addView(b, p);
        return b;
    }

    private void bindKey(Button button, int key) {
        bindHold(button,
                () -> setNativeKey(key, true),
                () -> setNativeKey(key, false));
    }

    private void setNativeKey(int key, boolean down) {
        if (down) heldNativeKeys.add(key);
        else heldNativeKeys.remove(key);
        ZztNative.key(key, down);
    }

    private void setNativeAction(int action, boolean down) {
        if (down) heldNativeActions.add(action);
        else heldNativeActions.remove(action);
        ZztNative.action(action, down);
    }

    private void releaseHeldInput() {
        for (Runnable release : activeTouchReleaseCallbacks) release.run();
        for (int key : new ArrayList<>(heldNativeKeys)) setNativeKey(key, false);
        for (int action : new ArrayList<>(heldNativeActions)) setNativeAction(action, false);
        Arrays.fill(handledGameplayKeyUps, false);
        Arrays.fill(gameplayNumberKeyUps, 0);
        if (editorShiftHeld) {
            editorShiftHeld = false;
            if (!editorShiftPainted) plotEditorCursor();
            editorShiftPainted = false;
        }
    }

    private void bindHold(Button button, Runnable onPress, Runnable onRelease) {
        final int[] activePointerId = {-1};
        Runnable releaseHold = () -> {
            if (activePointerId[0] == -1) return;
            activePointerId[0] = -1;
            onRelease.run();
            button.setPressed(false);
        };
        activeTouchReleaseCallbacks.add(releaseHold);
        button.setOnTouchListener((view, event) -> {
            int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                int actionIndex = event.getActionIndex();
                if (action == MotionEvent.ACTION_POINTER_DOWN
                        && (event.getX(actionIndex) < 0 || event.getY(actionIndex) < 0
                        || event.getX(actionIndex) >= view.getWidth()
                        || event.getY(actionIndex) >= view.getHeight())) {
                    return true;
                }
                if (activePointerId[0] == -1) {
                    activePointerId[0] = event.getPointerId(actionIndex);
                    onPress.run();
                    view.setPressed(true);
                }
                return true;
            }

            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
                int actionPointerId = event.getPointerId(event.getActionIndex());
                if (activePointerId[0] == actionPointerId) releaseHold.run();
                return true;
            }

            if (action == MotionEvent.ACTION_CANCEL) {
                releaseHold.run();
                return true;
            }

            return activePointerId[0] != -1;
        });
    }

    private Button button(String text) {
        Button b = new Button(this);
        b.setText(text);
        b.setTextSize(15);
        b.setTextColor(Color.WHITE);
        b.setAllCaps(false);
        b.setPadding(dp(4), 0, dp(4), 0);
        return b;
    }

    private TextView label(String text, int sp, int color) {
        TextView v = new TextView(this);
        v.setText(text);
        v.setTextSize(sp);
        v.setTextColor(color);
        return v;
    }

    private void acquireModalPromptBlock() {
        if (modalPromptBlockDepth++ == 0) ZztNative.setModalPromptBlocked(true);
    }

    private void releaseModalPromptBlock() {
        if (modalPromptBlockDepth == 0) return;
        if (--modalPromptBlockDepth == 0) ZztNative.setModalPromptBlocked(false);
    }

    private void showBlockedDialog(AlertDialog dialog) {
        acquireModalPromptBlock();
        dialog.setOnDismissListener(d -> releaseModalPromptBlock());
        dialog.show();
    }

    private void startExternalActivity(Intent intent, int requestCode) throws Exception {
        boolean blockGame = requestCode > 0
                && requestCode < externalActivityBlocks.length;
        if (blockGame) {
            externalActivityBlocks[requestCode] = true;
            acquireModalPromptBlock();
        }
        try {
            startActivityForResult(intent, requestCode);
        } catch (Exception e) {
            if (blockGame) {
                externalActivityBlocks[requestCode] = false;
                releaseModalPromptBlock();
            }
            throw e;
        }
    }

    private void releaseExternalActivityBlock(int requestCode) {
        if (requestCode <= 0 || requestCode >= externalActivityBlocks.length
                || !externalActivityBlocks[requestCode]) return;
        externalActivityBlocks[requestCode] = false;
        releaseModalPromptBlock();
    }

    private void showMenu() {
        if (editorActive) {
            String[] editorEntries = {
                    "Load world", "New world", "Save world", "Export world", "Done editing",
                    displayModeEntry()
            };
            AlertDialog editorDialog = new AlertDialog.Builder(this)
                    .setTitle("ZZT Editor")
                    .setItems(editorEntries, (dialog, which) -> {
                        if (which == 0) requestEditorWorldLoad();
                        else if (which == 1) confirmEditorNewWorld();
                        else if (which == 2) saveWorldToApp();
                        else if (which == 3) saveAndExportWorld();
                        else if (which == 4) requestEditorExit();
                        else if (which == 5) showDisplayModeDialog();
                    })
                    .create();
            showBlockedDialog(editorDialog);
            return;
        }
        int[] status = ZztNative.status();
        boolean atTitle = status != null && status.length > 0 && status[0] == GAME_STATE_TITLE;
        String[] entries = atTitle
                ? new String[] {"Play", "Load world", "Restore save", "Game speed", "Edit board", "About ZZT", "High Scores", "Help", "ZZT debug command", "Quit ZZT", displayModeEntry()}
                : new String[] {"Resume", "Pause / resume", "Save and export", "Type text", "Help", "Order form", "ZZT debug command", "Return to title", displayModeEntry()};
        AlertDialog menuDialog = new AlertDialog.Builder(this)
                .setTitle("ZZT")
                .setItems(entries, (dialog, which) -> {
                    if (atTitle) {
                        switch (which) {
                            case 0: {
                                int result = startGameFromTitle();
                                if (result != 0) toastError("Start game", result);
                                break;
                            }
                            case 1: pickWorld(false); break;
                            case 2: pickWorld(true); break;
                            case 3: showSpeedDialog(); break;
                            case 4: beginEditor(); break;
                            case 5: showAbout(); break;
                            case 6: showHighScores(); break;
                            case 7: showHelp(); break;
                            case 8: showDebugPrompt(); break;
                            case 9: confirmQuit(); break;
                            case 10: showDisplayModeDialog(); break;
                            default: break;
                        }
                    } else {
                        switch (which) {
                            case 0: {
                                // Dismissing the menu resumes the existing loop. Calling
                                // zoo_world_play here would reload the source .ZZT and
                                // discard a restored or already-running game.
                                break;
                            }
                            case 1: ZztNative.togglePause(); break;
                            case 2: saveAndExport(); break;
                            case 3: typeText(); break;
                            case 4: showHelp(); break;
                            case 5: showOrderForm(); break;
                            case 6: showDebugPrompt(); break;
                            case 7: requestEndPlay(); break;
                            case 8: showDisplayModeDialog(); break;
                            default: break;
                        }
                    }
                    refreshStatus();
                })
                .create();
        showBlockedDialog(menuDialog);
    }

    private String displayModeEntry() {
        return "Playfield display: "
                + (boardView != null && boardView.isMonochrome() ? "Monochrome" : "Color");
    }

    private void showDisplayModeDialog() {
        boolean monochrome = boardView != null && boardView.isMonochrome();
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Playfield display")
                .setSingleChoiceItems(new String[] {"Color", "Monochrome"}, monochrome ? 1 : 0,
                        (selection, which) -> {
                            boolean selectedMonochrome = which == 1;
                            if (boardView != null) boardView.setMonochrome(selectedMonochrome);
                            ZztNative.setMonochrome(selectedMonochrome);
                            getPreferences(MODE_PRIVATE).edit()
                                    .putBoolean(PREF_MONOCHROME_PLAYFIELD, selectedMonochrome)
                                    .apply();
                            selection.dismiss();
                        })
                .setNegativeButton("Cancel", null)
                .create();
        showBlockedDialog(dialog);
    }

    private void showHelp() {
        showHelpDialog("Playing ZZT",
                "Android controls: hold a direction to move. Tap Fire or press Space to fire once toward the last direction you faced. Hold Fire or Shift with a direction to fire as you move. Torch or T uses a torch. P pauses, B toggles sound, S saves, and Q asks to end the game.\n\n",
                "GAME.HLP");
    }

    private void showOrderForm() {
        showHelpDialog("Order form", "", "ORDER.HLP");
    }

    private void showEditorHelp() {
        showHelpDialog("World editor help",
                "Android editor: use Items, Creatures, Terrain, Patterns, and Boards to choose what to place. Tap a board cell to edit it; More contains copy, paste, color, stat, fill, text, and help-file tools. L loads a world, N creates a new world, and ! edits a named .HLP file in this world's app folder.\n\n",
                "EDITOR.HLP");
    }

    private void editHelpFilePrompt() {
        EditText nameInput = new EditText(this);
        nameInput.setSingleLine(true);
        nameInput.setFilters(new InputFilter[] {new InputFilter.LengthFilter(8)});
        nameInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS
                | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        nameInput.setHint("Name (up to 8 letters, digits, or hyphens)");
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Edit help file")
                .setMessage("Edit or create a separate .HLP file beside the active world. Bundled help remains read-only.")
                .setView(nameInput)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Edit", null)
                .create();
        dialog.setOnShowListener(ignored -> {
            nameInput.requestFocus();
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
                String name = nameInput.getText().toString().toUpperCase(Locale.ROOT);
                if (name.isEmpty()) {
                    dialog.dismiss();
                    return;
                }
                if (!name.matches("[A-Z0-9-]{1,8}")) {
                    Toast.makeText(this, "Use up to 8 letters, digits, or hyphens.", Toast.LENGTH_LONG).show();
                    return;
                }
                dialog.dismiss();
                openEditorHelpFile(name);
            });
            nameInput.postDelayed(() -> {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) imm.showSoftInput(nameInput, InputMethodManager.SHOW_IMPLICIT);
            }, 120);
        });
        dialog.show();
    }

    private void openEditorHelpFile(String baseName) {
        if (!editorActive || activeDirectory == null) return;
        File file = findDosFile(activeDirectory, baseName + ".HLP");
        String originalText;
        try {
            originalText = readEditorHelpFile(file);
        } catch (IOException e) {
            Toast.makeText(this, "Could not open help file: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        }

        EditText editor = new EditText(this);
        editor.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_MULTI_LINE
                | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        editor.setGravity(Gravity.TOP | Gravity.START);
        editor.setTypeface(android.graphics.Typeface.MONOSPACE);
        editor.setMinLines(12);
        editor.setFilters(new InputFilter[] {new InputFilter.LengthFilter(60000)});
        installPascalTextWindowControls(editor);
        editor.setText(originalText);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.addView(editor, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Editing " + baseName)
                .setView(scroll)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Save", null)
                .create();
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    byte[] content = encodeEditorHelpFile(editor.getText().toString(), originalText);
                    if (content == null) return;
                    try (FileOutputStream output = new FileOutputStream(file)) {
                        output.write(content);
                        output.flush();
                        dialog.dismiss();
                        Toast.makeText(this, "Saved " + file.getName() + " in this world's folder.",
                                Toast.LENGTH_LONG).show();
                    } catch (IOException e) {
                        Toast.makeText(this, "Could not save help file: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    }
                }));
        dialog.show();
    }

    private String readEditorHelpFile(File file) throws IOException {
        if (!file.isFile()) return "";
        if (file.length() > 65536) throw new IOException("The file exceeds the Pascal editor's text capacity.");
        ByteArrayOutputStream bytes = new ByteArrayOutputStream((int) file.length());
        try (InputStream input = new FileInputStream(file)) {
            byte[] chunk = new byte[4096];
            int count;
            while ((count = input.read(chunk)) != -1) {
                if (bytes.size() + count > 65536) throw new IOException("The file is too large.");
                bytes.write(chunk, 0, count);
            }
        }
        byte[] raw = bytes.toByteArray();
        int dosEof = -1;
        for (int i = 0; i < raw.length; i++) {
            if ((raw[i] & 0xff) == 0x1a) {
                dosEof = i;
                break;
            }
        }
        String text = new String(raw, 0, dosEof < 0 ? raw.length : dosEof, CP437)
                .replace("\r\n", "\n").replace('\r', '\n');
        String[] lines = editorHelpLines(text);
        if (lines.length > 1024) throw new IOException("The file exceeds the Pascal editor's 1,024-line limit.");
        for (int i = 0; i < lines.length; i++) {
            if (lines[i].length() > 50) lines[i] = lines[i].substring(0, 50);
        }
        return String.join("\n", lines);
    }

    private byte[] encodeEditorHelpFile(String text, String originalText) {
        String[] lines = editorHelpLines(text);
        String[] originalLines = editorHelpLines(originalText);
        if (lines.length > 1024) {
            Toast.makeText(this, "ZZT help files support at most 1,024 lines.", Toast.LENGTH_LONG).show();
            return null;
        }
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        for (int i = 0; i < lines.length; i++) {
            byte[] encoded = encodeCp437(lines[i]);
            if (encoded == null) {
                Toast.makeText(this, "Help text can only contain characters in IBM PC code page 437.",
                        Toast.LENGTH_LONG).show();
                return null;
            }
            boolean pascalEditable = isPascalEditableLine(encoded, originalLines);
            if (encoded.length > 50 || (encoded.length > 42 && !pascalEditable)) {
                Toast.makeText(this, "Lines over 42 CP437 bytes must come from existing text.", Toast.LENGTH_LONG).show();
                return null;
            }
            for (byte value : encoded) {
                if ((value & 0xff) < 32 && !pascalEditable) {
                    Toast.makeText(this, "Edited lines can only contain printable CP437 characters.",
                            Toast.LENGTH_LONG).show();
                    return null;
                }
            }
            bytes.write(encoded, 0, encoded.length);
            bytes.write('\r');
            bytes.write('\n');
        }
        return bytes.toByteArray();
    }

    private static boolean isPascalEditableLine(byte[] edited, String[] originalLines) {
        if (edited.length <= 42) {
            boolean hasControlByte = false;
            for (byte value : edited) hasControlByte |= (value & 0xff) < 32;
            if (!hasControlByte) return true;

            for (String original : originalLines) {
                byte[] source = encodeCp437(original);
                if (source == null) continue;
                int sourcePosition = 0;
                boolean matches = true;
                for (byte value : edited) {
                    if ((value & 0xff) >= 32) continue;
                    while (sourcePosition < source.length && source[sourcePosition] != value) sourcePosition++;
                    if (sourcePosition == source.length) {
                        matches = false;
                        break;
                    }
                    sourcePosition++;
                }
                if (matches) return true;
            }
            return false;
        }

        if (edited.length > 50) return false;
        for (String original : originalLines) {
            byte[] source = encodeCp437(original);
            if (source == null || source.length < edited.length) continue;

            boolean[] reachable = new boolean[edited.length + 1];
            reachable[0] = true;
            for (byte sourceByte : source) {
                boolean[] next = Arrays.copyOf(reachable, reachable.length); // Deleting a source byte is allowed.
                for (int position = 0; position < edited.length; position++) {
                    int value = edited[position] & 0xff;
                    if (reachable[position] && (sourceByte == edited[position]
                            || (position < 42 && value >= 32))) {
                        next[position + 1] = true;
                    }
                }
                reachable = next;
            }
            if (reachable[edited.length]) return true;
        }
        return false;
    }

    private static String[] editorHelpLines(String text) {
        String normalized = text.replace("\r\n", "\n").replace('\r', '\n');
        if (normalized.isEmpty()) return new String[0];
        String[] lines = normalized.split("\n", -1);
        if (lines.length > 0 && lines[lines.length - 1].isEmpty()) {
            lines = Arrays.copyOf(lines, lines.length - 1);
        }
        return lines;
    }

    private static void installPascalTextWindowControls(EditText editor) {
        boolean[] insertMode = {true};
        editor.setContentDescription("ZZT text editor, insert mode on. Press Insert to toggle.");
        editor.setOnKeyListener((view, keyCode, event) -> {
            if (event.getAction() != KeyEvent.ACTION_DOWN) {
                return false;
            }
            if (keyCode == KeyEvent.KEYCODE_INSERT) {
                if (event.getRepeatCount() == 0) {
                    insertMode[0] = !insertMode[0];
                    editor.setContentDescription(insertMode[0]
                            ? "ZZT text editor, insert mode on. Press Insert to toggle."
                            : "ZZT text editor, overwrite mode on. Press Insert to toggle.");
                }
                return true;
            }
            if (keyCode == KeyEvent.KEYCODE_Y && event.isCtrlPressed()) {
                android.text.Editable text = editor.getText();
                String value = text.toString();
                int cursor = Math.max(0, Math.min(editor.getSelectionStart(), value.length()));
                int start = value.lastIndexOf('\n', cursor - 1) + 1;
                int nextLine = value.indexOf('\n', cursor);
                int column = cursor - start;
                int newCursor;
                if (nextLine >= 0) {
                    text.delete(start, nextLine + 1);
                    String remaining = text.toString();
                    int replacementEnd = remaining.indexOf('\n', start);
                    if (replacementEnd < 0) replacementEnd = remaining.length();
                    newCursor = start + Math.min(column, replacementEnd - start);
                } else if (start > 0) {
                    int previousEnd = start - 1;
                    int previousStart = value.lastIndexOf('\n', previousEnd - 1) + 1;
                    newCursor = previousStart + Math.min(column, previousEnd - previousStart);
                    text.delete(previousEnd, value.length());
                } else {
                    text.clear();
                    newCursor = 0;
                }
                android.text.Selection.setSelection(text, Math.min(newCursor, text.length()));
                return true;
            }
            if (event.isCtrlPressed() || event.isAltPressed() || event.isMetaPressed()) {
                return false;
            }
            int codePoint = event.getUnicodeChar();
            if (codePoint < 32 || !Character.isValidCodePoint(codePoint)) return false;

            byte[] encoded = encodeCp437(new String(Character.toChars(codePoint)));
            if (encoded == null || encoded.length != 1) return true;
            android.text.Editable text = editor.getText();
            String value = text.toString();
            int start = Math.max(0, Math.min(editor.getSelectionStart(), value.length()));
            int end = Math.max(start, Math.min(editor.getSelectionEnd(), value.length()));
            int lineStart = value.lastIndexOf('\n', start - 1) + 1;
            int lineEnd = value.indexOf('\n', start);
            if (lineEnd < 0) lineEnd = value.length();
            int column = start - lineStart;
            int lineLength = lineEnd - lineStart;
            if (column >= 42 || (insertMode[0] && lineLength >= 42)) return true;

            if (insertMode[0]) return false;
            if (start == end && end < lineEnd) end++;
            String character = new String(encoded, CP437);
            text.replace(start, end, character);
            android.text.Selection.setSelection(text, start + character.length());
            return true;
        });
    }

    private void showHelpDialog(String title, String introduction, String assetName) {
        HelpDocument document = loadHelpDocument(assetName);
        SpannableStringBuilder text = new SpannableStringBuilder(introduction);
        int bodyStart = text.length();
        text.append(document.text);

        TextView content = new TextView(this);
        content.setTextColor(Color.WHITE);
        content.setTextSize(15);
        content.setTypeface(android.graphics.Typeface.MONOSPACE);
        content.setPadding(dp(12), dp(8), dp(12), dp(8));
        ScrollView scroll = new ScrollView(this);
        scroll.setPadding(dp(4), dp(4), dp(4), dp(4));
        scroll.addView(content, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(title)
                .setView(scroll)
                .setPositiveButton("OK", null)
                .create();

        for (HelpLink link : document.links) {
            int start = bodyStart + link.start;
            int end = bodyStart + link.end;
            text.setSpan(new ClickableSpan() {
                @Override public void onClick(View widget) {
                    if (link.target.startsWith("-")) {
                        String filename = link.target.substring(1);
                        if (filename.indexOf('.') < 0) filename += ".HLP";
                        dialog.dismiss();
                        showHelpDialog(title, introduction, filename.toUpperCase(Locale.ROOT));
                        return;
                    }

                    Integer anchor = document.anchors.get(link.target.toUpperCase(Locale.ROOT));
                    if (anchor == null) return;
                    int destination = bodyStart + anchor;
                    content.post(() -> {
                        Layout layout = content.getLayout();
                        if (layout == null) return;
                        int line = layout.getLineForOffset(Math.min(destination, content.length()));
                        scroll.smoothScrollTo(0, Math.max(0, layout.getLineTop(line) - scroll.getHeight() / 2));
                    });
                }

                @Override public void updateDrawState(TextPaint paint) {
                    paint.setColor(Color.rgb(85, 255, 255));
                    paint.setUnderlineText(true);
                }
            }, start, end, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
        }
        content.setText(text, TextView.BufferType.SPANNABLE);
        content.setMovementMethod(LinkMovementMethod.getInstance());
        showBlockedDialog(dialog);
    }

    private File findDosFile(File directory, String filename) {
        if (directory == null) return null;
        File requested = new File(directory, filename);
        if (requested.exists()) return requested;
        File[] files = directory.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.getName().equalsIgnoreCase(filename)) return file;
            }
        }
        return requested;
    }

    private HelpDocument loadHelpDocument(String assetName) {
        HelpDocument document = new HelpDocument();
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (InputStream input = getAssets().open(assetName)) {
            byte[] buffer = new byte[2048];
            int count;
            while ((count = input.read(buffer)) != -1) bytes.write(buffer, 0, count);
        } catch (IOException assetError) {
            File external = findDosFile(activeDirectory, assetName);
            if (external == null || !external.isFile()) {
                document.text.append("The original ").append(assetName)
                        .append(" document could not be loaded.");
                return document;
            }
            try (InputStream input = new FileInputStream(external)) {
                byte[] buffer = new byte[2048];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    if (bytes.size() + count > 65536) {
                        document.text.append("The original ").append(assetName)
                                .append(" document is too large to display.");
                        return document;
                    }
                    bytes.write(buffer, 0, count);
                }
            } catch (IOException e) {
                document.text.append("The original ").append(assetName)
                        .append(" document could not be loaded.");
                return document;
            }
        }

        byte[] raw = bytes.toByteArray();
        int dosEof = -1;
        for (int i = 0; i < raw.length; i++) {
            if ((raw[i] & 0xff) == 0x1a) {
                dosEof = i;
                break;
            }
        }
        String original = new String(raw, 0, dosEof < 0 ? raw.length : dosEof, CP437);
        String[] lines = original.split("\\r?\\n", -1);
        if (original.endsWith("\n") && lines.length > 0 && lines[lines.length - 1].isEmpty()) {
            lines = Arrays.copyOf(lines, lines.length - 1);
        }
        for (String line : lines) {
            // Pascal stores each text-window line in TTextWindowLine (string[50]).
            if (line.length() > 50) line = line.substring(0, 50);
            int separator = line.indexOf(';');
            String visible = line;
            if (line.startsWith("$")) {
                visible = line.substring(1).trim();
            } else if (separator >= 0 && (line.startsWith("!") || line.startsWith(":"))) {
                visible = line.substring(separator + 1);
            }

            int start = document.text.length();
            document.text.append(visible);
            int end = document.text.length();

            if (line.startsWith(":")) {
                String label = line.substring(1, separator >= 0 ? separator : line.length());
                if (label.length() > 20) label = label.substring(0, 20);
                document.anchors.putIfAbsent(label.toUpperCase(Locale.ROOT), start);
            } else if (line.startsWith("!") && end > start) {
                String target = line.substring(1, separator >= 0 ? separator : line.length());
                if (target.length() > 20) target = target.substring(0, 20);
                document.links.add(new HelpLink(start, end, target));
            }
            document.text.append('\n');
        }
        return document;
    }

    private boolean handleGameplayShortcut(int keyCode, KeyEvent event) {
        if (!ZztNative.gameCommandsAvailable()) return false;
        if (ZztNative.gameHasMonitor()
                && (keyCode == KeyEvent.KEYCODE_P || keyCode == KeyEvent.KEYCODE_Q
                    || keyCode == KeyEvent.KEYCODE_S || keyCode == KeyEvent.KEYCODE_H
                    || keyCode == KeyEvent.KEYCODE_B)) {
            return false;
        }
        if (keyCode == KeyEvent.KEYCODE_B) {
            int[] status = ZztNative.status();
            if (status != null && status.length > 14 && status[14] != 0) return false;
        }
        if (event.getUnicodeChar() == '?') {
            if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length) {
                handledGameplayKeyUps[keyCode] = true;
            }
            if (event.getRepeatCount() == 0) showDebugPrompt();
            return true;
        }
        switch (keyCode) {
            case KeyEvent.KEYCODE_SPACE:
            case KeyEvent.KEYCODE_Q:
            case KeyEvent.KEYCODE_P:
            case KeyEvent.KEYCODE_S:
            case KeyEvent.KEYCODE_B:
            case KeyEvent.KEYCODE_H:
            case KeyEvent.KEYCODE_F:
                if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length) {
                    handledGameplayKeyUps[keyCode] = true;
                }
                if (event.getRepeatCount() != 0) return true;
                switch (keyCode) {
                    case KeyEvent.KEYCODE_SPACE:
                        ZztNative.shootTap();
                        break;
                    case KeyEvent.KEYCODE_Q:
                        requestEndPlay();
                        break;
                    case KeyEvent.KEYCODE_P:
                        // Let ElementPlayerTick pause after finishing this cycle's timers, as Pascal does.
                        ZztNative.keyStroke('p');
                        refreshStatus();
                        break;
                    case KeyEvent.KEYCODE_S:
                        saveAndExport();
                        break;
                    case KeyEvent.KEYCODE_B:
                        boolean enabled = ZztNative.queueSoundToggle();
                        Toast.makeText(this, enabled ? "Sound on" : "Sound off", Toast.LENGTH_SHORT).show();
                        break;
                    case KeyEvent.KEYCODE_H:
                        showHelp();
                        break;
                    case KeyEvent.KEYCODE_F:
                        showOrderForm();
                        break;
                    default:
                        break;
                }
                return true;
            default:
                return false;
        }
    }

    private void confirmQuit() {
        AlertDialog quitDialog = new AlertDialog.Builder(this)
                .setTitle("Quit ZZT?")
                .setMessage("Leave ZZT for Android?")
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Quit", (dialog, which) -> finish())
                .create();
        showBlockedDialog(quitDialog);
    }

    private void handleEscapeAction() {
        if (editorActive) {
            requestEditorExit();
            return;
        }
        int[] status = ZztNative.status();
        if (status != null && status.length > 0 && status[0] == GAME_STATE_PLAY
                && !ZztNative.gameCommandsAvailable()) {
            ZztNative.keyStroke(KEY_ESCAPE);
            return;
        }
        if (status != null && status.length > 0 && status[0] == GAME_STATE_PLAY
                && ZztNative.gameHasMonitor()) {
            ZztNative.keyStroke(KEY_ESCAPE);
            return;
        }
        if (status == null || status.length == 0 || status[0] == GAME_STATE_TITLE) {
            confirmQuit();
        } else if (status[0] == GAME_STATE_PLAY) {
            ZztNative.keyStroke(KEY_ESCAPE);
            requestEndPlay();
        }
    }

    private void requestEndPlay() {
        int[] status = ZztNative.status();
        if (status == null || status.length < 15 || status[0] != GAME_STATE_PLAY) return;
        if (status[1] <= 0) {
            acquireModalPromptBlock();
            recordDeathScore(status[5]);
            return;
        }

        acquireModalPromptBlock();
        AlertDialog prompt = new AlertDialog.Builder(this)
                .setTitle("End this game?")
                .setNegativeButton("No", (dialog, which) -> {
                    releaseModalPromptBlock();
                    refreshStatus();
                })
                .setPositiveButton("Yes", (dialog, which) -> returnToTitle())
                .create();
        prompt.setOnCancelListener(dialog -> {
            releaseModalPromptBlock();
            refreshStatus();
        });
        prompt.show();
    }

    private void returnToTitle() {
        int result = ZztNative.returnToTitle();
        releaseModalPromptBlock();
        if (result != 0) toastError("Return to title", result);
        refreshStatus();
    }

    private File highScoreFile() {
        String base = safeWorldFileStem(ZztNative.worldName());
        return new File(activeDirectory, base + ".HI");
    }

    private void showHighScores() {
        highScores.loadInto(highScoreFile());
        AlertDialog.Builder dialog = new AlertDialog.Builder(this)
                .setTitle("High scores for " + ZztNative.worldName())
                .setNeutralButton("Export", (d, which) -> exportHighScores())
                .setPositiveButton("OK", null);
        if (highScores.hasNamedEntries()) {
            dialog.setItems(highScores.displayRows(), null);
        } else {
            dialog.setMessage("No high scores yet.");
        }
        showBlockedDialog(dialog.create());
    }

    private void recordDeathScore(int score) {
        File file = highScoreFile();
        HighScores scores = highScores;
        int position = scores.insertionPosition(score);
        if (position < 0) {
            returnToTitle();
            return;
        }

        EditText name = new EditText(this);
        name.setSingleLine(true);
        name.setFilters(new InputFilter[]{
                new InputFilter.LengthFilter(34),
                pascalPromptCharacterFilter()
        });
        name.setHint("Your name");
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("New high score for " + ZztNative.worldName())
                .setMessage("Congratulations! Enter your name:")
                .setView(name)
                .setPositiveButton("Save", null)
                .setNegativeButton("Skip", (d, which) -> saveDeathScore(scores, file, position, score, ""))
                .create();
        dialog.setOnCancelListener(d -> saveDeathScore(scores, file, position, score, ""));
        dialog.setCanceledOnTouchOutside(false);
        dialog.setOnShowListener(d -> {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
                if (saveDeathScore(scores, file, position, score, name.getText().toString())) {
                    dialog.dismiss();
                } else {
                    Toast.makeText(this, "Use characters available in IBM PC code page 437.",
                            Toast.LENGTH_LONG).show();
                }
            });
            name.requestFocus();
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) imm.showSoftInput(name, InputMethodManager.SHOW_IMPLICIT);
        });
        dialog.show();
    }

    private boolean saveDeathScore(HighScores scores, File file, int position, int score, String name) {
        if (!scores.insert(position, score, name)) return false;
        if (!scores.save(file)) toastError("Save high scores", -1);
        returnToTitle();
        return true;
    }

    private void exportHighScores() {
        File file = highScoreFile();
        if (!file.isFile()) {
            Toast.makeText(this, "This world has no high score file yet.", Toast.LENGTH_SHORT).show();
            return;
        }
        pendingExport = file;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE, file.getName());
        try {
            startExternalActivity(intent, EXPORT_HIGHSCORES);
        } catch (Exception e) {
            Toast.makeText(this, "No export picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private boolean handleTitleShortcut(int keyCode) {
        int[] status = ZztNative.status();
        if (editorActive || status == null || status.length == 0 || status[0] != GAME_STATE_TITLE) return false;

        switch (keyCode) {
            case KeyEvent.KEYCODE_P: {
                int result = startGameFromTitle();
                if (result != 0) toastError("Start game", result);
                refreshStatus();
                return true;
            }
            case KeyEvent.KEYCODE_W:
                pickWorld(false);
                return true;
            case KeyEvent.KEYCODE_R:
                pickWorld(true);
                return true;
            case KeyEvent.KEYCODE_A:
                showAbout();
                return true;
            case KeyEvent.KEYCODE_S:
                showSpeedDialog();
                return true;
            case KeyEvent.KEYCODE_H:
                showHighScores();
                return true;
            case KeyEvent.KEYCODE_E:
                beginEditor();
                return true;
            case KeyEvent.KEYCODE_Q:
            case KeyEvent.KEYCODE_ESCAPE:
                confirmQuit();
                return true;
            default:
                return false;
        }
    }

    private boolean handleEditorShortcut(int keyCode, KeyEvent event) {
        if (!editorActive) return false;
        if (event.getRepeatCount() != 0) return true;

        if (keyCode == KeyEvent.KEYCODE_SHIFT_LEFT || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT) {
            editorShiftHeld = true;
            editorShiftPainted = false;
            return true;
        }
        if (handleEditorMovementKey(keyCode, event)) return true;
        if (editorTextMode) return handleEditorTextKey(keyCode, event);

        switch (keyCode) {
            case KeyEvent.KEYCODE_H:
                showEditorHelp();
                return true;
            case KeyEvent.KEYCODE_ESCAPE:
            case KeyEvent.KEYCODE_Q:
            case KeyEvent.KEYCODE_BACK:
                requestEditorExit();
                return true;
            case KeyEvent.KEYCODE_MENU:
                showMenu();
                return true;
            case KeyEvent.KEYCODE_GRAVE: {
                int result = ZztNative.editorRefreshBoard();
                if (result != 0) toastError("Refresh editor board", result);
                updateDisplay(System.nanoTime() / 1_000_000L);
                return true;
            }
            case KeyEvent.KEYCODE_F1:
                showEditorPalette(CATEGORY_ITEM, "Items");
                return true;
            case KeyEvent.KEYCODE_F2:
                showEditorPalette(CATEGORY_CREATURE, "Creatures");
                return true;
            case KeyEvent.KEYCODE_F3:
                showEditorPalette(CATEGORY_TERRAIN, "Terrain");
                return true;
            case KeyEvent.KEYCODE_F4:
                toggleEditorTextMode();
                return true;
            case KeyEvent.KEYCODE_TAB:
                toggleEditorDrawMode();
                return true;
            case KeyEvent.KEYCODE_SPACE:
                plotEditorCursor();
                return true;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER:
                activateEditorCursor();
                return true;
            case KeyEvent.KEYCODE_B:
                showEditorBoards();
                return true;
            case KeyEvent.KEYCODE_I:
                showEditorBoardInfoDialog();
                return true;
            case KeyEvent.KEYCODE_T:
                showEditorBoardTransferDialog();
                return true;
            case KeyEvent.KEYCODE_P:
                cycleEditorPattern();
                return true;
            case KeyEvent.KEYCODE_C:
                cycleEditorColor();
                return true;
            case KeyEvent.KEYCODE_X:
                toggleEditorFloodFill(false);
                return true;
            case KeyEvent.KEYCODE_Z:
                confirmClearEditorBoard();
                return true;
            case KeyEvent.KEYCODE_S:
                saveWorldToApp();
                return true;
            case KeyEvent.KEYCODE_L:
                requestEditorWorldLoad();
                return true;
            case KeyEvent.KEYCODE_N:
                confirmEditorNewWorld();
                return true;
            default:
                if (event.getUnicodeChar() == '?') {
                    showDebugPrompt();
                    return true;
                }
                if (event.getUnicodeChar() == '!') {
                    editHelpFilePrompt();
                    return true;
                }
                return true;
        }
    }

    private boolean handleEditorMovementKey(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP:
            case KeyEvent.KEYCODE_NUMPAD_8:
                moveEditorCursor(0, -1);
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
            case KeyEvent.KEYCODE_NUMPAD_2:
                moveEditorCursor(0, 1);
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
            case KeyEvent.KEYCODE_NUMPAD_4:
                moveEditorCursor(-1, 0);
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
            case KeyEvent.KEYCODE_NUMPAD_6:
                moveEditorCursor(1, 0);
                return true;
            default:
                int direction = numericDirectionForKey(keyCode, event);
                if (direction == KEY_UP) moveEditorCursor(0, -1);
                else if (direction == KEY_DOWN) moveEditorCursor(0, 1);
                else if (direction == KEY_LEFT) moveEditorCursor(-1, 0);
                else if (direction == KEY_RIGHT) moveEditorCursor(1, 0);
                else return false;
                return true;
        }
    }

    private void requestEditorWorldLoad() {
        if (!hasEditorChanges()) {
            launchEditorWorldPicker();
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Load another world?")
                .setMessage("Save the current editor changes before loading?")
                .setPositiveButton("Save and load", (dialog, which) -> {
                    if (saveWorldToApp()) launchEditorWorldPicker();
                })
                .setNegativeButton("Discard and load", (dialog, which) -> launchEditorWorldPicker())
                .setNeutralButton("Cancel", null)
                .show();
    }

    private void launchEditorWorldPicker() {
        pickWorld(false);
    }

    private void confirmEditorNewWorld() {
        AlertDialog.Builder dialog = new AlertDialog.Builder(this)
                .setTitle("Create a new world?");
        if (hasEditorChanges()) {
            dialog.setMessage("Save the current editor changes before replacing this world?")
                    .setPositiveButton("Save and create", (ignored, which) -> {
                        if (saveWorldToApp()) createNewEditorWorld();
                    })
                    .setNegativeButton("Discard and create", (ignored, which) -> createNewEditorWorld())
                    .setNeutralButton("Cancel", null);
        } else {
            dialog.setMessage("The current world will be replaced by a blank editing world.")
                    .setPositiveButton("Create", (ignored, which) -> createNewEditorWorld())
                    .setNegativeButton("Cancel", null);
        }
        dialog.show();
    }

    private void createNewEditorWorld() {
        File worlds = new File(getFilesDir(), "worlds");
        if (!worlds.exists() && !worlds.mkdirs()) {
            Toast.makeText(this, "Could not prepare app storage for the new world.", Toast.LENGTH_LONG).show();
            return;
        }
        File workspace = new File(worlds, "editor-" + System.currentTimeMillis());
        if (!workspace.mkdirs()) {
            Toast.makeText(this, "Could not prepare a private folder for the new world.", Toast.LENGTH_LONG).show();
            return;
        }

        File previousDirectory = activeDirectory;
        int result = ZztNative.setWorldDirectory(workspace.getAbsolutePath());
        if (result == 0) result = ZztNative.editorNewWorld();
        if (result != 0) {
            if (previousDirectory != null) ZztNative.setWorldDirectory(previousDirectory.getAbsolutePath());
            workspace.delete();
            toastError("Create new world", result);
            return;
        }

        activeDirectory = workspace;
        activeFilename = "world.ZZT";
        // Pascal's WorldCreate leaves the global high-score table untouched.
        editorModified = false;
        editorCopyMode = false;
        editorPasteMode = false;
        editorFloodFillMode = false;
        editorFloodFillUsesClipboard = false;
        editorShiftHeld = false;
        editorShiftPainted = false;
        if (editorCellX < 1 || editorCellX > BoardView.COLS
                || editorCellY < 1 || editorCellY > BoardView.ROWS) {
            editorCellX = BoardView.COLS / 2;
            editorCellY = BoardView.ROWS / 2;
        }
        boardView.setEditorCursor(editorCellX, editorCellY);
        boardView.setContinuousCellMode(editorDrawMode);
        updateDisplay(System.nanoTime() / 1_000_000L);
        refreshStatus();
        Toast.makeText(this, "Blank world created. Save it to keep your changes.", Toast.LENGTH_LONG).show();
    }

    private boolean handleTitleDebugShortcut(KeyEvent event) {
        if (event.getUnicodeChar() != '|') return false;
        int[] status = ZztNative.status();
        if (editorActive || status == null || status.length == 0 || status[0] != GAME_STATE_TITLE) {
            return false;
        }
        showDebugPrompt();
        return true;
    }

    private void showDebugPrompt() {
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setFilters(new InputFilter[] {
                new InputFilter.LengthFilter(11), pascalPromptCharacterFilter()
        });
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("ZZT debug command")
                .setMessage("Enter command:")
                .setView(input)
                .setPositiveButton("Run", null)
                .setNegativeButton("Cancel", null)
                .create();
        dialog.setOnShowListener(d -> {
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
                if (runDebugCommand(input.getText().toString())) dialog.dismiss();
            });
            input.setOnEditorActionListener((v, actionId, event) -> {
                if (actionId == EditorInfo.IME_ACTION_DONE
                        || (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                    if (runDebugCommand(input.getText().toString())) dialog.dismiss();
                    return true;
                }
                return false;
            });
            input.requestFocus();
            input.postDelayed(() -> {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) imm.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
            }, 120);
        });
        showBlockedDialog(dialog);
    }

    private boolean runDebugCommand(String command) {
        byte[] encoded = encodeCp437(command);
        if (encoded == null) {
            Toast.makeText(this, "ZZT commands can only contain characters in IBM PC code page 437.",
                    Toast.LENGTH_LONG).show();
            return false;
        }
        ZztNative.debugCommand(encoded);
        refreshStatus();
        return true;
    }

    private void showAbout() {
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("About ZZT")
                .setMessage("ZZT for Android\n\nA native Android port of the reconstructed ZZT 3.2 engine. The board keeps its original 60 by 25 character layout.")
                .setPositiveButton("OK", null)
                .create();
        showBlockedDialog(dialog);
    }

    private void showSpeedDialog() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(24), dp(12), dp(24), dp(4));

        TextView value = label("Speed: " + (ZztNative.tickSpeed() + 1), 16, Color.WHITE);
        SeekBar speed = new SeekBar(this);
        speed.setMax(8);
        speed.setProgress(ZztNative.tickSpeed());
        speed.setContentDescription("Game speed, from 1 slowest to 9 fastest");
        speed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                ZztNative.setTickSpeed(progress);
                value.setText("Speed: " + (progress + 1));
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
        content.addView(value);
        content.addView(speed, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(52)));

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Game speed")
                .setView(content)
                .setPositiveButton("Done", null)
                .create();
        showBlockedDialog(dialog);
    }

    private void beginEditor() {
        int result = ZztNative.editorBegin();
        if (result < 0) {
            Toast.makeText(this, "The board editor is available from the title screen.", Toast.LENGTH_LONG).show();
            return;
        }
        if (result > 0) {
            File worlds = new File(getFilesDir(), "worlds");
            File workspace = new File(worlds, "editor-" + System.currentTimeMillis());
            if (!workspace.mkdirs() && !workspace.isDirectory()) {
                ZztNative.editorEnd();
                Toast.makeText(this, "Could not prepare a private folder for the new editor world.",
                        Toast.LENGTH_LONG).show();
                return;
            }
            int directoryResult = ZztNative.setWorldDirectory(workspace.getAbsolutePath());
            if (directoryResult != 0) {
                ZztNative.editorEnd();
                toastError("Prepare editor world", directoryResult);
                return;
            }
            activeDirectory = workspace;
            activeFilename = "world.ZZT";
            // Entering a fresh editor world also preserves Pascal's loaded table.
            Toast.makeText(this,
                    "Original ZZT starts a fresh world when editing a save or a SECRET-flagged world. The imported file remains in its original folder.",
                    Toast.LENGTH_LONG).show();
        }
        editorActive = true;
        editorModified = false;
        editorElement = 21;
        editorElementName = "Solid";
        editorColor = 14;
        editorPatternIndex = 0;
        editorClipboardPattern = false;
        editorCopyMode = false;
        editorPasteMode = false;
        editorClipboardValid = false;
        editorFloodFillMode = false;
        editorFloodFillUsesClipboard = false;
        editorTextMode = false;
        editorDrawMode = false;
        editorShiftHeld = false;
        editorShiftPainted = false;
        editorTextCharacter = 'A';
        editorCellX = BoardView.COLS / 2;
        editorCellY = BoardView.ROWS / 2;
        controls.setVisibility(View.GONE);
        tools.setVisibility(View.GONE);
        editorTools.setVisibility(View.VISIBLE);
        boardView.setCellTapListener(this::onEditorCellTapped);
        boardView.setEditorCursor(editorCellX, editorCellY);
        boardView.setContinuousCellMode(false);
        boardView.requestFocus();
        refreshStatus();
    }

    private void onEditorCellTapped(int x, int y) {
        editorCellX = x;
        editorCellY = y;
        boardView.setEditorCursor(x, y);
        if (editorCopyMode) {
            editorCopyMode = false;
            int result = ZztNative.editorCopyTile(x, y);
            if (result == 0) {
                editorClipboardValid = true;
                Toast.makeText(this, "Tile copied, including its stat and code.", Toast.LENGTH_SHORT).show();
            } else {
                toastError("Copy tile", result);
            }
            refreshStatus();
            return;
        }
        if (editorPasteMode) {
            editorPasteMode = false;
            int result = ZztNative.editorPasteTile(x, y);
            if (result == 1) {
                Toast.makeText(this, "Cannot paste onto the player or beyond the stat limit.",
                        Toast.LENGTH_LONG).show();
            } else if (result == 2) {
                Toast.makeText(this, "Copy a tile before pasting.", Toast.LENGTH_LONG).show();
            } else if (result != 0) {
                toastError("Paste tile", result);
            } else {
                editorModified = true;
                updateDisplay(System.nanoTime() / 1_000_000L);
            }
            refreshStatus();
            return;
        }
        if (editorFloodFillMode) {
            editorFloodFillMode = false;
            int filled = ZztNative.editorFloodFill(x, y, editorElement, editorColor,
                    editorFloodFillUsesClipboard, !editorFloodFillUsesClipboard);
            editorFloodFillUsesClipboard = false;
            if (filled < 0) {
                toastError("Flood fill", filled);
            } else if (filled == 0) {
                Toast.makeText(this, "No connected tiles changed.", Toast.LENGTH_SHORT).show();
            } else {
                editorModified = true;
                Toast.makeText(this, "Filled " + filled + " tiles.", Toast.LENGTH_SHORT).show();
            }
            updateDisplay(System.nanoTime() / 1_000_000L);
            return;
        }
        if (editorTextMode) {
            int result = ZztNative.editorPlaceText(x, y, editorTextCharacter, editorColor);
            if (result == 1) {
                Toast.makeText(this, "The player tile cannot be overwritten.", Toast.LENGTH_SHORT).show();
            } else if (result != 0) {
                toastError("Place text", result);
            } else {
                editorModified = true;
            }
            updateDisplay(System.nanoTime() / 1_000_000L);
            return;
        }
        placeEditorElementAt(x, y, !editorDrawMode);
    }

    private void placeEditorElementAt(int x, int y, boolean showParameters) {
        placeEditorElementAt(x, y, showParameters, true);
    }

    private void placeEditorElementAt(int x, int y, boolean showParameters,
                                      boolean usePatternColor) {
        int result = editorClipboardPattern
                ? ZztNative.editorPlaceClipboardPattern(x, y)
                : ZztNative.editorPlace(x, y, editorElement, editorColor, usePatternColor);
        if (result == 1) {
            Toast.makeText(this, "The player tile cannot be overwritten.", Toast.LENGTH_SHORT).show();
            return;
        }
        if (result != 0) {
            toastError("Place " + editorElementName, result);
            return;
        }
        editorModified = true;
        updateDisplay(System.nanoTime() / 1_000_000L);
        if (editorClipboardPattern) {
            refreshStatus();
            return;
        }
        int[] values = ZztNative.editorStatValues(x, y);
        String[] parameters = values == null ? null : ZztNative.editorElementParameters(values[1]);
        if (showParameters && hasEditorParameters(parameters)) {
            showEditorStatDialog(x, y, true, values, parameters);
        } else if (showParameters && values != null && editorElement != ELEMENT_PLAYER) {
            int copyResult = ZztNative.editorCopyTile(x, y);
            if (copyResult == 0) editorClipboardValid = true;
            else toastError("Copy placed stat", copyResult);
            refreshStatus();
        }
    }

    private void moveEditorCursor(int dx, int dy) {
        if (editorCellX < 1 || editorCellY < 1) return;
        if (editorDrawMode || editorShiftHeld) {
            placeEditorElementAt(editorCellX, editorCellY, false);
            if (editorShiftHeld) editorShiftPainted = true;
        }
        editorCellX = clamp(editorCellX + dx, 1, BoardView.COLS);
        editorCellY = clamp(editorCellY + dy, 1, BoardView.ROWS);
        boardView.setEditorCursor(editorCellX, editorCellY);
    }

    private void plotEditorCursor() {
        if (editorCellX < 1 || editorCellY < 1) return;
        if (editorCopyMode || editorPasteMode || editorFloodFillMode || editorTextMode) {
            onEditorCellTapped(editorCellX, editorCellY);
            return;
        }
        int result = editorClipboardPattern
                ? ZztNative.editorPlotClipboardPattern(editorCellX, editorCellY)
                : ZztNative.editorPlot(editorCellX, editorCellY, editorElement, editorColor, true);
        if (result == 1) {
            Toast.makeText(this, "The player tile cannot be overwritten.", Toast.LENGTH_SHORT).show();
        } else if (result != 0) {
            toastError("Plot tile", result);
        } else {
            editorModified = true;
            updateDisplay(System.nanoTime() / 1_000_000L);
        }
    }

    private void activateEditorCursor() {
        if (editorCellX < 1 || editorCellY < 1) return;
        int[] values = ZztNative.editorStatValues(editorCellX, editorCellY);
        if (values == null) {
            int result = ZztNative.editorCopyTile(editorCellX, editorCellY);
            if (result == 0) {
                editorClipboardValid = true;
                Toast.makeText(this, "Tile copied, including its stat and code.", Toast.LENGTH_SHORT).show();
            } else {
                toastError("Copy tile", result);
            }
            refreshStatus();
            return;
        }
        String[] parameters = ZztNative.editorElementParameters(values[1]);
        if (hasEditorParameters(parameters)) {
            showEditorStatDialog(editorCellX, editorCellY, false, values, parameters);
        } else {
            int result = ZztNative.editorCopyTile(editorCellX, editorCellY);
            if (result == 0) {
                editorClipboardValid = true;
                editorModified = true;
                Toast.makeText(this, "Stat copied for placement.", Toast.LENGTH_SHORT).show();
            } else {
                toastError("Copy stat", result);
            }
            refreshStatus();
        }
    }

    private void toggleEditorDrawMode() {
        editorDrawMode = !editorDrawMode;
        if (editorDrawMode) {
            editorCopyMode = false;
            editorPasteMode = false;
            editorFloodFillMode = false;
            editorFloodFillUsesClipboard = false;
            editorTextMode = false;
            placeEditorElementAt(editorCellX, editorCellY, false);
        }
        boardView.setContinuousCellMode(editorDrawMode);
        Toast.makeText(this, editorDrawMode
                ? "Draw mode on. Drag across the board to paint."
                : "Draw mode off.", Toast.LENGTH_SHORT).show();
        refreshStatus();
    }

    private void toggleEditorTextMode() {
        editorTextMode = !editorTextMode;
        if (editorTextMode) {
            editorDrawMode = false;
            editorCopyMode = false;
            editorPasteMode = false;
            editorFloodFillMode = false;
            editorFloodFillUsesClipboard = false;
        }
        boardView.setContinuousCellMode(editorDrawMode);
        refreshStatus();
    }

    private boolean handleEditorTextKey(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_F4 || keyCode == KeyEvent.KEYCODE_ESCAPE
                || keyCode == KeyEvent.KEYCODE_ENTER || keyCode == KeyEvent.KEYCODE_NUMPAD_ENTER
                || keyCode == KeyEvent.KEYCODE_TAB || keyCode == KeyEvent.KEYCODE_BACK) {
            editorTextMode = false;
            editorDrawMode = false;
            boardView.setContinuousCellMode(false);
            refreshStatus();
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_DEL || keyCode == KeyEvent.KEYCODE_FORWARD_DEL) {
            if (editorCellX > 1) {
                int result = ZztNative.editorPrepareTile(editorCellX - 1, editorCellY);
                if (result == 0) {
                    editorModified = true;
                    editorCellX--;
                    boardView.setEditorCursor(editorCellX, editorCellY);
                    updateDisplay(System.nanoTime() / 1_000_000L);
                } else if (result != 1) {
                    toastError("Erase text tile", result);
                }
            }
            return true;
        }
        int character = event.getUnicodeChar();
        if (character >= 32 && character < 128) {
            int result = ZztNative.editorPlaceText(editorCellX, editorCellY, character, editorColor);
            if (result == 0) {
                editorModified = true;
                editorTextCharacter = character;
                editorCellX = clamp(editorCellX + 1, 1, BoardView.COLS);
                boardView.setEditorCursor(editorCellX, editorCellY);
                updateDisplay(System.nanoTime() / 1_000_000L);
                refreshStatus();
            } else if (result == 1) {
                Toast.makeText(this, "The player tile cannot be overwritten.", Toast.LENGTH_SHORT).show();
            } else {
                toastError("Place text", result);
            }
        }
        return true;
    }

    private static boolean hasEditorParameters(String[] parameters) {
        if (parameters == null) return false;
        for (String parameter : parameters) {
            if (parameter != null && !parameter.isEmpty()) return true;
        }
        return false;
    }

    int startGameFromTitle() {
        int[] status = ZztNative.status();
        if (status == null || status.length == 0 || status[0] != GAME_STATE_TITLE) return 0;
        int result = ZztNative.startGame();
        if (result == 0) {
            highScores.loadInto(highScoreFile());
            if (playTool != null) playTool.setVisibility(View.GONE);
            if (boardView != null) boardView.requestFocus();
        }
        return result;
    }

    private void editSelectedStat() {
        if (editorCellX < 1 || editorCellY < 1) {
            Toast.makeText(this, "Tap a stat on the board first.", Toast.LENGTH_SHORT).show();
            return;
        }
        int[] values = ZztNative.editorStatValues(editorCellX, editorCellY);
        if (values == null) {
            Toast.makeText(this, "There is no stat at that cell.", Toast.LENGTH_SHORT).show();
            return;
        }
        String[] parameters = ZztNative.editorElementParameters(values[1]);
        if (!hasEditorParameters(parameters)) {
            Toast.makeText(this, "That stat has no editable parameters.", Toast.LENGTH_SHORT).show();
            return;
        }
        showEditorStatDialog(editorCellX, editorCellY, false, values, parameters);
    }

    private void showEditorStatDialog(int x, int y, boolean newlyPlaced,
                                      int[] stat, String[] parameters) {
        // Pascal marks the board modified as soon as EditorEditStat is entered,
        // even if the user later exits that prompt without changing a value.
        editorModified = true;
        LinearLayout fields = new LinearLayout(this);
        fields.setOrientation(LinearLayout.VERTICAL);
        fields.setPadding(dp(12), dp(4), dp(12), dp(8));

        boolean hasP1 = !parameters[0].isEmpty();
        boolean hasP2 = !parameters[1].isEmpty();
        boolean hasBulletType = !parameters[2].isEmpty();
        boolean hasBoard = !parameters[3].isEmpty();
        boolean hasDirection = !parameters[4].isEmpty();
        boolean hasText = !parameters[5].isEmpty();
        final boolean[] p1Touched = {false};
        final boolean[] p2Touched = {false};
        NumberPicker p1Picker = null;
        NumberPicker p2Picker = null;

        if (hasP1) {
            int max = hasText ? 255 : 8;
            int initial = clamp(stat[2], 0, max);
            p1Picker = addEditorNumberField(fields, parameters[0], 0, max, initial,
                    p1Touched);
        }
        if (hasText) {
            TextView help = label(parameters[5] + " (IBM PC code page 437; 50 characters per line)",
                    13, Color.LTGRAY);
            help.setPadding(0, dp(8), 0, dp(4));
            fields.addView(help);
        }
        if (hasP2) {
            int initial = clamp(stat[3] & 0x7f, 0, 8);
            p2Picker = addEditorNumberField(fields, parameters[1], 0, 8, initial,
                    p2Touched);
        }

        Spinner bulletPicker = null;
        int initialBullet = (stat[3] >>> 7) & 1;
        if (hasBulletType) {
            bulletPicker = addEditorChoiceField(fields, parameters[2],
                    new String[] {"Bullets", "Stars"}, initialBullet);
        }

        Spinner directionPicker = null;
        int initialDirection = directionIndex(stat[5], stat[6]);
        if (hasDirection) {
            directionPicker = addEditorChoiceField(fields, parameters[4],
                    new String[] {"Up", "Down", "Left", "Right"}, initialDirection);
        }

        Spinner boardPicker = null;
        int initialBoard = stat[4];
        int displayedBoard = initialBoard;
        if (hasBoard) {
            int[] status = ZztNative.status();
            int boardCount = status != null && status.length > 7 ? Math.max(0, status[7]) : 0;
            String[] boardNames = new String[boardCount + 1];
            boardNames[0] = "None (0)";
            for (int i = 1; i <= boardCount; i++) {
                byte[] rawName = ZztNative.editorBoardNameBytes(i);
                String name = rawName == null ? "" : new String(rawName, CP437);
                boardNames[i] = name.isEmpty() ? ("Board " + i) : (i + ": " + name);
            }
            displayedBoard = clamp(initialBoard, 0, boardCount);
            boardPicker = addEditorChoiceField(fields, parameters[3], boardNames, displayedBoard);
        }

        EditText textEditor = null;
        String originalTextShown = "";
        boolean textInitiallyEmpty = false;
        if (hasText) {
            byte[] storedText = ZztNative.editorStatText(x, y);
            textInitiallyEmpty = storedText == null || storedText.length == 0;
            String decoded = storedText == null ? "" : new String(storedText, CP437);
            originalTextShown = decoded.replace('\r', '\n');
            if (storedText != null && storedText.length > 0
                    && storedText[storedText.length - 1] == 13 && originalTextShown.endsWith("\n")) {
                originalTextShown = originalTextShown.substring(0, originalTextShown.length() - 1);
            }
            textEditor = new EditText(this);
            textEditor.setGravity(Gravity.TOP | Gravity.START);
            textEditor.setInputType(InputType.TYPE_CLASS_TEXT
                    | InputType.TYPE_TEXT_FLAG_MULTI_LINE
                    | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
            textEditor.setTypeface(android.graphics.Typeface.MONOSPACE);
            textEditor.setMinLines(5);
            textEditor.setMaxLines(12);
            installPascalTextWindowControls(textEditor);
            textEditor.setText(originalTextShown);
            fields.addView(textEditor, new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, dp(180)));
        }

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(false);
        scroll.addView(fields);
        final NumberPicker finalP1Picker = p1Picker;
        final NumberPicker finalP2Picker = p2Picker;
        final Spinner finalBulletPicker = bulletPicker;
        final Spinner finalDirectionPicker = directionPicker;
        final Spinner finalBoardPicker = boardPicker;
        final EditText finalTextEditor = textEditor;
        final String initialText = originalTextShown;
        final boolean initialTextEmpty = textInitiallyEmpty;
        final int directionShown = initialDirection;
        final int boardShown = displayedBoard;
        final boolean[] undoDone = {false};

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Edit stat")
                .setView(scroll)
                .setNegativeButton(newlyPlaced ? "Undo placement" : "Cancel", (d, which) -> {
                    if (newlyPlaced) removeNewlyPlacedStat(x, y, undoDone);
                })
                .setPositiveButton("Apply", null)
                .create();
        dialog.setOnCancelListener(d -> {
            if (newlyPlaced) removeNewlyPlacedStat(x, y, undoDone);
        });
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    byte[] textBytes = null;
                    if (finalTextEditor != null && (newlyPlaced || initialTextEmpty
                            || !finalTextEditor.getText().toString().equals(initialText))) {
                        textBytes = encodeEditorStatText(finalTextEditor, initialText);
                        if (textBytes == null) return;
                    }

                    int p1 = stat[2];
                    if (finalP1Picker != null && p1Touched[0]) p1 = finalP1Picker.getValue();
                    int p2 = stat[3];
                    if (finalP2Picker != null && p2Touched[0]) {
                        p2 = (p2 & 0x80) | finalP2Picker.getValue();
                    }
                    if (finalBulletPicker != null
                            && finalBulletPicker.getSelectedItemPosition() != initialBullet) {
                        p2 = (p2 & 0x7f) | (finalBulletPicker.getSelectedItemPosition() << 7);
                    }
                    int p3 = stat[4];
                    if (finalBoardPicker != null
                            && finalBoardPicker.getSelectedItemPosition() != boardShown) {
                        p3 = finalBoardPicker.getSelectedItemPosition();
                    }
                    int stepX = stat[5];
                    int stepY = stat[6];
                    if (finalDirectionPicker != null
                            && finalDirectionPicker.getSelectedItemPosition() != directionShown) {
                        int direction = finalDirectionPicker.getSelectedItemPosition();
                        stepX = direction == 2 ? -1 : direction == 3 ? 1 : 0;
                        stepY = direction == 0 ? -1 : direction == 1 ? 1 : 0;
                    }

                    if (textBytes != null) {
                        int result = ZztNative.editorUpdateStatText(x, y, textBytes);
                        if (result != 0) {
                            toastError("Save stat text", result);
                            return;
                        }
                    }
                    int result = ZztNative.editorUpdateStat(x, y, p1, p2, p3, stepX, stepY);
                    if (result != 0) {
                        toastError("Save stat settings", result);
                        return;
                    }
                    editorModified = true;
                    int copyResult = ZztNative.editorCopyTile(x, y);
                    if (copyResult == 0) editorClipboardValid = true;
                    dialog.dismiss();
                    updateDisplay(System.nanoTime() / 1_000_000L);
                    refreshStatus();
                }));
        dialog.show();
    }

    private NumberPicker addEditorNumberField(LinearLayout fields, String title, int min, int max,
                                              int value, boolean[] touched) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        TextView caption = label(title, 14, Color.WHITE);
        row.addView(caption, new LinearLayout.LayoutParams(0, dp(50), 1));
        NumberPicker picker = new NumberPicker(this);
        picker.setMinValue(min);
        picker.setMaxValue(max);
        picker.setWrapSelectorWheel(false);
        picker.setValue(clamp(value, min, max));
        picker.setOnValueChangedListener((view, oldValue, newValue) -> touched[0] = true);
        row.addView(picker, new LinearLayout.LayoutParams(dp(84), dp(54)));
        fields.addView(row);
        return picker;
    }

    private void trackEditorFieldInteraction(View field, boolean[] visited) {
        field.setOnTouchListener((view, event) -> {
            if (event.getActionMasked() == MotionEvent.ACTION_UP) visited[0] = true;
            return false;
        });
    }

    private Spinner addEditorChoiceField(LinearLayout fields, String title,
                                         String[] choices, int selected) {
        TextView caption = label(title, 14, Color.WHITE);
        caption.setPadding(0, dp(6), 0, 0);
        fields.addView(caption);
        Spinner spinner = new Spinner(this);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this,
                android.R.layout.simple_spinner_item, choices);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        spinner.setSelection(clamp(selected, 0, Math.max(0, choices.length - 1)));
        fields.addView(spinner, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(48)));
        return spinner;
    }

    private byte[] encodeEditorStatText(EditText editor, String originalText) {
        String[] lines = editorHelpLines(editor.getText().toString());
        String[] originalLines = editorHelpLines(originalText);
        if (lines.length > 1024) {
            Toast.makeText(this, "ZZT text supports at most 1024 lines.", Toast.LENGTH_LONG).show();
            return null;
        }
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        for (String line : lines) {
            byte[] encodedLine = encodeCp437(line);
            if (encodedLine == null) {
                Toast.makeText(this, "ZZT text can only contain characters in IBM PC code page 437.",
                        Toast.LENGTH_LONG).show();
                return null;
            }
            if (encodedLine.length > 50 || (encodedLine.length > 42
                    && !isPascalEditableLine(encodedLine, originalLines))) {
                Toast.makeText(this, "New ZZT text lines can contain at most 42 CP437 bytes.", Toast.LENGTH_LONG).show();
                return null;
            }
            if (bytes.size() + encodedLine.length + 1 > 32767) {
                Toast.makeText(this, "This stat's text is too large for a ZZT file.", Toast.LENGTH_LONG).show();
                return null;
            }
            bytes.write(encodedLine, 0, encodedLine.length);
            bytes.write(13);
        }
        return bytes.toByteArray();
    }

    private static byte[] encodeCp437(String text) {
        try {
            ByteBuffer encoded = CP437.newEncoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .encode(CharBuffer.wrap(text));
            byte[] bytes = new byte[encoded.remaining()];
            encoded.get(bytes);
            return bytes;
        } catch (CharacterCodingException e) {
            return null;
        }
    }

    private static InputFilter pascalPromptCharacterFilter() {
        return (source, start, end, dest, dstart, dend) -> {
            StringBuilder accepted = new StringBuilder(end - start);
            for (int i = start; i < end; i++) {
                char character = source.charAt(i);
                if (character >= 32 && character < 128) accepted.append(character);
            }
            return accepted.length() == end - start ? null : accepted;
        };
    }

    private static boolean isPascalPromptText(String text) {
        for (int i = 0; i < text.length(); i++) {
            char character = text.charAt(i);
            if (character < 32 || character >= 128) return false;
        }
        return true;
    }

    private static boolean matchesOriginalPromptEdit(String original, String edited) {
        if (original.equals(edited) || original.startsWith(edited)) return true;
        if (edited.length() > 34) return false;
        for (int prefixLength = 0; prefixLength <= Math.min(original.length(), edited.length()); prefixLength++) {
            if (original.startsWith(edited.substring(0, prefixLength))
                    && isPascalPromptText(edited.substring(prefixLength))) {
                return true;
            }
        }
        return false;
    }

    private void removeNewlyPlacedStat(int x, int y, boolean[] alreadyRemoved) {
        if (alreadyRemoved[0]) return;
        alreadyRemoved[0] = true;
        int result = ZztNative.editorRemoveStat(x, y);
        if (result == 0) {
            editorModified = true;
            updateDisplay(System.nanoTime() / 1_000_000L);
        } else {
            toastError("Undo placement", result);
        }
    }

    private static int directionIndex(int stepX, int stepY) {
        if (stepY == -1) return 0;
        if (stepY == 1) return 1;
        if (stepX == -1) return 2;
        return 3;
    }

    private static int clamp(int value, int minimum, int maximum) {
        return Math.max(minimum, Math.min(maximum, value));
    }

    private void showEditorPalette(int category, String title) {
        String[] packed = ZztNative.editorElements(category);
        if (packed == null || packed.length == 0) return;
        String[] names = new String[packed.length];
        int[] ids = new int[packed.length];
        for (int i = 0; i < packed.length; i++) {
            String[] parts = packed[i].split("\\|", 2);
            ids[i] = Integer.parseInt(parts[0]);
            names[i] = parts.length > 1 ? parts[1] : parts[0];
        }
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(title)
                .setItems(names, (selectionDialog, which) -> {
                    selectEditorPaletteElement(ids[which], names[which]);
                })
                .create();
        dialog.setOnKeyListener((ignored, keyCode, event) -> {
            if (event.getAction() != KeyEvent.ACTION_DOWN || event.getRepeatCount() != 0) return false;
            int shortcut = event.getUnicodeChar();
            if (shortcut == 0) return false;
            int element = ZztNative.editorElementByShortcut(category, shortcut);
            if (element < 0) return false;
            String name = Integer.toString(element);
            for (int i = 0; i < ids.length; i++) {
                if (ids[i] == element) {
                    name = names[i];
                    break;
                }
            }
            selectEditorPaletteElement(element, name);
            dialog.dismiss();
            placeEditorElementAt(editorCellX, editorCellY, true, false);
            restoreEditorPatternSelection();
            return true;
        });
        dialog.show();
    }

    private void selectEditorPaletteElement(int element, String name) {
        editorCopyMode = false;
        editorPasteMode = false;
        editorFloodFillMode = false;
        editorFloodFillUsesClipboard = false;
        editorTextMode = false;
        editorClipboardPattern = false;
        editorElement = element;
        editorElementName = name;
        refreshStatus();
    }

    private void restoreEditorPatternSelection() {
        editorClipboardPattern = editorPatternIndex == EDITOR_PATTERN_NAMES.length - 1;
        editorElementName = EDITOR_PATTERN_NAMES[editorPatternIndex];
        if (!editorClipboardPattern) editorElement = EDITOR_PATTERN_ELEMENTS[editorPatternIndex];
        refreshStatus();
    }

    private void showEditorPatterns() {
        new AlertDialog.Builder(this)
                .setTitle("Wall pattern")
                .setItems(EDITOR_PATTERN_NAMES, (dialog, which) -> {
                    editorCopyMode = false;
                    editorPasteMode = false;
                    editorFloodFillMode = false;
                    editorFloodFillUsesClipboard = false;
                    editorTextMode = false;
                    editorPatternIndex = which;
                    editorClipboardPattern = which == EDITOR_PATTERN_NAMES.length - 1;
                    if (!editorClipboardPattern) editorElement = EDITOR_PATTERN_ELEMENTS[which];
                    editorElementName = EDITOR_PATTERN_NAMES[which];
                    refreshStatus();
                })
                .show();
    }

    private void cycleEditorPattern() {
        editorPatternIndex = (editorPatternIndex + 1) % EDITOR_PATTERN_NAMES.length;
        if (editorPatternIndex == EDITOR_PATTERN_NAMES.length - 1) {
            editorCopyMode = false;
            editorPasteMode = false;
            editorFloodFillMode = false;
            editorFloodFillUsesClipboard = false;
            editorTextMode = false;
            editorClipboardPattern = true;
            editorElementName = EDITOR_PATTERN_NAMES[editorPatternIndex];
            refreshStatus();
        } else {
            selectEditorPaletteElement(EDITOR_PATTERN_ELEMENTS[editorPatternIndex],
                    EDITOR_PATTERN_NAMES[editorPatternIndex]);
        }
    }

    private void cycleEditorColor() {
        int foreground = editorColor & 0x0f;
        editorColor = (editorColor & 0xf0) | (foreground == 0x0f ? 9 : foreground + 1);
        refreshStatus();
    }

    private void showEditorColors() {
        String[] names = {"Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White"};
        new AlertDialog.Builder(this)
                .setTitle("Pattern color")
                .setSingleChoiceItems(names, editorColor - 9, (dialog, which) -> {
                    editorColor = which + 9;
                    dialog.dismiss();
                    refreshStatus();
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    private void showEditorMore() {
        String copyLabel = editorCopyMode ? "Cancel copy mode" : "Copy tile from board";
        String pasteLabel = editorPasteMode ? "Cancel paste mode" : "Paste copied tile";
        String fillSelectedLabel = editorFloodFillMode && !editorFloodFillUsesClipboard
                ? "Cancel selected-element fill" : "Fill with selected element";
        String fillClipboardLabel = editorFloodFillMode && editorFloodFillUsesClipboard
                ? "Cancel copied-tile fill" : "Fill with copied tile/stat";
        String textLabel = editorTextMode ? "Stop text entry" : "Text entry mode";
        String drawLabel = editorDrawMode ? "Stop draw mode" : "Draw mode: paint by dragging";
        String[] choices = {copyLabel, pasteLabel, "Color", "Edit selected stat",
                fillSelectedLabel, fillClipboardLabel, textLabel, drawLabel,
                "Editor help", "Edit help file"};
        new AlertDialog.Builder(this)
                .setTitle("Editor tools")
                .setItems(choices, (dialog, which) -> {
                    if (which == 0) {
                        editorCopyMode = !editorCopyMode;
                        if (editorCopyMode) {
                            editorPasteMode = false;
                            editorFloodFillMode = false;
                            editorFloodFillUsesClipboard = false;
                            editorTextMode = false;
                            editorDrawMode = false;
                            boardView.setContinuousCellMode(false);
                        }
                        Toast.makeText(this, editorCopyMode
                                ? "Tap a source tile to copy." : "Copy mode canceled.", Toast.LENGTH_SHORT).show();
                        refreshStatus();
                    } else if (which == 1) {
                        editorPasteMode = !editorPasteMode;
                        if (editorPasteMode) {
                            editorCopyMode = false;
                            editorFloodFillMode = false;
                            editorFloodFillUsesClipboard = false;
                            editorTextMode = false;
                            editorDrawMode = false;
                            boardView.setContinuousCellMode(false);
                        }
                        Toast.makeText(this, editorPasteMode
                                ? "Tap a destination tile to paste." : "Paste mode canceled.", Toast.LENGTH_SHORT).show();
                        refreshStatus();
                    } else if (which == 2) {
                        showEditorColors();
                    } else if (which == 3) {
                        editSelectedStat();
                    } else if (which == 4 || which == 5) {
                        toggleEditorFloodFill(which == 5);
                    } else if (which == 6) {
                        if (editorTextMode) {
                            editorTextMode = false;
                            refreshStatus();
                        } else {
                            showEditorTextCharacterDialog();
                        }
                    } else if (which == 7) {
                        toggleEditorDrawMode();
                    } else if (which == 8) {
                        showEditorHelp();
                    } else if (which == 9) {
                        editHelpFilePrompt();
                    }
                })
                .show();
    }

    private void toggleEditorFloodFill(boolean useClipboard) {
        if (useClipboard && !editorClipboardValid) {
            Toast.makeText(this, "Copy a tile before filling with the clipboard.",
                    Toast.LENGTH_LONG).show();
            return;
        }
        boolean sameMode = editorFloodFillMode && editorFloodFillUsesClipboard == useClipboard;
        editorFloodFillMode = !sameMode;
        editorFloodFillUsesClipboard = editorFloodFillMode && useClipboard;
        if (editorFloodFillMode) {
            editorCopyMode = false;
            editorPasteMode = false;
            editorTextMode = false;
            editorDrawMode = false;
        }
        boardView.setContinuousCellMode(editorDrawMode);
        Toast.makeText(this, editorFloodFillMode
                ? (useClipboard ? "Tap a connected area to fill from the copied tile."
                        : "Tap a connected area to fill from the selected element.")
                : "Flood fill canceled.", Toast.LENGTH_SHORT).show();
        refreshStatus();
    }

    private void showEditorTextCharacterDialog() {
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setFilters(new InputFilter[] {new InputFilter.LengthFilter(1)});
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        input.setHint("One ASCII character");
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Text tile character")
                .setMessage("Choose the character to place. The current color controls its text color.")
                .setView(input)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Use character", null)
                .create();
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String value = input.getText().toString();
                    if (value.length() != 1 || value.charAt(0) < 32 || value.charAt(0) >= 128) {
                        Toast.makeText(this, "Enter one printable ASCII character.", Toast.LENGTH_LONG).show();
                        return;
                    }
                    editorTextCharacter = value.charAt(0);
                    editorFloodFillMode = false;
                    editorFloodFillUsesClipboard = false;
                    editorCopyMode = false;
                    editorPasteMode = false;
                    editorTextMode = true;
                    editorDrawMode = false;
                    boardView.setContinuousCellMode(false);
                    refreshStatus();
                    dialog.dismiss();
                }));
        dialog.show();
    }

    private void showEditorBoards() {
        int[] status = ZztNative.status();
        if (status == null || status.length <= 7) return;
        int boardCount = Math.max(0, status[7]);
        String[] choices = new String[boardCount + 5];
        for (int boardId = 0; boardId <= boardCount; boardId++) {
            byte[] rawName = ZztNative.editorBoardNameBytes(boardId);
            String name = rawName == null ? "" : new String(rawName, CP437);
            if (name.isEmpty()) name = boardId == 0 ? "Title screen" : "Board " + boardId;
            choices[boardId] = boardId == 0 ? (name + " (0)") : (boardId + ": " + name);
        }
        choices[boardCount + 1] = "Edit board information...";
        choices[boardCount + 2] = "Add new board...";
        choices[boardCount + 3] = "Transfer board (.BRD)...";
        choices[boardCount + 4] = "Clear current board...";
        new AlertDialog.Builder(this)
                .setTitle("Switch boards")
                .setItems(choices, (dialog, which) -> {
                    if (which == boardCount + 1) {
                        showEditorBoardInfoDialog();
                        return;
                    }
                    if (which == boardCount + 2) {
                        showNewEditorBoardDialog();
                        return;
                    }
                    if (which == boardCount + 3) {
                        showEditorBoardTransferDialog();
                        return;
                    }
                    if (which == boardCount + 4) {
                        confirmClearEditorBoard();
                        return;
                    }
                    int result = ZztNative.editorSwitchBoard(which);
                    if (result != 0) {
                        toastError("Switch board", result);
                        return;
                    }
                    updateDisplay(System.nanoTime() / 1_000_000L);
                })
                .show();
    }

    private void confirmClearEditorBoard() {
        new AlertDialog.Builder(this)
                .setTitle("Clear current board?")
                .setMessage("This removes the board contents and resets its settings. The player returns to the center.")
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Clear board", (dialog, which) -> {
                    int result = ZztNative.editorClearBoard();
                    if (result != 0) {
                        toastError("Clear board", result);
                        return;
                    }
                    // Pascal's Z command calls BoardCreate without setting wasModified.
                    editorFloodFillMode = false;
                    editorFloodFillUsesClipboard = false;
                    editorCopyMode = false;
                    editorPasteMode = false;
                    updateDisplay(System.nanoTime() / 1_000_000L);
                    refreshStatus();
                })
                .show();
    }

    private void showEditorBoardTransferDialog() {
        new AlertDialog.Builder(this)
                .setTitle("Transfer board")
                .setItems(new String[] {"Import .BRD", "Export .BRD"}, (dialog, which) -> {
                    if (which == 0) pickEditorBoardImport();
                    else exportEditorBoard();
                })
                .show();
    }

    private void pickEditorBoardImport() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        try {
            startExternalActivity(intent, OPEN_BOARD);
        } catch (Exception e) {
            Toast.makeText(this, "No document picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private void importEditorBoard(Uri uri) {
        String name = safeDocumentName(queryName(uri));
        if (!name.toLowerCase(Locale.ROOT).endsWith(".brd")) {
            Toast.makeText(this, "Choose a .BRD board file.", Toast.LENGTH_LONG).show();
            return;
        }
        byte[] boardRecord;
        boolean blockReadStarted = false;
        InputStream input = null;
        try {
            input = getContentResolver().openInputStream(uri);
            if (input == null) throw new IOException("Could not read the selected board file.");
            // Pascal BoardTransfer closes and frees the current board before BlockRead.
            // A short read then sets the board length to zero and calls BoardCreate.
            blockReadStarted = true;
            int lengthLow = input.read();
            int lengthHigh = input.read();
            if (lengthLow < 0 || lengthHigh < 0) throw new IOException("The .BRD length header is incomplete.");
            int boardLength = lengthLow | (lengthHigh << 8);
            if (boardLength > MAX_BRD_RECORD_LENGTH) {
                throw new IOException("The .BRD record length is invalid.");
            }
            boardRecord = new byte[boardLength + 2];
            boardRecord[0] = (byte) lengthLow;
            boardRecord[1] = (byte) lengthHigh;
            int offset = 0;
            while (offset < boardLength) {
                int count = input.read(boardRecord, offset + 2, boardLength - offset);
                if (count < 0) throw new IOException("The .BRD record is truncated.");
                if (count == 0) {
                    int value = input.read();
                    if (value < 0) throw new IOException("The .BRD record is truncated.");
                    boardRecord[offset++ + 2] = (byte) value;
                } else {
                    offset += count;
                }
            }
        } catch (Exception e) {
            if (blockReadStarted) {
                int clearResult = ZztNative.editorClearBoard();
                if (clearResult != 0) {
                    toastError("Clear board after failed import", clearResult);
                } else {
                    // BoardCreate and its editor refresh do not set wasModified.
                    editorFloodFillMode = false;
                    editorFloodFillUsesClipboard = false;
                    editorCopyMode = false;
                    editorPasteMode = false;
                    updateDisplay(System.nanoTime() / 1_000_000L);
                    refreshStatus();
                }
            }
            Toast.makeText(this, "Board import failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
            return;
        } finally {
            if (input != null) {
                try {
                    input.close();
                } catch (Exception ignored) {
                    // Pascal's import path never closes the input file or checks a close error.
                }
            }
        }
        int result = ZztNative.editorImportBoard(boardRecord);
        if (result != 0) {
            toastError("Import board", result);
            return;
        }
        // EditorTransferBoard imports a record without setting wasModified.
        updateDisplay(System.nanoTime() / 1_000_000L);
        Toast.makeText(this, "Board imported. Neighbor links were cleared.", Toast.LENGTH_SHORT).show();
    }

    private void exportEditorBoard() {
        byte[] board = ZztNative.editorExportBoard();
        if (board == null) {
            Toast.makeText(this, "This board could not be serialized as a .BRD file.", Toast.LENGTH_LONG).show();
            return;
        }
        String base = safeBaseName(ZztNative.boardName());
        if (base.isEmpty()) base = "BOARD";
        pendingBoardExport = board;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE, base + ".BRD");
        try {
            startExternalActivity(intent, EXPORT_BOARD);
        } catch (Exception e) {
            pendingBoardExport = null;
            Toast.makeText(this, "No export picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private void showEditorBoardInfoDialog() {
        int[] info = ZztNative.editorBoardInfo();
        int[] status = ZztNative.status();
        if (info == null || info.length < 8 || status == null || status.length <= 7) return;
        int boardCount = Math.max(0, status[7]);
        LinearLayout fields = new LinearLayout(this);
        fields.setOrientation(LinearLayout.VERTICAL);
        fields.setPadding(dp(14), dp(4), dp(14), dp(8));

        String originalTitle = ZztNative.boardName();
        TextView titleLabel = label("Board title", 14, Color.WHITE);
        fields.addView(titleLabel);
        EditText titleInput = new EditText(this);
        titleInput.setSingleLine(true);
        titleInput.setText(originalTitle);
        titleInput.setFilters(new InputFilter[] {
                new InputFilter.LengthFilter(50), pascalPromptCharacterFilter()
        });
        boolean[] nonTimeFieldVisited = {false};
        trackEditorFieldInteraction(titleInput, nonTimeFieldVisited);
        titleInput.addTextChangedListener(new android.text.TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {
                nonTimeFieldVisited[0] = true;
            }
            @Override public void afterTextChanged(android.text.Editable s) { }
        });
        fields.addView(titleInput, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(52)));

        boolean[] maxShotsTouched = {false};
        NumberPicker maxShots = addEditorNumberField(fields, "Can fire: shots", 0, 255,
                info[0], maxShotsTouched);
        trackEditorFieldInteraction(maxShots, nonTimeFieldVisited);
        CheckBox dark = new CheckBox(this);
        dark.setText("Board is dark");
        dark.setTextColor(Color.WHITE);
        dark.setChecked(info[1] != 0);
        trackEditorFieldInteraction(dark, nonTimeFieldVisited);
        fields.addView(dark);

        String[] boardChoices = new String[boardCount + 1];
        boardChoices[0] = "None (0)";
        for (int boardId = 1; boardId <= boardCount; boardId++) {
            byte[] rawName = ZztNative.editorBoardNameBytes(boardId);
            String name = rawName == null ? "" : new String(rawName, CP437);
            boardChoices[boardId] = name.isEmpty() ? ("Board " + boardId) : (boardId + ": " + name);
        }
        Spinner[] neighborPickers = new Spinner[4];
        int[] shownNeighbor = new int[4];
        String[] neighborTitles = {"North neighbor", "South neighbor", "West neighbor", "East neighbor"};
        for (int i = 0; i < neighborPickers.length; i++) {
            shownNeighbor[i] = clamp(info[3 + i], 0, boardCount);
            neighborPickers[i] = addEditorChoiceField(fields, neighborTitles[i], boardChoices,
                    shownNeighbor[i]);
            trackEditorFieldInteraction(neighborPickers[i], nonTimeFieldVisited);
        }

        CheckBox reenter = new CheckBox(this);
        reenter.setText("Re-enter when zapped");
        reenter.setTextColor(Color.WHITE);
        reenter.setChecked(info[2] != 0);
        trackEditorFieldInteraction(reenter, nonTimeFieldVisited);
        fields.addView(reenter);

        boolean[] timeTouched = {false};
        NumberPicker timeLimit = addEditorNumberField(fields, "Time limit in seconds (0 = none)",
                0, 32767, clamp(info[7], 0, 32767), timeTouched);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(false);
        scroll.addView(fields);
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Board information")
                .setView(scroll)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Apply", null)
                .create();
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String enteredTitle = titleInput.getText().toString();
                    byte[] titleBytes = encodeCp437(enteredTitle);
                    if (titleBytes == null) {
                        Toast.makeText(this, "Board titles can only contain characters in IBM PC code page 437.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    if (!matchesOriginalPromptEdit(originalTitle, enteredTitle)) {
                        Toast.makeText(this, "This title does not fit the original 34-character prompt.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    if (titleBytes.length > 50) {
                        Toast.makeText(this, "Board titles can contain at most 50 CP437 bytes.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    int[] neighbors = new int[4];
                    for (int i = 0; i < neighbors.length; i++) {
                        neighbors[i] = neighborPickers[i].getSelectedItemPosition() == shownNeighbor[i]
                                ? info[3 + i] : neighborPickers[i].getSelectedItemPosition();
                    }
                    int seconds = timeTouched[0] ? timeLimit.getValue() : info[7];
                    boolean changesTrackedByPascal = nonTimeFieldVisited[0] || maxShotsTouched[0]
                            || !enteredTitle.equals(originalTitle)
                            || maxShots.getValue() != info[0]
                            || (dark.isChecked() ? 1 : 0) != info[1]
                            || (reenter.isChecked() ? 1 : 0) != info[2];
                    for (int i = 0; i < neighbors.length; i++) {
                        changesTrackedByPascal |= neighbors[i] != info[3 + i];
                    }
                    int result = ZztNative.editorUpdateBoardInfo(titleBytes, maxShots.getValue(),
                            dark.isChecked() ? 1 : 0, reenter.isChecked() ? 1 : 0,
                            neighbors[0], neighbors[1], neighbors[2], neighbors[3], seconds);
                    if (result != 0) {
                        toastError("Save board information", result);
                        return;
                    }
                    // Pascal's Board Information handler omits its time-limit row (9)
                    // when setting wasModified, so time-only edits do not prompt to save.
                    if (changesTrackedByPascal) editorModified = true;
                    dialog.dismiss();
                    updateDisplay(System.nanoTime() / 1_000_000L);
                }));
        dialog.show();
    }

    private void showNewEditorBoardDialog() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(dp(20), dp(8), dp(20), dp(4));
        TextView title = label("Room's title (1 to 34 printable ASCII characters)", 14, Color.WHITE);
        content.addView(title);
        EditText nameInput = new EditText(this);
        nameInput.setSingleLine(true);
        nameInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_SENTENCES);
        nameInput.setHint("Room title");
        nameInput.setFilters(new InputFilter[] {
                new InputFilter.LengthFilter(34), pascalPromptCharacterFilter()
        });
        content.addView(nameInput, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, dp(54)));
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Add new board")
                .setView(content)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Create", null)
                .create();
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    byte[] encodedName = encodeCp437(nameInput.getText().toString());
                    if (encodedName == null) {
                        Toast.makeText(this, "Board titles can only contain characters in IBM PC code page 437.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    if (encodedName.length < 1 || encodedName.length > 34
                            || !isPascalPromptText(nameInput.getText().toString())) {
                        Toast.makeText(this, "Enter a board title from 1 to 34 printable ASCII characters.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    int result = ZztNative.editorAppendBoard(encodedName);
                    if (result < 0) {
                        toastError("Add board", result);
                        return;
                    }
                    // EditorAppendBoard itself leaves the Pascal dirty flag unchanged.
                    dialog.dismiss();
                    updateDisplay(System.nanoTime() / 1_000_000L);
                }));
        dialog.show();
    }

    private boolean saveWorldToApp() {
        if (activeFilename == null || activeFilename.isEmpty()) return false;
        String lowerFilename = activeFilename.toLowerCase(Locale.ROOT);
        if (!lowerFilename.endsWith(".zzt")) {
            Toast.makeText(this, "The editor can only save .ZZT world files.", Toast.LENGTH_LONG).show();
            return false;
        }
        String worldName = activeFilename.substring(0, activeFilename.length() - 4);
        byte[] worldNameBytes = encodeCp437(worldName);
        if (worldNameBytes == null) {
            Toast.makeText(this, "World filenames can only contain characters in IBM PC code page 437.",
                    Toast.LENGTH_LONG).show();
            return false;
        }
        int result = ZztNative.setWorldName(worldNameBytes);
        if (result != 0) {
            toastError("Set world name", result);
            return false;
        }
        result = ZztNative.saveWorld(activeFilename);
        if (result != 0) {
            toastError("Save world", result);
            return false;
        }
        result = ZztNative.installWorldAlias(activeFilename);
        editorModified = false;
        if (result != 0) {
            Toast.makeText(this,
                    "World saved, but its original-name reload file could not be prepared.",
                    Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(this, "World saved inside the app.", Toast.LENGTH_SHORT).show();
        }
        return true;
    }

    private void saveAndExportWorld() {
        if (!saveWorldToApp()) return;
        pendingExport = new File(activeDirectory, activeFilename);
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE, activeFilename);
        try {
            startExternalActivity(intent, EXPORT_WORLD);
        } catch (Exception e) {
            Toast.makeText(this, "World saved inside the app, but no export picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private boolean hasEditorChanges() {
        return editorModified || ZztNative.editorWasModified();
    }

    private void requestEditorExit() {
        if (!hasEditorChanges()) {
            finishEditor();
            return;
        }
        new AlertDialog.Builder(this)
                .setTitle("Save world changes?")
                .setMessage("The board has changed.")
                .setPositiveButton("Save", (dialog, which) -> {
                    if (saveWorldToApp()) finishEditor();
                })
                .setNegativeButton("Discard", (dialog, which) -> finishEditor())
                .setNeutralButton("Keep editing", null)
                .show();
    }

    private void finishEditor() {
        boardView.setCellTapListener(null);
        boardView.setContinuousCellMode(false);
        editorActive = false;
        editorDrawMode = false;
        editorShiftHeld = false;
        editorShiftPainted = false;
        editorClipboardValid = false;
        editorFloodFillUsesClipboard = false;
        ZztNative.editorEnd();
        editorTools.setVisibility(View.GONE);
        controls.setVisibility(View.VISIBLE);
        tools.setVisibility(View.VISIBLE);
        refreshStatus();
    }

    @Override public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length
                && handledGameplayKeyUps[keyCode]) {
            if (event.getRepeatCount() != 0) {
                if (editorActive) handleEditorMovementKey(keyCode, event);
                return true;
            }
            handledGameplayKeyUps[keyCode] = false;
        }
        if (keyCode >= 0 && keyCode < gameplayNumberKeyUps.length
                && gameplayNumberKeyUps[keyCode] != 0) return true;
        if (event.getRepeatCount() == 0 && handleEditorShortcut(keyCode, event)) {
            if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length) {
                handledGameplayKeyUps[keyCode] = true;
            }
            return true;
        }
        if (event.getRepeatCount() == 0 && handleTitleDebugShortcut(event)) {
            if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length) {
                handledGameplayKeyUps[keyCode] = true;
            }
            return true;
        }
        if (event.getRepeatCount() == 0 && handleTitleShortcut(keyCode)) {
            if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length) {
                handledGameplayKeyUps[keyCode] = true;
            }
            return true;
        }
        if (handleGameplayShortcut(keyCode, event)) return true;
        if (ZztNative.gameCommandsAvailable()) {
            int direction = numericDirectionForKey(keyCode, event);
            if (direction != 0) {
                if (keyCode >= 0 && keyCode < gameplayNumberKeyUps.length) {
                    gameplayNumberKeyUps[keyCode] = direction;
                }
                setNativeKey(direction, true);
                return true;
            }
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP: setNativeKey(KEY_UP, true); return true;
            case KeyEvent.KEYCODE_DPAD_LEFT: setNativeKey(KEY_LEFT, true); return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT: setNativeKey(KEY_RIGHT, true); return true;
            case KeyEvent.KEYCODE_DPAD_DOWN: setNativeKey(KEY_DOWN, true); return true;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER: setNativeKey(KEY_ENTER, true); return true;
            case KeyEvent.KEYCODE_DEL: setNativeKey(8, true); return true;
            case KeyEvent.KEYCODE_TAB: setNativeKey(9, true); return true;
            case KeyEvent.KEYCODE_T: setNativeKey('t', true); return true;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT: setNativeAction(ACTION_SHOOT, true); return true;
            case KeyEvent.KEYCODE_F1:
            case KeyEvent.KEYCODE_MENU:
                showMenu();
                return true;
            case KeyEvent.KEYCODE_ESCAPE: handleEscapeAction(); return true;
            default:
                int characterKey = cp437TextKey(event.getUnicodeChar());
                if (characterKey != 0) {
                    setNativeKey(characterKey, true);
                    return true;
                }
                return super.onKeyDown(keyCode, event);
        }
    }

    @Override public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (editorActive && (keyCode == KeyEvent.KEYCODE_SHIFT_LEFT
                || keyCode == KeyEvent.KEYCODE_SHIFT_RIGHT)) {
            editorShiftHeld = false;
            if (!editorShiftPainted) plotEditorCursor();
            editorShiftPainted = false;
            return true;
        }
        if (keyCode >= 0 && keyCode < handledGameplayKeyUps.length
                && handledGameplayKeyUps[keyCode]) {
            handledGameplayKeyUps[keyCode] = false;
            return true;
        }
        if (keyCode >= 0 && keyCode < gameplayNumberKeyUps.length
                && gameplayNumberKeyUps[keyCode] != 0) {
            int direction = gameplayNumberKeyUps[keyCode];
            gameplayNumberKeyUps[keyCode] = 0;
            setNativeKey(direction, false);
            return true;
        }
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_UP: setNativeKey(KEY_UP, false); return true;
            case KeyEvent.KEYCODE_DPAD_LEFT: setNativeKey(KEY_LEFT, false); return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT: setNativeKey(KEY_RIGHT, false); return true;
            case KeyEvent.KEYCODE_DPAD_DOWN: setNativeKey(KEY_DOWN, false); return true;
            case KeyEvent.KEYCODE_ENTER:
            case KeyEvent.KEYCODE_NUMPAD_ENTER: setNativeKey(KEY_ENTER, false); return true;
            case KeyEvent.KEYCODE_DEL: setNativeKey(8, false); return true;
            case KeyEvent.KEYCODE_TAB: setNativeKey(9, false); return true;
            case KeyEvent.KEYCODE_T: setNativeKey('t', false); return true;
            case KeyEvent.KEYCODE_SHIFT_LEFT:
            case KeyEvent.KEYCODE_SHIFT_RIGHT: setNativeAction(ACTION_SHOOT, false); return true;
            case KeyEvent.KEYCODE_ESCAPE: return true;
            default:
                int characterKey = cp437TextKey(event.getUnicodeChar());
                if (characterKey != 0) {
                    setNativeKey(characterKey, false);
                    return true;
                }
                return super.onKeyUp(keyCode, event);
        }
    }

    private static int cp437TextKey(int codePoint) {
        return DosKeyboardMapping.fromUnicode(codePoint);
    }

    private int numericDirectionForKey(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_NUMPAD_8: return KEY_UP;
            case KeyEvent.KEYCODE_NUMPAD_4: return KEY_LEFT;
            case KeyEvent.KEYCODE_NUMPAD_6: return KEY_RIGHT;
            case KeyEvent.KEYCODE_NUMPAD_2: return KEY_DOWN;
        }
        switch (event.getUnicodeChar()) {
            case '8': return KEY_UP;
            case '4': return KEY_LEFT;
            case '6': return KEY_RIGHT;
            case '2': return KEY_DOWN;
            default: return 0;
        }
    }

    private void pickWorld(boolean saveOnly) {
        pickerSaveOnly = saveOnly;
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        try {
            startExternalActivity(intent, OPEN_WORLD);
        } catch (Exception e) {
            Toast.makeText(this, "No document picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private void saveAndExport() {
        String base = safeBaseName(ZztNative.worldName());
        if (base.isEmpty()) base = "ZZT";
        String filename = base + ".SAV";
        int result = ZztNative.saveWorld(filename);
        if (result != 0) {
            toastError("Save game", result);
            return;
        }
        pendingExport = new File(activeDirectory, filename);
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/octet-stream");
        intent.putExtra(Intent.EXTRA_TITLE, filename);
        try {
            startExternalActivity(intent, EXPORT_SAVE);
        } catch (Exception e) {
            Toast.makeText(this, "Saved inside the app, but no export picker is available.", Toast.LENGTH_LONG).show();
        }
    }

    private void typeText() {
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setImeOptions(EditorInfo.IME_ACTION_DONE);
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle("Type into ZZT")
                .setView(input)
                .setPositiveButton("Send", (d, w) -> sendText(input.getText().toString()))
                .setNegativeButton("Cancel", null)
                .create();
        input.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE || (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                sendText(input.getText().toString());
                dialog.dismiss();
                return true;
            }
            return false;
        });
        dialog.setOnShowListener(d -> {
            input.requestFocus();
            input.postDelayed(() -> {
                InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                if (imm != null) imm.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
            }, 120);
        });
        showBlockedDialog(dialog);
    }

    private void sendText(String text) {
        byte[] encoded = encodeCp437(text);
        if (encoded == null) {
            Toast.makeText(this, "ZZT text can only contain characters in IBM PC code page 437.",
                    Toast.LENGTH_LONG).show();
            return;
        }
        Handler handler = new Handler(Looper.getMainLooper());
        long delay = 0;
        for (byte value : encoded) {
            int c = value & 0xff;
            if (c >= 32) {
                final int key = DosKeyboardMapping.keyForByte(c);
                handler.postDelayed(() -> ZztNative.keyStroke(key), delay);
                delay += 45;
            }
        }
        handler.postDelayed(() -> ZztNative.keyStroke(KEY_ENTER), delay + 45);
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        releaseExternalActivityBlock(requestCode);
        if (requestCode == OPEN_BOARD) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null && editorActive) {
                importEditorBoard(data.getData());
            }
            return;
        }
        if (requestCode == OPEN_WORLD) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null) {
                importFile(data.getData());
            }
            return;
        }
        if (requestCode == EXPORT_BOARD) {
            byte[] board = pendingBoardExport;
            pendingBoardExport = null;
            if (resultCode != RESULT_OK || data == null || data.getData() == null || board == null) return;
            try (OutputStream out = getContentResolver().openOutputStream(data.getData(), "wt")) {
                if (out == null) throw new IllegalStateException("Could not open the selected document.");
                out.write(board);
                out.flush();
                Toast.makeText(this, "Board exported.", Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "Board export failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
            return;
        }
        if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
        Uri uri = data.getData();
        if ((requestCode == EXPORT_SAVE || requestCode == EXPORT_WORLD || requestCode == EXPORT_HIGHSCORES) && pendingExport != null) {
            try (InputStream in = new FileInputStream(pendingExport);
                 OutputStream out = getContentResolver().openOutputStream(uri, "wt")) {
                if (out == null) throw new IllegalStateException("Could not open the selected document.");
                copy(in, out);
                String message = requestCode == EXPORT_WORLD ? "World exported."
                        : requestCode == EXPORT_HIGHSCORES ? "High scores exported." : "Save exported.";
                Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "Export failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
        }
    }

    private void importFile(Uri uri) {
        String displayName = safeDocumentName(queryName(uri));
        String lower = displayName.toLowerCase(Locale.ROOT);
        if (lower.endsWith(".hi")) {
            if (pickerSaveOnly) {
                Toast.makeText(this, "Choose a .HI file using Load world.", Toast.LENGTH_LONG).show();
                return;
            }
            try (InputStream in = getContentResolver().openInputStream(uri);
                 OutputStream out = new FileOutputStream(highScoreFile())) {
                if (in == null) throw new IllegalStateException("Could not read the selected document.");
                copy(in, out);
                highScores.loadInto(highScoreFile());
                Toast.makeText(this, "High scores imported.", Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "High score import failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
            }
            return;
        }
        if (!lower.endsWith(".zzt") && !lower.endsWith(".sav")) {
            Toast.makeText(this, "Choose a .ZZT world, .SAV game, or .HI high score file.", Toast.LENGTH_LONG).show();
            return;
        }
        if (pickerSaveOnly && !lower.endsWith(".sav")) {
            Toast.makeText(this, "Choose a .SAV game to restore.", Toast.LENGTH_LONG).show();
            return;
        }
        if (!pickerSaveOnly && lower.endsWith(".sav")) {
            Toast.makeText(this, "Choose a .ZZT world here; use Restore save for .SAV files.", Toast.LENGTH_LONG).show();
            return;
        }
        String stem = displayName.substring(0, displayName.lastIndexOf('.'));
        String folderName = safeBaseName(stem);
        if (folderName.isEmpty()) folderName = "world";
        File worlds = new File(getFilesDir(), "worlds");
        File directory = new File(worlds, folderName + "-" + System.currentTimeMillis());
        if (!directory.mkdirs()) {
            Toast.makeText(this, "Could not create an app storage folder.", Toast.LENGTH_LONG).show();
            return;
        }
        File imported = new File(directory, displayName);
        File previousDirectory = activeDirectory;
        String previousFilename = activeFilename;
        String editorCheckpointName = null;
        boolean previousEditorModified = hasEditorChanges();
        boolean editorLoadCommitted = false;
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream out = new FileOutputStream(imported)) {
            if (in == null) throw new IllegalStateException("Could not read the selected document.");
            copy(in, out);
            boolean loadingInEditor = editorActive && lower.endsWith(".zzt");
            if (loadingInEditor) {
                editorCheckpointName = ".editor-rollback-" + System.nanoTime() + ".ZZT";
                int checkpointResult = ZztNative.saveWorld(editorCheckpointName);
                if (checkpointResult != 0) {
                    new File(previousDirectory, editorCheckpointName).delete();
                    toastError("Preserve editor state", checkpointResult);
                    return;
                }
            }
            int result = ZztNative.setWorldDirectory(directory.getAbsolutePath());
            if (result == 0) {
                result = loadingInEditor
                        ? ZztNative.editorLoadWorldFile(imported.getName())
                        : ZztNative.loadFile(imported.getName());
            }
            if (result < 0 || (result > 0 && !(loadingInEditor && result == 1))) {
                restoreEditorLoadCheckpoint(editorCheckpointName, previousDirectory,
                        previousFilename, previousEditorModified);
                if (editorCheckpointName != null) {
                    new File(previousDirectory, editorCheckpointName).delete();
                }
                if (!loadingInEditor) {
                    ZztNative.setWorldDirectory(previousDirectory.getAbsolutePath());
                    if (previousFilename != null) ZztNative.loadFile(previousFilename);
                }
                toastError("Load " + displayName, result);
                refreshStatus();
                return;
            }
            if (loadingInEditor) {
                activeDirectory = directory;
                activeFilename = imported.getName();
                installInternalNameAlias(imported);
                highScores.loadInto(highScoreFile());
                String loadedMessage = "Loaded " + displayName + " in the editor.";
                if (result == 1) {
                    File resetDirectory = new File(worlds, "editor-" + System.currentTimeMillis());
                    if (!resetDirectory.mkdirs() && !resetDirectory.isDirectory()) {
                        restoreEditorLoadCheckpoint(editorCheckpointName, previousDirectory,
                                previousFilename, previousEditorModified);
                        if (editorCheckpointName != null) {
                            new File(previousDirectory, editorCheckpointName).delete();
                        }
                        Toast.makeText(this,
                                "Could not prepare a private folder for the fresh editor world.",
                                Toast.LENGTH_LONG).show();
                        return;
                    }
                    int directoryResult = ZztNative.setWorldDirectory(resetDirectory.getAbsolutePath());
                    int resetResult = directoryResult == 0 ? ZztNative.editorResetLoadedWorld() : directoryResult;
                    if (resetResult != 0) {
                        restoreEditorLoadCheckpoint(editorCheckpointName, previousDirectory,
                                previousFilename, previousEditorModified);
                        if (editorCheckpointName != null) {
                            new File(previousDirectory, editorCheckpointName).delete();
                        }
                        if (resetDirectory.list() != null && resetDirectory.list().length == 0) {
                            resetDirectory.delete();
                        }
                        toastError("Create fresh editor world", resetResult);
                        refreshStatus();
                        return;
                    }
                    activeDirectory = resetDirectory;
                    activeFilename = "world.ZZT";
                    loadedMessage = "Original ZZT started a blank editor world for this save or SECRET world.";
                }
                editorModified = false;
                if (editorCheckpointName != null) new File(previousDirectory, editorCheckpointName).delete();
                editorLoadCommitted = true;
                updateDisplay(System.nanoTime() / 1_000_000L);
                refreshStatus();
                Toast.makeText(this, loadedMessage, Toast.LENGTH_LONG).show();
                return;
            }
            File loadedDirectory = directory;
            String loadedFilename = imported.getName();
            String loadedMessage = "Loaded " + displayName;
            if (lower.endsWith(".zzt")) {
                installInternalNameAlias(imported);
            } else {
                byte[] sourceWorldName = ZztNative.worldNameBytes();
                File sourceWorldDirectory = findWorldDirectory(
                        worlds, directory, previousDirectory, sourceWorldName);
                if (sourceWorldDirectory != null) {
                    File attachedSave = null;
                    try {
                        attachedSave = File.createTempFile("sav-", ".SAV", sourceWorldDirectory);
                        try (InputStream saveInput = new FileInputStream(imported);
                             OutputStream saveOutput = new FileOutputStream(attachedSave)) {
                            copy(saveInput, saveOutput);
                        }
                        loadedDirectory = sourceWorldDirectory;
                        loadedFilename = attachedSave.getName();
                        loadedMessage = "Loaded save with its source world.";
                        if (imported.delete()) directory.delete();
                    } catch (Exception e) {
                        if (attachedSave != null) attachedSave.delete();
                        ZztNative.setWorldDirectory(directory.getAbsolutePath());
                        loadedMessage = "Loaded save, but could not attach its source world: " + e.getMessage();
                    }
                } else {
                    ZztNative.setWorldDirectory(directory.getAbsolutePath());
                    loadedMessage = "Loaded save. Load its original .ZZT world before restarting from the title.";
                }
            }
            activeDirectory = loadedDirectory;
            activeFilename = loadedFilename;
            highScores.loadInto(highScoreFile());
            refreshStatus();
            Toast.makeText(this, loadedMessage, Toast.LENGTH_LONG).show();
        } catch (Exception e) {
            if (editorCheckpointName != null && !editorLoadCommitted) {
                restoreEditorLoadCheckpoint(editorCheckpointName, previousDirectory,
                        previousFilename, previousEditorModified);
            }
            if (editorCheckpointName != null) new File(previousDirectory, editorCheckpointName).delete();
            Toast.makeText(this, "Import failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }

    private void restoreEditorLoadCheckpoint(String checkpointName, File previousDirectory,
                                             String previousFilename, boolean previousModified) {
        if (checkpointName == null || previousDirectory == null) return;
        if (ZztNative.setWorldDirectory(previousDirectory.getAbsolutePath()) == 0) {
            int result = ZztNative.editorLoadWorldFile(checkpointName);
            if (result == 1) result = ZztNative.editorResetLoadedWorld();
            if (result != 0) toastError("Restore editor state", result);
        }
        activeDirectory = previousDirectory;
        activeFilename = previousFilename;
        editorModified = previousModified;
        highScores.loadInto(highScoreFile());
        updateDisplay(System.nanoTime() / 1_000_000L);
        refreshStatus();
    }

    private File findWorldDirectory(File worlds, File importedDirectory, File preferredDirectory,
                                    byte[] cp437WorldName) {
        if (preferredDirectory != null && !preferredDirectory.equals(importedDirectory)
                && ZztNative.setWorldDirectory(preferredDirectory.getAbsolutePath()) == 0
                && ZztNative.worldFileExists(cp437WorldName)) {
            return preferredDirectory;
        }

        File[] candidates = worlds.listFiles();
        if (candidates == null) return null;
        for (File candidate : candidates) {
            if (!candidate.isDirectory() || candidate.equals(importedDirectory)
                    || candidate.equals(preferredDirectory)) continue;
            if (ZztNative.setWorldDirectory(candidate.getAbsolutePath()) == 0
                    && ZztNative.worldFileExists(cp437WorldName)) {
                return candidate;
            }
        }
        return null;
    }

    private void installInternalNameAlias(File source) {
        int result = ZztNative.installWorldAlias(source.getName());
        if (result != 0) {
            Toast.makeText(this,
                    "Loaded the world, but its original-name reload file could not be prepared.",
                    Toast.LENGTH_LONG).show();
        }
    }

    private String queryName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) return cursor.getString(column);
            }
        } catch (Exception ignored) {}
        String path = uri.getLastPathSegment();
        return path == null ? "world.ZZT" : path;
    }

    private String safeDocumentName(String displayName) {
        String name = displayName == null ? "world.ZZT" : displayName.replace('\\', '/');
        int separator = name.lastIndexOf('/');
        if (separator >= 0) name = name.substring(separator + 1);
        if (name.isEmpty() || name.equals(".") || name.equals("..")) return "world.ZZT";
        return name.replaceAll("[\\p{Cntrl}]", "_");
    }

    private String safeBaseName(String name) {
        if (name == null) return "";
        return name.trim().replaceAll("[^A-Za-z0-9 _-]", "_").replaceAll("[ .]+$", "");
    }

    private String safeWorldFileStem(String name) {
        if (name == null) return "";
        int nul = name.indexOf('\0');
        if (nul >= 0) name = name.substring(0, nul);
        StringBuilder stem = new StringBuilder(name.length());
        for (int i = 0; i < name.length(); i++) {
            char c = name.charAt(i);
            stem.append(c == '/' || c == '\\' || Character.isISOControl(c) ? '_' : c);
        }
        return stem.toString();
    }

    private void copyAsset(String assetName, File destination) throws Exception {
        try (InputStream in = getAssets().open(assetName); OutputStream out = new FileOutputStream(destination)) {
            copy(in, out);
        }
    }

    private void copy(InputStream in, OutputStream out) throws Exception {
        byte[] buffer = new byte[32768];
        int count;
        while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
        out.flush();
    }

    private void updateDisplay(long nowMillis) {
        if (boardView == null) return;
        int transitionType;
        while ((transitionType = ZztNative.takeTransition(
                transitionFrom, transitionTo, transitionOrder)) != 0) {
            boardView.enqueueTransition(transitionFrom, transitionTo, transitionOrder,
                    transitionType, nowMillis);
        }
        ZztNative.copyBoard(frameBytes);
        boardView.update(frameBytes, nowMillis);
        refreshStatus();
    }

    private void refreshStatus() {
        if (boardView == null) return;
        int[] s = ZztNative.status();
        if (s == null || s.length == 0) return;
        if (playTool != null) {
            int visibility = s[0] == GAME_STATE_TITLE ? View.VISIBLE : View.GONE;
            playTool.setVisibility(visibility);
        }
        if (saveTool != null) {
            int visibility = s[0] == GAME_STATE_PLAY ? View.VISIBLE : View.GONE;
            saveTool.setVisibility(visibility);
        }
        if (s.length < 19) return;
        String world = ZztNative.worldName();
        String board = ZztNative.boardName();
        if (worldTitle != null) worldTitle.setText(world == null || world.isEmpty() ? "ZZT" : world);
        String boardLabel = board == null || board.isEmpty() ? "Title screen" : board;
        if (boardTitle != null) boardTitle.setText(editorActive ? boardLabel + " — Editor" : boardLabel);
        if (editorActive) {
            String editorMode = editorCopyMode
                    ? "COPY MODE — tap a source tile"
                    : editorPasteMode
                    ? "PASTE MODE — tap a destination"
                    : editorFloodFillMode
                    ? (editorFloodFillUsesClipboard
                            ? "FILL MODE — copied tile/stat"
                            : "FILL MODE — selected element")
                    : editorTextMode
                    ? "TEXT MODE — tap to place '" + (char) editorTextCharacter + "'"
                    : editorDrawMode
                    ? "DRAW MODE — drag to place " + editorElementName
                    : "PLACE " + editorElementName + "   COLOR " + editorColor;
            String editorStatus = editorMode + "\nMore opens copy/paste, color, stat, fill, and text tools.";
            if (statusText != null && !editorStatus.equals(statusText.getText().toString())) statusText.setText(editorStatus);
            lastStatus = Arrays.copyOf(s, s.length);
            return;
        }
        String flags = (s[8] != 0 ? "   DARK" : "") + (s[14] != 0 ? "   PAUSED" : "");
        // Pascal computes this sidebar value as a signed 16-bit Integer.
        int timeRemaining = (short) (s[18] - s[16]);
        String time = s[18] > 0 ? String.format(Locale.ROOT, "   TIME %d", timeRemaining) : "";
        StringBuilder statusBuilder = new StringBuilder(String.format(Locale.ROOT,
                "HP %d   AMMO %d   GEMS %d   TORCH %d\nSCORE %d   BOARD %d/%d%s%s\nKEYS ",
                s[1], s[2], s[3], s[4], s[5], s[6] + 1, s[7] + 1, time, flags));
        int[] keyBits = {0, 1, 2, 3, 4, 5, 6};
        String[] keyNames = {"B", "G", "C", "R", "P", "Y", "W"};
        int[] keyColors = {
                0xff5555ff, 0xff55ff55, 0xff55ffff, 0xffff5555,
                0xffff55ff, 0xffffff55, 0xffffffff
        };
        int keyMask = s[11];
        int keyStart = statusBuilder.length();
        for (int i = 0; i < keyNames.length; i++) {
            if (i > 0) statusBuilder.append(' ');
            boolean held = (keyMask & (1 << keyBits[i])) != 0;
            statusBuilder.append(keyNames[i]).append(held ? '+' : '-');
        }
        statusBuilder.append(' ');
        boolean blackKeyHeld = (s[3] & 0xff00) != 0;
        statusBuilder.append("BK").append(blackKeyHeld ? '+' : '-');
        String status = statusBuilder.toString();
        if (statusText != null && !status.equals(statusText.getText().toString())) {
            SpannableStringBuilder styledStatus = new SpannableStringBuilder(status);
            int cursor = keyStart;
            for (int i = 0; i < keyNames.length; i++) {
                boolean held = (keyMask & (1 << keyBits[i])) != 0;
                styledStatus.setSpan(new ForegroundColorSpan(
                                held ? keyColors[i] : Color.rgb(120, 120, 120)),
                        cursor, cursor + 2, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
                cursor += 3;
            }
            styledStatus.setSpan(new ForegroundColorSpan(
                            blackKeyHeld ? Color.rgb(210, 210, 210) : Color.rgb(120, 120, 120)),
                    status.length() - 3, status.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
            statusText.setText(styledStatus);
        }
        lastStatus = Arrays.copyOf(s, s.length);
    }

    private void toastError(String operation, int result) {
        Toast.makeText(this, operation + " failed (engine " + result + ").", Toast.LENGTH_LONG).show();
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    @Override protected void onResume() {
        super.onResume();
        startAudio();
        if (!framePosted) {
            framePosted = true;
            Choreographer.getInstance().postFrameCallback(this);
        }
    }

    @Override protected void onPause() {
        releaseHeldInput();
        if (framePosted) {
            Choreographer.getInstance().removeFrameCallback(this);
            framePosted = false;
        }
        stopAudio();
        super.onPause();
    }

    private void startAudio() {
        if (audioThread != null) return;
        int sampleRate = 22050;
        int minBuffer = AudioTrack.getMinBufferSize(sampleRate,
                AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_8BIT);
        if (minBuffer <= 0) return;
        AudioTrack track = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_8BIT,
                Math.max(minBuffer, 4096), AudioTrack.MODE_STREAM);
        if (track.getState() != AudioTrack.STATE_INITIALIZED) {
            track.release();
            return;
        }
        audioTrack = track;
        audioRunning = true;
        track.play();
        audioThread = new Thread(() -> {
            byte[] samples = new byte[512];
            while (audioRunning) {
                try {
                    ZztNative.renderAudio(samples);
                    int written = track.write(samples, 0, samples.length);
                    if (written < 0) break;
                } catch (RuntimeException e) {
                    break;
                }
            }
        }, "ZZT PC speaker");
        audioThread.start();
    }

    private void stopAudio() {
        audioRunning = false;
        Thread thread = audioThread;
        AudioTrack track = audioTrack;
        audioThread = null;
        audioTrack = null;
        if (track != null) {
            try {
                track.stop();
            } catch (IllegalStateException ignored) {
            }
        }
        if (thread != null) {
            thread.interrupt();
            try {
                thread.join(250);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        if (track != null) track.release();
    }

    @Override public void onBackPressed() {
        showMenu();
    }

    @Override public void doFrame(long frameTimeNanos) {
        if (!framePosted) return;
        long nowMillis = frameTimeNanos / 1_000_000L;
        ZztNative.frame(frameTimeNanos);
        updateDisplay(nowMillis);
        Choreographer.getInstance().postFrameCallback(this);
    }
}
