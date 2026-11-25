package com.kerry.zzt;

import java.nio.charset.Charset;

final class ZztNative {
    private static final Charset CP437 = Charset.forName("IBM437");

    static final int EDITOR_BRD_IMPORTED = 0;
    static final int EDITOR_BRD_REOPENED_PREVIOUS = 1;
    static final int EDITOR_BRD_RESET_AFTER_SHORT_PAYLOAD = 2;

    static {
        System.loadLibrary("zztengine");
    }

    static native int init(String worldDirectory, String initialWorld);
    static native int setWorldDirectory(String worldDirectory);
    static native int installWorldAlias(String sourceFilename);
    static native boolean worldFileExists(byte[] cp437WorldName);
    static native int loadFile(String filename);
    static native int startGame();
    static native int tickSpeed();
    static native void setTickSpeed(int speed);
    static native int editorBegin();
    static native boolean editorWasModified();
    static native int editorRefreshBoard();
    static native int editorLoadWorldFile(String filename);
    static native int editorResetLoadedWorld();
    static native int editorNewWorld();
    static native void editorEnd();
    static native int editorClearBoard();
    static native int editorSwitchBoard(int boardId);
    static native int editorAppendBoard(byte[] cp437Name);
    static native byte[] editorBoardNameBytes(int boardId);
    static native int[] editorBoardInfo();
    static native int editorUpdateBoardInfo(byte[] cp437Name, int maxShots, int isDark,
            int reenterWhenZapped, int north, int south, int west, int east, int timeLimitSec);
    static native int editorImportBoard(byte[] boardFile);
    static native byte[] editorExportBoard();
    static native String[] editorElements(int category);
    static native int editorElementByShortcut(int category, int shortcut);
    static native String[] editorElementParameters(int element);
    static native int editorPlace(int x, int y, int element, int cursorColor,
            boolean usePatternColor);
    static native int editorPlot(int x, int y, int element, int cursorColor,
            boolean usePatternColor);
    static native int editorPrepareTile(int x, int y);
    static native int editorCopyTile(int x, int y);
    static native int editorPasteTile(int x, int y);
    static native int editorPlaceClipboardPattern(int x, int y);
    static native int editorPlotClipboardPattern(int x, int y);
    static native int editorPlaceText(int x, int y, int character, int cursorColor);
    static native int editorFloodFill(int x, int y, int element, int cursorColor,
            boolean useClipboard, boolean usePatternColor);
    static native int[] editorStatValues(int x, int y);
    static native int editorUpdateStat(int x, int y, int p1, int p2, int p3, int stepX, int stepY);
    static native byte[] editorStatText(int x, int y);
    static native int editorUpdateStatText(int x, int y, byte[] text);
    static native int editorRemoveStat(int x, int y);
    static native int returnToTitle();
    static native void setMonochrome(boolean enabled);
    static native void setModalPromptBlocked(boolean blocked);
    static native void togglePause();
    static native void pauseGame();
    static native boolean gameCommandsAvailable();
    static native boolean gameHasMonitor();
    static native void debugCommand(byte[] cp437Command);
    static native boolean toggleSound();
    static native boolean queueSoundToggle();
    static native int setWorldName(byte[] cp437Name);
    static native int saveWorld(String filename);
    static native void key(int key, boolean down);
    static native void keyStroke(int key);
    static native void action(int action, boolean down);
    static native void shootTap();
    static native void frame(long frameTimeNanos);
    static native void copyBoard(byte[] target);
    static native int takeTransition(byte[] from, byte[] to, short[] order);
    static native int[] status();
    static native int highScoresInitialize(byte[] table);
    static native int highScoresLoad(byte[] table, byte[] record);
    static native byte[] highScoresSerialize(byte[] table);
    static native int highScoresInsertionPosition(byte[] table, int score);
    static native int highScoresInsert(byte[] table, int position, int score, byte[] name);
    static native int highScoreValue(byte[] table, int index);
    static native int highScoreNameLength(byte[] table, int index);
    static native byte[] highScoreNameBytes(byte[] table, int index);
    static native byte[] worldNameBytes();
    private static native byte[] boardNameBytes();
    static String worldName() { return decodeCp437(worldNameBytes()); }
    static String boardName() { return decodeCp437(boardNameBytes()); }
    static native byte[] font8x14();
    static native void renderAudio(byte[] target);

    private static String decodeCp437(byte[] bytes) {
        return bytes == null ? "" : new String(bytes, CP437);
    }

    private ZztNative() {}
}
