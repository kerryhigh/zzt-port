package com.kerry.zzt;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Set;

/** Exercises the Android controls with two independent touchscreen pointers. */
public final class TouchControlsInstrumentation extends Instrumentation {
    private static final String TAG = "ZZTTouchCheck";
    private static final int GAME_STATE_PLAY = 2;
    private static final int KEY_UP = 200;
    private static final int KEY_RIGHT = 205;
    private static final int ACTION_RIGHT = 2;
    private static final int ACTION_SHOOT = 6;
    private static final int STATUS_LAST_ERROR_FIELD = 17;
    private static final int ELEMENT_EMPTY = 0;
    private static final int ELEMENT_PLAYER = 4;
    private static final int ELEMENT_SOLID = 21;
    private static final int CHARACTER_PLAYER = 2;
    private static final int CHARACTER_SOLID = 219;
    private Bundle instrumentationArguments;

    @Override public void onCreate(Bundle arguments) {
        super.onCreate(arguments);
        instrumentationArguments = arguments;
        start();
    }

    @Override public void onStart() {
        Bundle result = new Bundle();
        try {
            if (instrumentationArguments != null
                    && "true".equals(instrumentationArguments.getString("frameLifecycleOnly"))) {
                verifyChoreographerFrameLifecycle(launchMainActivity());
                result.putString("result", "PASS: Choreographer frame delivery through JNI and pause/resume lifecycle");
            } else if (instrumentationArguments != null
                    && "true".equals(instrumentationArguments.getString("fileFlowOnly"))) {
                verifyImportSaveRestoreAndExport(launchMainActivity());
                result.putString("result", "PASS: successful ZZT, SAV, HI, and BRD import/export file flows");
            } else {
                verifySeparateFingerMoveAndFire();
                result.putString("result", "PASS: monochrome attributes, touch controls, D-pad/numpad gameplay input, bounded/queue-wrap/dense/full-board editor fills, pause input release, title/gameplay Play and Save visibility, failed world open, successful ZZT import, SAV save/export/restore, and HI/BRD import/export");
            }
            Log.i(TAG, result.getString("result"));
            finish(Activity.RESULT_OK, result);
        } catch (Throwable failure) {
            result.putString("result", "FAIL: " + failure);
            Log.e(TAG, result.getString("result"), failure);
            finish(Activity.RESULT_CANCELED, result);
        }
    }

    private void verifySeparateFingerMoveAndFire() throws Exception {
        Activity activity = launchMainActivity();
        Log.i(TAG, "Attached to " + activity.getComponentName());

        verifyPascalMonochromeTextAttribute();

        final boolean[] playFound = {false};
        final boolean[] playClicked = {false};
        runOnMain(() -> {
            ZztNative.returnToTitle();
            require(ZztNative.editorBegin() >= 0,
                    "Could not open the title board to prepare keyboard movement coverage");
            require(ZztNative.editorClearBoard() == 0,
                    "Could not clear the board for predictable keyboard movement");
            require(ZztNative.editorPlace(30, 13, ELEMENT_PLAYER, 15, false) == 0
                            && ZztNative.editorPlace(31, 13, ELEMENT_EMPTY, 15, false) == 0
                            && ZztNative.editorPlace(30, 12, ELEMENT_EMPTY, 15, false) == 0,
                    "Could not prepare the empty cells used by keyboard movement checks");
            ZztNative.editorEnd();
            ZztNative.debugCommand(new byte[] {'A', 'M', 'M', 'O'});
            Button playButton = findButton(activity.getWindow().getDecorView(), "Play");
            if (playButton != null) {
                playFound[0] = true;
            }
        });
        waitForSaveToolVisibility(activity, View.GONE);
        runOnMain(() -> {
            Button playButton = findButton(activity.getWindow().getDecorView(), "Play");
            if (playButton != null) playClicked[0] = playButton.performClick();
        });
        require(playFound[0] && playClicked[0], "The title Play control could not start the test game");
        waitForPlayState(true);
        waitForSaveToolVisibility(activity, View.VISIBLE);
        verifyTitlePlayDoesNotRestartCurrentGame(activity);
        Object gameplayBoard = readField(MainActivity.class, activity, "boardView");
        runOnMain(() -> require(activity.getCurrentFocus() == gameplayBoard,
                "Starting gameplay did not give the board focus for physical keyboard input"));

        final int[][] initialKeyboardPosition = {null};
        runOnMain(() -> initialKeyboardPosition[0] = ZztNative.status());
        require(initialKeyboardPosition[0] != null && initialKeyboardPosition[0].length > 13,
                "The player position was unavailable before keyboard movement");
        pressKeyFor(KeyEvent.KEYCODE_DPAD_RIGHT, 350);
        int[] afterDpadRight = waitForPlayerCoordinateChange(12,
                initialKeyboardPosition[0][12], true);
        require(afterDpadRight[12] > initialKeyboardPosition[0][12],
                "Android D-pad Right did not move the player right");
        require(!((Set<?>) readField(MainActivity.class, activity, "heldNativeKeys"))
                        .contains(KEY_RIGHT),
                "Android D-pad Right remained held after its key-up event");
        pressKeyFor(KeyEvent.KEYCODE_NUMPAD_8, 350);
        int[] afterNumpadUp = waitForPlayerCoordinateChange(13, afterDpadRight[13], false);
        require(afterNumpadUp[13] < afterDpadRight[13],
                "Android Numpad 8 did not move the player up");
        require(!((Set<?>) readField(MainActivity.class, activity, "heldNativeKeys"))
                        .contains(KEY_UP),
                "Android Numpad 8 remained held after its key-up event");
        Log.i(TAG, "Gameplay input: D-pad Right and Numpad 8 moved the player");

        int[] right = viewCenter(activity, "Move ▶");
        int[] fire = viewCenter(activity, "Tap to fire; hold with a direction to keep firing");

        long directionDown = SystemClock.uptimeMillis();
        sendOnePointer(MotionEvent.ACTION_DOWN, directionDown, right);
        Thread.sleep(350);
        sendOnePointer(MotionEvent.ACTION_UP, directionDown, right);
        int[] facing = waitForPlayState(false);
        int ammoBefore = facing[2];
        require(ammoBefore >= 5, "The debug command did not add test ammo");

        long downTime = SystemClock.uptimeMillis();
        sendOnePointer(MotionEvent.ACTION_DOWN, downTime, right);
        Thread.sleep(80);
        sendTwoPointers(
                MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                downTime, right, fire);
        Thread.sleep(1800);
        sendTwoPointers(
                MotionEvent.ACTION_POINTER_UP | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                downTime, right, fire);
        sendOnePointer(MotionEvent.ACTION_UP, downTime, right);

        int[] after = waitForPlayState(false);
        require(after[2] <= ammoBefore - 2,
                "Repeated Fire did not consume at least two rounds (before="
                        + ammoBefore + ", after=" + after[2] + ")");
        verifyMissingWorldOpenPreservesGame();
        verifyHeldInputReleasedOnPause(activity, right, fire);
        runOnMain(ZztNative::returnToTitle);
        verifyEditorFloodFill();
        verifyImportSaveRestoreAndExport(activity);
    }

    private Activity launchMainActivity() throws Exception {
        Intent intent = new Intent();
        intent.setClassName(getTargetContext().getPackageName(), MainActivity.class.getName());
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        ActivityMonitor monitor = new ActivityMonitor(MainActivity.class.getName(), null, false);
        addMonitor(monitor);
        getTargetContext().startActivity(intent);
        Activity activity = waitForMonitorWithTimeout(monitor, 5000);
        removeMonitor(monitor);
        require(activity != null, "Android did not launch the target MainActivity");
        return activity;
    }

    private void verifyChoreographerFrameLifecycle(Activity activity) throws Exception {
        try {
            runOnMain(() -> {
                try {
                    require(Boolean.TRUE.equals(readField(MainActivity.class, activity, "framePosted")),
                            "MainActivity did not post a Choreographer callback after resume");
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not inspect the initial frame lifecycle", failure);
                }
            });
            runOnMain(() -> clearLastUiStatus(activity));
            int[] firstFrameStatus = waitForChoreographerStatus(activity);
            require(firstFrameStatus.length == 19,
                    "The Choreographer frame did not reach JNI and refresh native game status");

            runOnMain(() -> {
                callActivityOnPause(activity);
                try {
                    require(Boolean.FALSE.equals(readField(MainActivity.class, activity, "framePosted")),
                            "MainActivity left frame callbacks enabled after pause");
                    writeField(MainActivity.class, activity, "lastStatus", null);
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not inspect the paused frame lifecycle", failure);
                }
            });
            SystemClock.sleep(100);
            runOnMain(() -> {
                try {
                    require(readField(MainActivity.class, activity, "lastStatus") == null,
                            "A Choreographer callback refreshed the UI while the activity was paused");
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not inspect UI state while paused", failure);
                }
            });

            runOnMain(() -> {
                callActivityOnResume(activity);
                try {
                    require(Boolean.TRUE.equals(readField(MainActivity.class, activity, "framePosted")),
                            "MainActivity did not restart Choreographer callbacks after resume");
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not inspect the resumed frame lifecycle", failure);
                }
            });
            int[] resumedFrameStatus = waitForChoreographerStatus(activity);
            require(resumedFrameStatus.length == 19,
                    "Choreographer did not resume JNI frame delivery after activity resume");
        } finally {
            runOnMain(() -> {
                try {
                    if (Boolean.TRUE.equals(readField(MainActivity.class, activity, "framePosted"))) {
                        callActivityOnPause(activity);
                    }
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not stop the lifecycle test callbacks", failure);
                }
            });
        }
    }

    private void clearLastUiStatus(Activity activity) {
        try {
            writeField(MainActivity.class, activity, "lastStatus", null);
        } catch (ReflectiveOperationException failure) {
            throw new AssertionError("Could not reset the UI status frame marker", failure);
        }
    }

    private int[] waitForChoreographerStatus(Activity activity) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 3000;
        do {
            final int[][] status = {null};
            runOnMain(() -> {
                try {
                    Object value = readField(MainActivity.class, activity, "lastStatus");
                    if (value instanceof int[]) status[0] = Arrays.copyOf((int[]) value, ((int[]) value).length);
                } catch (ReflectiveOperationException failure) {
                    throw new AssertionError("Could not inspect the latest Choreographer status", failure);
                }
            });
            if (status[0] != null) return status[0];
            Thread.sleep(16);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("No Choreographer callback reached JNI/UI status refresh");
    }

    private void verifyImportSaveRestoreAndExport(Activity activity) throws Exception {
        File originalDirectory = (File) readField(MainActivity.class, activity, "activeDirectory");
        String originalFilename = (String) readField(MainActivity.class, activity, "activeFilename");
        boolean previousPickerSaveOnly = (Boolean) readField(MainActivity.class, activity, "pickerSaveOnly");
        require(originalDirectory != null && originalFilename != null,
                "The app did not expose its starting world directory and filename");

        String suffix = Long.toHexString(System.nanoTime());
        File sourceWorld = new File(activity.getCacheDir(), "ZZT-IMPORT-" + suffix + ".ZZT");
        File exportTarget = new File(activity.getCacheDir(), "ZZT-EXPORT-" + suffix + ".SAV");
        File restoreSource = new File(activity.getCacheDir(), "ZZT-RESTORE-" + suffix + ".SAV");
        File importedDirectory = null;
        String saveFilename = "ZZT-ROUNDTRIP-" + suffix + ".SAV";
        Method importFile = MainActivity.class.getDeclaredMethod("importFile", Uri.class);
        importFile.setAccessible(true);
        Field pickerSaveOnly = MainActivity.class.getDeclaredField("pickerSaveOnly");
        pickerSaveOnly.setAccessible(true);

        try {
            Log.i(TAG, "File flow: importing ZZT fixture");
            try (InputStream input = activity.getAssets().open("DEMO.ZZT");
                 OutputStream output = new FileOutputStream(sourceWorld)) {
                copy(input, output);
            }
            byte[] expectedWorld = readBytes(sourceWorld);

            runOnMain(() -> {
                try {
                    pickerSaveOnly.setBoolean(activity, false);
                    importFile.invoke(activity, Uri.fromFile(sourceWorld));
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's ZZT import path", failure);
                }
            });

            importedDirectory = (File) readField(MainActivity.class, activity, "activeDirectory");
            require(importedDirectory != null && !importedDirectory.equals(originalDirectory),
                    "Importing a valid ZZT did not switch to its private world folder");
            File importedWorld = new File(importedDirectory, sourceWorld.getName());
            require(importedWorld.isFile() && Arrays.equals(expectedWorld, readBytes(importedWorld)),
                    "Imported ZZT bytes did not match the source fixture");
            byte[] worldName = ZztNative.worldNameBytes();
            require(worldName != null && worldName.length > 0 && ZztNative.worldFileExists(worldName),
                    "The imported world could not find its internal-name reload alias");
            Log.i(TAG, "File flow: ZZT import and internal-name alias passed");
            int[] importedStatus = ZztNative.status();
            require(importedStatus != null && importedStatus.length > 15
                            && importedStatus[0] != GAME_STATE_PLAY && importedStatus[15] == 0,
                    "The imported ZZT did not leave the engine in its original title-world state");

            final int[] startResult = {-1};
            final int[] saveResult = {-1};
            final byte[][] boardBeforeSave = {new byte[BoardView.COLS * BoardView.ROWS * 2]};
            runOnMain(() -> {
                Log.i(TAG, "File flow: calling startGame");
                startResult[0] = ZztNative.startGame();
                Log.i(TAG, "File flow: startGame returned " + startResult[0]);
            });
            require(startResult[0] == 0, "Could not start the imported world for the save round-trip");

            // Pascal marks World.Info.IsSave only when a paused player moves to resume.
            // Reproduce that original transition before saving this .SAV fixture.
            final int[][] statusBeforeSave = {null};
            runOnMain(() -> {
                ZztNative.pauseGame();
                ZztNative.action(ACTION_RIGHT, true);
            });
            long saveReadyDeadline = SystemClock.uptimeMillis() + 3000;
            do {
                SystemClock.sleep(10);
                runOnMain(() -> statusBeforeSave[0] = ZztNative.status());
            } while ((statusBeforeSave[0] == null || statusBeforeSave[0].length <= 15
                    || statusBeforeSave[0][14] != 0 || statusBeforeSave[0][15] == 0)
                    && SystemClock.uptimeMillis() < saveReadyDeadline);
            require(statusBeforeSave[0] != null && statusBeforeSave[0].length > 15
                            && statusBeforeSave[0][14] == 0 && statusBeforeSave[0][15] == 1,
                    "Paused movement did not reach the original save-game state");

            runOnMain(() -> {
                ZztNative.action(ACTION_RIGHT, false);
                ZztNative.pauseGame();
                Log.i(TAG, "File flow: pauseGame returned");
                ZztNative.copyBoard(boardBeforeSave[0]);
                Log.i(TAG, "File flow: board capture before save returned");
                saveResult[0] = ZztNative.saveWorld(saveFilename);
                Log.i(TAG, "File flow: saveWorld returned " + saveResult[0]);
            });
            require(saveResult[0] == 0, "The app's native SAV save operation failed");
            File savedGame = new File(importedDirectory, saveFilename);
            require(savedGame.isFile() && savedGame.length() > 0,
                    "The native save operation did not create a nonempty SAV file");
            byte[] savedGameBytes = readBytes(savedGame);
            Log.i(TAG, "File flow: native SAV save passed");

            Field pendingExport = MainActivity.class.getDeclaredField("pendingExport");
            pendingExport.setAccessible(true);
            Field exportRequestCode = MainActivity.class.getDeclaredField("EXPORT_SAVE");
            exportRequestCode.setAccessible(true);
            Method onActivityResult = MainActivity.class.getDeclaredMethod(
                    "onActivityResult", int.class, int.class, Intent.class);
            onActivityResult.setAccessible(true);
            Intent exportResult = new Intent().setData(Uri.fromFile(exportTarget));
            runOnMain(() -> {
                try {
                    pendingExport.set(activity, savedGame);
                    onActivityResult.invoke(activity, exportRequestCode.getInt(null),
                            Activity.RESULT_OK, exportResult);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's SAV export result path", failure);
                }
            });
            require(exportTarget.isFile() && Arrays.equals(savedGameBytes, readBytes(exportTarget)),
                    "The app's SAV export callback changed the saved bytes");
            Log.i(TAG, "File flow: SAV export callback passed");

            try (InputStream input = new FileInputStream(savedGame);
                 OutputStream output = new FileOutputStream(restoreSource)) {
                copy(input, output);
            }
            final int[][] restoredStatus = new int[1][];
            final byte[][] restoredBoard = {new byte[boardBeforeSave[0].length]};
            Log.i(TAG, "File flow: importing saved game");
            runOnMain(() -> {
                try {
                    pickerSaveOnly.setBoolean(activity, true);
                    importFile.invoke(activity, Uri.fromFile(restoreSource));
                    Log.i(TAG, "File flow: SAV import method returned");
                    restoredStatus[0] = ZztNative.status();
                    Log.i(TAG, "File flow: native status read after SAV import");
                    ZztNative.copyBoard(restoredBoard[0]);
                    Log.i(TAG, "File flow: board read after SAV import");
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's SAV restore import path", failure);
                }
            });
            File restoredDirectory = (File) readField(MainActivity.class, activity, "activeDirectory");
            String restoredFilename = (String) readField(MainActivity.class, activity, "activeFilename");
            require(importedDirectory.equals(restoredDirectory)
                            && restoredFilename != null && restoredFilename.toLowerCase().endsWith(".sav"),
                    "Restoring the SAV did not reconnect it to its imported source world");
            require(restoredStatus[0] != null && restoredStatus[0].length > 15
                            && restoredStatus[0][0] == GAME_STATE_PLAY && restoredStatus[0][15] == 1,
                    "The imported SAV did not restore an active saved game");
            require(Arrays.equals(boardBeforeSave[0], restoredBoard[0]),
                    "The imported SAV did not restore the saved 60-by-25 playfield");
            verifyHighScoreImportExport(activity);
            verifyBoardImportExport(activity, sourceWorld.getName());
        } finally {
            try {
                runOnMain(() -> {
                    try {
                        ZztNative.pauseGame();
                        int directoryResult = ZztNative.setWorldDirectory(originalDirectory.getAbsolutePath());
                        int loadResult = directoryResult == 0
                                ? ZztNative.loadFile(originalFilename) : directoryResult;
                        if (loadResult != 0) {
                            Log.w(TAG, "Could not restore the original test world after file-flow checks: "
                                    + loadResult);
                        }
                        writeField(MainActivity.class, activity, "activeDirectory", originalDirectory);
                        writeField(MainActivity.class, activity, "activeFilename", originalFilename);
                        writeField(MainActivity.class, activity, "pendingExport", null);
                        pickerSaveOnly.setBoolean(activity, previousPickerSaveOnly);
                        Method updateDisplay = MainActivity.class.getDeclaredMethod("updateDisplay", long.class);
                        updateDisplay.setAccessible(true);
                        updateDisplay.invoke(activity, System.nanoTime() / 1_000_000L);
                    } catch (Throwable failure) {
                        Log.w(TAG, "Could not cleanly restore the test app after file-flow checks", failure);
                    }
                });
            } catch (Throwable failure) {
                Log.w(TAG, "Could not run test app file-flow cleanup on the UI thread", failure);
            }
            sourceWorld.delete();
            exportTarget.delete();
            restoreSource.delete();
            if (importedDirectory != null && !importedDirectory.equals(originalDirectory)) {
                deleteRecursively(importedDirectory);
            }
        }
    }

    private void verifyHighScoreImportExport(Activity activity) throws Exception {
        Method highScoreFileMethod = MainActivity.class.getDeclaredMethod("highScoreFile");
        highScoreFileMethod.setAccessible(true);
        File highScoreFile = (File) highScoreFileMethod.invoke(activity);
        boolean previousFileExists = highScoreFile.isFile();
        byte[] previousFile = previousFileExists ? readBytes(highScoreFile) : null;
        boolean previousPickerSaveOnly = (Boolean) readField(
                MainActivity.class, activity, "pickerSaveOnly");

        String suffix = Long.toHexString(System.nanoTime());
        File fixture = new File(activity.getCacheDir(), "ZZT-HIGHSCORES-" + suffix + ".HI");
        File exportTarget = new File(activity.getCacheDir(), "ZZT-HIGHSCORES-EXPORT-" + suffix + ".HI");
        HighScores fixtureScores = new HighScores();
        int position = fixtureScores.insertionPosition(32123);
        require(position == 0 && fixtureScores.insert(position, 32123, "PORT FLOW"),
                "Could not create the Android .HI import fixture");
        require(fixtureScores.save(fixture), "Could not write the Android .HI import fixture");
        byte[] expectedBytes = readBytes(fixture);

        Method importFile = MainActivity.class.getDeclaredMethod("importFile", Uri.class);
        importFile.setAccessible(true);
        Field pickerSaveOnly = MainActivity.class.getDeclaredField("pickerSaveOnly");
        pickerSaveOnly.setAccessible(true);
        Field pendingExport = MainActivity.class.getDeclaredField("pendingExport");
        pendingExport.setAccessible(true);
        Field exportRequestCode = MainActivity.class.getDeclaredField("EXPORT_HIGHSCORES");
        exportRequestCode.setAccessible(true);
        Method onActivityResult = MainActivity.class.getDeclaredMethod(
                "onActivityResult", int.class, int.class, Intent.class);
        onActivityResult.setAccessible(true);

        try {
            Log.i(TAG, "File flow: importing HI fixture");
            runOnMain(() -> {
                try {
                    pickerSaveOnly.setBoolean(activity, false);
                    importFile.invoke(activity, Uri.fromFile(fixture));
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's HI import path", failure);
                }
            });
            require(highScoreFile.isFile() && Arrays.equals(expectedBytes, readBytes(highScoreFile)),
                    "The app's HI import path changed the file bytes");

            Object liveScores = readField(MainActivity.class, activity, "highScores");
            Method displayRows = liveScores.getClass().getDeclaredMethod("displayRows");
            displayRows.setAccessible(true);
            String[] rows = (String[]) displayRows.invoke(liveScores);
            require(rows.length == 1 && rows[0].contains("PORT FLOW") && rows[0].contains("32123"),
                    "The imported HI entries were not loaded into the high-score display");
            Log.i(TAG, "File flow: HI import and display passed");

            Intent exportResult = new Intent().setData(Uri.fromFile(exportTarget));
            runOnMain(() -> {
                try {
                    pendingExport.set(activity, highScoreFile);
                    onActivityResult.invoke(activity, exportRequestCode.getInt(null),
                            Activity.RESULT_OK, exportResult);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's HI export result path", failure);
                }
            });
            require(exportTarget.isFile() && Arrays.equals(expectedBytes, readBytes(exportTarget)),
                    "The app's HI export callback changed the high-score bytes");
            Log.i(TAG, "File flow: HI export passed");
        } finally {
            if (previousFileExists) {
                try (OutputStream output = new FileOutputStream(highScoreFile)) {
                    output.write(previousFile);
                }
            } else {
                highScoreFile.delete();
            }
            runOnMain(() -> {
                try {
                    Object liveScores = readField(MainActivity.class, activity, "highScores");
                    Method loadInto = liveScores.getClass().getDeclaredMethod("loadInto", File.class);
                    loadInto.setAccessible(true);
                    loadInto.invoke(liveScores, highScoreFile);
                    pickerSaveOnly.setBoolean(activity, previousPickerSaveOnly);
                    pendingExport.set(activity, null);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not restore the app's high-score state", failure);
                }
            });
            fixture.delete();
            exportTarget.delete();
        }
    }

    private void verifyBoardImportExport(Activity activity, String worldFilename) throws Exception {
        String suffix = Long.toHexString(System.nanoTime());
        File fixture = new File(activity.getCacheDir(), "ZZT-BOARD-FLOW-" + suffix + ".BRD");
        File truncatedFixture = new File(activity.getCacheDir(), "ZZT-BOARD-SHORT-" + suffix + ".BRD");
        File exportTarget = new File(activity.getCacheDir(), "ZZT-BOARD-EXPORT-" + suffix + ".BRD");
        Method beginEditor = MainActivity.class.getDeclaredMethod("beginEditor");
        Method finishEditor = MainActivity.class.getDeclaredMethod("finishEditor");
        Method onActivityResult = MainActivity.class.getDeclaredMethod(
                "onActivityResult", int.class, int.class, Intent.class);
        Field editorActive = MainActivity.class.getDeclaredField("editorActive");
        Field activeFilename = MainActivity.class.getDeclaredField("activeFilename");
        Field pendingBoardExport = MainActivity.class.getDeclaredField("pendingBoardExport");
        Field openBoardRequestCode = MainActivity.class.getDeclaredField("OPEN_BOARD");
        Field exportBoardRequestCode = MainActivity.class.getDeclaredField("EXPORT_BOARD");
        beginEditor.setAccessible(true);
        finishEditor.setAccessible(true);
        onActivityResult.setAccessible(true);
        editorActive.setAccessible(true);
        activeFilename.setAccessible(true);
        pendingBoardExport.setAccessible(true);
        openBoardRequestCode.setAccessible(true);
        exportBoardRequestCode.setAccessible(true);

        final byte[][] expectedBoard = {null};
        final byte[][] expectedClearedBoard = {null};
        try {
            Log.i(TAG, "File flow: preparing editor BRD fixture");
            runOnMain(() -> {
                try {
                    ZztNative.returnToTitle();
                    require(ZztNative.loadFile(worldFilename) == 0,
                            "Could not load the imported ZZT world before .BRD testing");
                    activeFilename.set(activity, worldFilename);
                    beginEditor.invoke(activity);
                    require(editorActive.getBoolean(activity),
                            "The app did not enter its board editor for .BRD testing");
                    require(ZztNative.editorNewWorld() == 0,
                            "Could not create the blank editor world for .BRD testing");
                    expectedBoard[0] = ZztNative.editorExportBoard();
                    require(expectedBoard[0] != null && expectedBoard[0].length > 2,
                            "Could not serialize the .BRD import fixture");
                } catch (Throwable failure) {
                    throw new AssertionError("Could not prepare the app's .BRD editor flow", failure);
                }
            });
            try (OutputStream output = new FileOutputStream(fixture)) {
                output.write(expectedBoard[0]);
            }

            Intent importResult = new Intent().setData(Uri.fromFile(fixture));
            Log.i(TAG, "File flow: importing BRD fixture");
            runOnMain(() -> {
                try {
                    onActivityResult.invoke(activity, openBoardRequestCode.getInt(null),
                            Activity.RESULT_OK, importResult);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's .BRD import result path", failure);
                }
            });
            final byte[][] importedBoard = {null};
            runOnMain(() -> importedBoard[0] = ZztNative.editorExportBoard());
            require(Arrays.equals(expectedBoard[0], importedBoard[0]),
                    "The app's .BRD import path changed the serialized board bytes");
            Log.i(TAG, "File flow: BRD import and byte-exact board round-trip passed");

            Intent exportResult = new Intent().setData(Uri.fromFile(exportTarget));
            runOnMain(() -> {
                try {
                    pendingBoardExport.set(activity, importedBoard[0]);
                    onActivityResult.invoke(activity, exportBoardRequestCode.getInt(null),
                            Activity.RESULT_OK, exportResult);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's .BRD export result path", failure);
                }
            });
            require(exportTarget.isFile() && Arrays.equals(importedBoard[0], readBytes(exportTarget)),
                    "The app's .BRD export callback changed the board bytes");
            Log.i(TAG, "File flow: BRD export callback passed");

            runOnMain(() -> {
                try {
                    require(ZztNative.editorClearBoard() == 0,
                            "Could not capture the Pascal BoardCreate result for the short-read oracle");
                    expectedClearedBoard[0] = ZztNative.editorExportBoard();
                    require(expectedClearedBoard[0] != null,
                            "Could not serialize the Pascal BoardCreate short-read oracle");
                    require(ZztNative.editorImportBoard(expectedBoard[0]) == 0,
                            "Could not restore the valid board before the short-read test");
                } catch (Throwable failure) {
                    throw new AssertionError("Could not prepare the .BRD short-read oracle", failure);
                }
            });
            try (OutputStream output = new FileOutputStream(truncatedFixture)) {
                output.write(new byte[] {4, 0, 0x5a});
            }
            Intent truncatedResult = new Intent().setData(Uri.fromFile(truncatedFixture));
            runOnMain(() -> {
                try {
                    onActivityResult.invoke(activity, openBoardRequestCode.getInt(null),
                            Activity.RESULT_OK, truncatedResult);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not invoke the app's truncated .BRD import path", failure);
                }
            });
            final byte[][] actualClearedBoard = {null};
            runOnMain(() -> actualClearedBoard[0] = ZztNative.editorExportBoard());
            require(Arrays.equals(expectedClearedBoard[0], actualClearedBoard[0]),
                    "A truncated .BRD payload did not reset the current board like Pascal BoardCreate");
            Log.i(TAG, "File flow: truncated BRD read reset matched BoardCreate");
        } finally {
            runOnMain(() -> {
                try {
                    if (editorActive.getBoolean(activity)) finishEditor.invoke(activity);
                    int loadResult = ZztNative.loadFile(worldFilename);
                    require(loadResult == 0, "Could not restore the imported ZZT after .BRD testing");
                    activeFilename.set(activity, worldFilename);
                    pendingBoardExport.set(activity, null);
                    Method updateDisplay = MainActivity.class.getDeclaredMethod("updateDisplay", long.class);
                    updateDisplay.setAccessible(true);
                    updateDisplay.invoke(activity, System.nanoTime() / 1_000_000L);
                } catch (Throwable failure) {
                    throw new AssertionError("Could not restore the app after .BRD file-flow checks", failure);
                }
            });
            fixture.delete();
            truncatedFixture.delete();
            exportTarget.delete();
        }
    }

    private static Object readField(Class<?> owner, Object instance, String name)
            throws ReflectiveOperationException {
        Field field = owner.getDeclaredField(name);
        field.setAccessible(true);
        return field.get(instance);
    }

    private static void writeField(Class<?> owner, Object instance, String name, Object value)
            throws ReflectiveOperationException {
        Field field = owner.getDeclaredField(name);
        field.setAccessible(true);
        field.set(instance, value);
    }

    private static byte[] readBytes(File file) throws Exception {
        try (InputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            copy(input, output);
            return output.toByteArray();
        }
    }

    private static void copy(InputStream input, OutputStream output) throws Exception {
        byte[] buffer = new byte[8192];
        int count;
        while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
        output.flush();
    }

    private static void deleteRecursively(File file) {
        if (file == null || !file.exists()) return;
        File[] children = file.listFiles();
        if (children != null) {
            for (File child : children) deleteRecursively(child);
        }
        if (!file.delete()) Log.w(TAG, "Could not remove Android test file " + file);
    }

    private void verifyEditorFloodFill() {
        runOnMain(() -> {
            boolean editorStarted = false;
            try {
                require(ZztNative.editorBegin() >= 0,
                        "Could not enter the title board editor for flood-fill testing");
                editorStarted = true;
                require(ZztNative.editorClearBoard() == 0,
                        "Could not clear the title board for flood-fill testing");
                require(ZztNative.editorPlace(5, 5, ELEMENT_PLAYER, 15, false) == 0,
                        "Could not move the player outside the bounded flood-fill test area");

                for (int y = 10; y <= 14; y++) {
                    for (int x = 28; x <= 32; x++) {
                        require(ZztNative.editorPlace(x, y, ELEMENT_EMPTY, 15, false) == 0,
                                "Could not prepare the bounded flood-fill test area");
                    }
                }
                for (int x = 28; x <= 32; x++) {
                    require(ZztNative.editorPlace(x, 10, ELEMENT_SOLID, 15, false) == 0
                                    && ZztNative.editorPlace(x, 14, ELEMENT_SOLID, 15, false) == 0,
                            "Could not place the horizontal flood-fill boundary");
                }
                for (int y = 11; y <= 13; y++) {
                    require(ZztNative.editorPlace(28, y, ELEMENT_SOLID, 15, false) == 0
                                    && ZztNative.editorPlace(32, y, ELEMENT_SOLID, 15, false) == 0,
                            "Could not place the vertical flood-fill boundary");
                }

                byte[] before = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(before);
                int filled = ZztNative.editorFloodFill(30, 12, ELEMENT_SOLID, 14, false, true);
                require(filled == 9,
                        "The enclosed 3-by-3 flood fill changed " + filled + " cells instead of 9");

                byte[] after = new byte[before.length];
                ZztNative.copyBoard(after);
                for (int y = 11; y <= 13; y++) {
                    for (int x = 29; x <= 31; x++) {
                        int offset = boardOffset(x, y);
                        require((after[offset] & 0xff) == 219 && (after[offset + 1] & 0xff) == 14,
                                "Pattern flood fill did not preserve cursor color at " + x + "," + y);
                    }
                }
                int boundary = boardOffset(28, 12);
                require((after[boundary] & 0xff) == 219 && (after[boundary + 1] & 0xff) == 15,
                        "Flood fill changed its solid boundary");
                int outside = boardOffset(27, 12);
                require(after[outside] == before[outside]
                                && after[outside + 1] == before[outside + 1],
                        "Flood fill crossed the bounded region");

                require(ZztNative.editorClearBoard() == 0,
                        "Could not clear the title board for the queue-wrap test");
                require(ZztNative.editorPlace(30, 2, ELEMENT_PLAYER, 15, false) == 0,
                        "Could not move the player away from the queue-wrap path");
                for (int y = 1; y <= BoardView.ROWS; y++) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        if (x == 30 && y == 2) continue;
                        require(ZztNative.editorPlace(x, y, ELEMENT_SOLID, 15, false) == 0,
                                "Could not create the solid queue-wrap test board");
                    }
                }
                for (int y = 1; y <= BoardView.ROWS; y += 2) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        require(ZztNative.editorPlace(x, y, ELEMENT_EMPTY, 15, false) == 0,
                                "Could not create a queue-wrap corridor");
                    }
                }
                for (int y = 2; y <= BoardView.ROWS; y += 2) {
                    int connectorX = ((y / 2) % 2) == 1 ? BoardView.COLS : 1;
                    require(ZztNative.editorPlace(connectorX, y, ELEMENT_EMPTY, 15, false) == 0,
                            "Could not connect the queue-wrap corridor rows");
                }

                int wrapped = ZztNative.editorFloodFill(30, 1, ELEMENT_SOLID, 14, false, true);
                require(wrapped == 792,
                        "The byte-counter-wrap corridor changed " + wrapped + " cells instead of 792");
                byte[] wrappedBoard = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(wrappedBoard);
                for (int y = 1; y <= BoardView.ROWS; y++) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        int offset = boardOffset(x, y);
                        boolean player = x == 30 && y == 2;
                        boolean corridor = (y % 2) == 1
                                || ((y % 2) == 0
                                        && x == (((y / 2) % 2) == 1 ? BoardView.COLS : 1));
                        int expectedElement = player ? CHARACTER_PLAYER : CHARACTER_SOLID;
                        int actualElement = wrappedBoard[offset] & 0xff;
                        require(actualElement == expectedElement,
                                "Queue-wrap fill changed the board element at " + x + "," + y
                                        + " (expected=" + expectedElement + ", actual=" + actualElement
                                        + ", changed=" + wrapped + ")");
                        if (!player) {
                            int expectedColor = corridor ? 14 : 15;
                            require((wrappedBoard[offset + 1] & 0xff) == expectedColor,
                                    "Queue-wrap fill changed the board color at " + x + "," + y);
                        }
                    }
                }

                require(ZztNative.editorClearBoard() == 0,
                        "Could not clear the title board for the full-board fill test");
                require(ZztNative.editorPlace(1, 1, ELEMENT_PLAYER, 15, false) == 0,
                        "Could not place the player outside the full-board fill region");
                for (int y = 2; y <= 24; y++) {
                    for (int x = 2; x <= 59; x++) {
                        require(ZztNative.editorPlace(x, y, ELEMENT_EMPTY, 15, false) == 0,
                                "Could not create the 58-by-23 full-board fill region");
                    }
                }
                byte[] beforeFullBoard = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(beforeFullBoard);
                int fullBoardFilled = ZztNative.editorFloodFill(30, 12,
                        ELEMENT_SOLID, 14, false, true);
                require(fullBoardFilled == 1334,
                        "The full-board fill changed " + fullBoardFilled + " cells instead of 1334");
                byte[] fullBoard = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(fullBoard);
                for (int y = 1; y <= BoardView.ROWS; y++) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        int offset = boardOffset(x, y);
                        boolean player = x == 1 && y == 1;
                        boolean region = x >= 2 && x <= 59 && y >= 2 && y <= 24;
                        if (region) {
                            require((fullBoard[offset] & 0xff) == CHARACTER_SOLID
                                            && (fullBoard[offset + 1] & 0xff) == 14,
                                    "Full-board fill changed the tile at " + x + "," + y);
                        } else if (!player) {
                            require(fullBoard[offset] == beforeFullBoard[offset]
                                            && fullBoard[offset + 1] == beforeFullBoard[offset + 1],
                                    "Full-board fill changed its border at " + x + "," + y);
                        }
                    }
                }

                require(ZztNative.editorClearBoard() == 0,
                        "Could not clear the title board for the dense queue test");
                require(ZztNative.editorPlace(1, 1, ELEMENT_PLAYER, 15, false) == 0,
                        "Could not move the player away from the dense fill region");
                for (int y = 1; y <= BoardView.ROWS; y++) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        if (x == 1 && y == 1) continue;
                        require(ZztNative.editorPlace(x, y, ELEMENT_SOLID, 15, false) == 0,
                                "Could not create the dense queue test board");
                    }
                }
                for (int y = 4; y <= 20; y++) {
                    for (int x = 20; x <= 36; x++) {
                        require(ZztNative.editorPlace(x, y, ELEMENT_EMPTY, 15, false) == 0,
                                "Could not create the dense 17-by-17 fill region");
                    }
                }
                int denseFilled = ZztNative.editorFloodFill(28, 12,
                        ELEMENT_SOLID, 14, false, true);
                require(denseFilled == 289,
                        "The dense 17-by-17 flood fill changed " + denseFilled + " cells instead of 289");
                byte[] denseBoard = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(denseBoard);
                for (int y = 1; y <= BoardView.ROWS; y++) {
                    for (int x = 1; x <= BoardView.COLS; x++) {
                        int offset = boardOffset(x, y);
                        boolean player = x == 1 && y == 1;
                        boolean region = x >= 20 && x <= 36 && y >= 4 && y <= 20;
                        int expectedElement = player ? CHARACTER_PLAYER : CHARACTER_SOLID;
                        require((denseBoard[offset] & 0xff) == expectedElement,
                                "Dense fill changed the board element at " + x + "," + y);
                        if (!player) {
                            int expectedColor = region ? 14 : 15;
                            require((denseBoard[offset + 1] & 0xff) == expectedColor,
                                    "Dense fill changed the board color at " + x + "," + y);
                        }
                    }
                }

            } finally {
                if (editorStarted) ZztNative.editorEnd();
            }
        });
    }

    private static int boardOffset(int x, int y) {
        return ((y - 1) * BoardView.COLS + x - 1) * 2;
    }

    private void verifyHeldInputReleasedOnPause(Activity activity, int[] right, int[] fire)
            throws Exception {
        long downTime = SystemClock.uptimeMillis();
        sendOnePointer(MotionEvent.ACTION_DOWN, downTime, right);
        sendTwoPointers(
                MotionEvent.ACTION_POINTER_DOWN | (1 << MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                downTime, right, fire);

        runOnMain(() -> {
            try {
                Set<Integer> keys = readInputSet(activity, "heldNativeKeys");
                Set<Integer> actions = readInputSet(activity, "heldNativeActions");
                require(keys.contains(KEY_RIGHT), "Touch did not register a held right key");
                require(actions.contains(ACTION_SHOOT), "Touch did not register a held Fire action");

                Method release = MainActivity.class.getDeclaredMethod("releaseHeldInput");
                release.setAccessible(true);
                release.invoke(activity);
                require(keys.isEmpty() && actions.isEmpty(),
                        "Pause cleanup left a native key or action held");
                View rightButton = findDescription(activity.getWindow().getDecorView(),
                        "Move ▶");
                require(rightButton != null && !rightButton.isPressed(),
                        "Pause cleanup left the touch button visually pressed");
            } catch (ReflectiveOperationException failure) {
                throw new AssertionError("Could not inspect pause input cleanup", failure);
            }
        });

        sendTwoPointers(MotionEvent.ACTION_CANCEL, downTime, right, fire);
        long nextDown = SystemClock.uptimeMillis();
        sendOnePointer(MotionEvent.ACTION_DOWN, nextDown, right);
        runOnMain(() -> {
            try {
                require(readInputSet(activity, "heldNativeKeys").contains(KEY_RIGHT),
                        "Touch direction did not work after pause cleanup");
            } catch (ReflectiveOperationException failure) {
                throw new AssertionError("Could not inspect touch input after pause cleanup", failure);
            }
        });
        sendOnePointer(MotionEvent.ACTION_UP, nextDown, right);
        runOnMain(() -> {
            try {
                require(readInputSet(activity, "heldNativeKeys").isEmpty(),
                        "Touch direction did not release after the next gesture");
            } catch (ReflectiveOperationException failure) {
                throw new AssertionError("Could not inspect released touch input", failure);
            }
        });
    }

    @SuppressWarnings("unchecked")
    private static Set<Integer> readInputSet(Activity activity, String fieldName)
            throws ReflectiveOperationException {
        Field field = MainActivity.class.getDeclaredField(fieldName);
        field.setAccessible(true);
        return (Set<Integer>) field.get(activity);
    }

    private void verifyPascalMonochromeTextAttribute() {
        runOnMain(() -> {
            boolean editorStarted = false;
            try {
                int beginResult = ZztNative.editorBegin();
                require(beginResult >= 0, "Could not enter the title board editor for monochrome testing");
                editorStarted = true;

                byte[] frame = new byte[BoardView.COLS * BoardView.ROWS * 2];
                ZztNative.copyBoard(frame);
                int x = -1;
                int y = -1;
                for (int cellY = 1; cellY <= BoardView.ROWS && x < 0; cellY++) {
                    for (int cellX = 1; cellX <= BoardView.COLS; cellX++) {
                        int offset = ((cellY - 1) * BoardView.COLS + cellX - 1) * 2;
                        if (frame[offset] == ' ' && (frame[offset + 1] & 0xff) == 0x0f) {
                            x = cellX;
                            y = cellY;
                            break;
                        }
                    }
                }
                require(x >= 0, "Could not find an empty board cell for a test text tile");
                require(ZztNative.editorPlaceText(x, y, 'A', 14) == 0,
                        "Could not place a colored text tile for monochrome testing");

                int offset = ((y - 1) * BoardView.COLS + x - 1) * 2;
                ZztNative.copyBoard(frame);
                require((frame[offset + 1] & 0xff) == 0x6f,
                        "Color text-tile attribute differed from Pascal's 0x6F value");

                byte[] monochromeFrame = new byte[frame.length];
                ZztNative.setMonochrome(true);
                ZztNative.copyBoard(monochromeFrame);
                require((monochromeFrame[offset + 1] & 0xff) == 0x60,
                        "Monochrome text tile did not use Pascal's background-only 0x60 attribute");

                ZztNative.setMonochrome(false);
                ZztNative.copyBoard(frame);
                require((frame[offset + 1] & 0xff) == 0x6f,
                        "Returning to color did not restore Pascal's text-tile attribute");
            } finally {
                ZztNative.setMonochrome(false);
                if (editorStarted) ZztNative.editorEnd();
            }
        });
    }

    private void verifyTitlePlayDoesNotRestartCurrentGame(Activity activity) throws Exception {
        final int[][] beforeStatus = new int[1][];
        final int[][] afterStatus = new int[1][];
        final byte[][] beforeBoard = new byte[1][];
        final byte[][] afterBoard = new byte[1][];
        final int[] playVisibility = {-1};
        runOnMain(() -> {
            beforeStatus[0] = ZztNative.status();
            beforeBoard[0] = new byte[BoardView.COLS * BoardView.ROWS * 2];
            ZztNative.copyBoard(beforeBoard[0]);
            Button playButton = findButton(activity.getWindow().getDecorView(), "Play");
            if (playButton != null) {
                playVisibility[0] = playButton.getVisibility();
                playButton.performClick();
            }
            afterStatus[0] = ZztNative.status();
            afterBoard[0] = new byte[beforeBoard[0].length];
            ZztNative.copyBoard(afterBoard[0]);
        });
        require(beforeStatus[0] != null && beforeStatus[0].length > 0
                        && beforeStatus[0][0] == GAME_STATE_PLAY,
                "The title-only Play check did not start from gameplay");
        require(playVisibility[0] == View.GONE,
                "The title-only Play control is missing or visible during gameplay"
                        + " (visibility=" + playVisibility[0]
                        + ", state=" + (beforeStatus[0] == null || beforeStatus[0].length == 0
                                ? "missing" : beforeStatus[0][0]) + ")");
        require(Arrays.equals(beforeStatus[0], afterStatus[0]),
                "The Play control restarted or changed an already-running game");
        require(Arrays.equals(beforeBoard[0], afterBoard[0]),
                "The Play control changed the active board while gameplay was in progress");
    }

    private void waitForSaveToolVisibility(Activity activity, int expectedVisibility) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 3000;
        do {
            final int[] visibility = {-1};
            runOnMain(() -> {
                Button saveButton = findButton(activity.getWindow().getDecorView(), "Save");
                if (saveButton != null) visibility[0] = saveButton.getVisibility();
            });
            if (visibility[0] == expectedVisibility) return;
            Thread.sleep(30);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("Save game toolbar visibility was "
                + (expectedVisibility == View.GONE ? "not hidden at title" : "not shown during play"));
    }

    private void verifyMissingWorldOpenPreservesGame() throws Exception {
        final int[][] beforeStatus = new int[1][];
        final int[][] afterStatus = new int[1][];
        final byte[][] beforeBoard = new byte[1][];
        final byte[][] afterBoard = new byte[1][];
        final int[] loadResult = new int[1];
        runOnMain(() -> {
            beforeStatus[0] = ZztNative.status();
            beforeBoard[0] = new byte[BoardView.COLS * BoardView.ROWS * 2];
            ZztNative.copyBoard(beforeBoard[0]);
            String missingName = "MISSING-COMPAT-" + Long.toHexString(System.nanoTime()) + ".ZZT";
            loadResult[0] = ZztNative.loadFile(missingName);
            afterStatus[0] = ZztNative.status();
            afterBoard[0] = new byte[beforeBoard[0].length];
            ZztNative.copyBoard(afterBoard[0]);
        });

        require(loadResult[0] != 0, "Opening a missing world unexpectedly succeeded");
        require(beforeStatus[0] != null && afterStatus[0] != null
                        && beforeStatus[0].length == afterStatus[0].length,
                "Native status could not be read around the failed open");
        for (int i = 0; i < beforeStatus[0].length; i++) {
            if (i == STATUS_LAST_ERROR_FIELD) continue;
            require(beforeStatus[0][i] == afterStatus[0][i],
                    "Failed world open changed native status field " + i);
        }
        require(Arrays.equals(beforeBoard[0], afterBoard[0]),
                "Failed world open changed the active 60-by-25 board");
    }

    private int[] waitForPlayState(boolean paused) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 3000;
        do {
            final int[][] current = new int[1][];
            runOnMain(() -> current[0] = ZztNative.status());
            if (current[0] != null && current[0].length >= 15
                    && current[0][0] == GAME_STATE_PLAY
                    && current[0][14] == (paused ? 1 : 0)) {
                return current[0];
            }
            Thread.sleep(30);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("Native game did not reach play state; paused=" + paused);
    }

    private void pressKeyFor(int keyCode, long durationMillis) throws Exception {
        long downTime = SystemClock.uptimeMillis();
        sendKeySync(new KeyEvent(downTime, downTime, KeyEvent.ACTION_DOWN, keyCode, 0));
        try {
            Thread.sleep(durationMillis);
        } finally {
            sendKeySync(new KeyEvent(downTime, SystemClock.uptimeMillis(),
                    KeyEvent.ACTION_UP, keyCode, 0));
        }
    }

    private int[] waitForPlayerCoordinateChange(int coordinateIndex, int startingValue,
            boolean mustIncrease) throws Exception {
        long deadline = SystemClock.uptimeMillis() + 3000;
        do {
            final int[][] current = new int[1][];
            runOnMain(() -> current[0] = ZztNative.status());
            if (current[0] != null && current[0].length > coordinateIndex
                    && current[0][0] == GAME_STATE_PLAY
                    && (mustIncrease ? current[0][coordinateIndex] > startingValue
                    : current[0][coordinateIndex] < startingValue)) {
                return current[0];
            }
            Thread.sleep(20);
        } while (SystemClock.uptimeMillis() < deadline);
        throw new AssertionError("Player coordinate " + coordinateIndex
                + " did not move in the requested direction from " + startingValue);
    }

    private int[] viewCenter(Activity activity, String description) {
        final View[] found = new View[1];
        runOnMain(() -> findDescription(activity.getWindow().getDecorView(), description, found));
        require(found[0] != null, "Missing touch control: " + description);
        int[] location = new int[2];
        found[0].getLocationOnScreen(location);
        return new int[] {location[0] + found[0].getWidth() / 2,
                location[1] + found[0].getHeight() / 2};
    }

    private static Button findButton(View view, String label) {
        if (view instanceof Button && label.contentEquals(((Button) view).getText())) {
            return (Button) view;
        }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                Button found = findButton(group.getChildAt(i), label);
                if (found != null) return found;
            }
        }
        return null;
    }

    private static void findDescription(View view, String description, View[] result) {
        if (description.equals(view.getContentDescription())) {
            result[0] = view;
            return;
        }
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount() && result[0] == null; i++) {
                findDescription(group.getChildAt(i), description, result);
            }
        }
    }

    private static View findDescription(View view, String description) {
        View[] result = new View[1];
        findDescription(view, description, result);
        return result[0];
    }

    private void sendOnePointer(int action, long downTime, int[] point) {
        MotionEvent.PointerProperties property = new MotionEvent.PointerProperties();
        property.id = 0;
        property.toolType = MotionEvent.TOOL_TYPE_FINGER;
        send(action, downTime, new MotionEvent.PointerProperties[] {property},
                new MotionEvent.PointerCoords[] {coordinates(point)});
    }

    private void sendTwoPointers(int action, long downTime, int[] first, int[] second) {
        MotionEvent.PointerProperties firstProperty = new MotionEvent.PointerProperties();
        firstProperty.id = 0;
        firstProperty.toolType = MotionEvent.TOOL_TYPE_FINGER;
        MotionEvent.PointerProperties secondProperty = new MotionEvent.PointerProperties();
        secondProperty.id = 1;
        secondProperty.toolType = MotionEvent.TOOL_TYPE_FINGER;
        send(action, downTime,
                new MotionEvent.PointerProperties[] {firstProperty, secondProperty},
                new MotionEvent.PointerCoords[] {coordinates(first), coordinates(second)});
    }

    private static MotionEvent.PointerCoords coordinates(int[] point) {
        MotionEvent.PointerCoords result = new MotionEvent.PointerCoords();
        result.x = point[0];
        result.y = point[1];
        result.pressure = 1f;
        result.size = 1f;
        return result;
    }

    private void send(int action, long downTime,
                      MotionEvent.PointerProperties[] properties,
                      MotionEvent.PointerCoords[] coordinates) {
        MotionEvent event = MotionEvent.obtain(downTime, SystemClock.uptimeMillis(), action,
                properties.length, properties, coordinates, 0, 0, 1f, 1f, 0, 0,
                InputDevice.SOURCE_TOUCHSCREEN, 0);
        try {
            sendPointerSync(event);
        } finally {
            event.recycle();
        }
    }

    private void runOnMain(Runnable runnable) {
        final Throwable[] failure = {null};
        runOnMainSync(() -> {
            try {
                runnable.run();
            } catch (Throwable throwable) {
                failure[0] = throwable;
            }
        });
        if (failure[0] instanceof Error) throw (Error) failure[0];
        if (failure[0] instanceof RuntimeException) throw (RuntimeException) failure[0];
        if (failure[0] != null) throw new AssertionError(failure[0]);
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
