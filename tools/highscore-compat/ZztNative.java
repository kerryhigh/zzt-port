package com.kerry.zzt;

/** Compile-time JNI stub; the adapter test only calls HighScores.encodeCp437. */
final class ZztNative {
    static int highScoresInitialize(byte[] table) { return 0; }
    static int highScoresLoad(byte[] table, byte[] record) { return 0; }
    static byte[] highScoresSerialize(byte[] table) { return null; }
    static int highScoresInsertionPosition(byte[] table, int score) { return -1; }
    static int highScoresInsert(byte[] table, int position, int score, byte[] name) { return -1; }
    static int highScoreValue(byte[] table, int index) { return 0; }
    static int highScoreNameLength(byte[] table, int index) { return 0; }
    static byte[] highScoreNameBytes(byte[] table, int index) { return null; }
}
