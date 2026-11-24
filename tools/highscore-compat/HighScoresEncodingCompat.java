package com.kerry.zzt;

import java.util.Arrays;

/** Checks the Android name-entry adapter's byte encoding independently of JNI. */
final class HighScoresEncodingCompat {
    public static void main(String[] args) {
        require(Arrays.equals(HighScores.encodeCp437("Böb"), new byte[] {0x42, (byte) 0x94, 0x62}),
                "A representable name did not encode to its IBM CP437 bytes.");
        require(Arrays.equals(HighScores.encodeCp437(""), new byte[0]),
                "An empty Pascal shortstring name did not encode as zero bytes.");
        require(HighScores.encodeCp437("😀") == null,
                "An unrepresentable Unicode name was accepted as a CP437 name.");
        require(HighScores.encodeCp437("\ud800") == null,
                "A malformed UTF-16 name was accepted as CP437.");
        require(HighScores.encodeCp437("N".repeat(51)).length == 51,
                "The adapter truncated before shared Pascal TString50 semantics were applied.");
        System.out.println("Android high-score name adapter: CP437 bytes and invalid-character rejection pass");
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }

    private HighScoresEncodingCompat() {}
}
