package com.kerry.zzt;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;

/** Compares production Android insertion with bytes emitted by compiled Pascal HighScoresAdd. */
final class PascalHighScoresInsertionCompare {
    private static final int FILE_BYTES = 30 * 53;

    public static void main(String[] args) throws Exception {
        if (args.length != 6) {
            throw new IllegalArgumentException(
                    "Expected case name, Pascal input/output paths, score, entered name, and expected path.");
        }
        String caseName = args[0];
        File input = new File(args[1]);
        File pascalOutput = new File(args[2]);
        int score = Integer.parseInt(args[3]);
        String enteredName = args[4].equals("-") ? "" : args[4];
        File expectedOutput = new File(args[5]);

        byte[] inputBytes = Files.readAllBytes(input.toPath());
        byte[] pascalBytes = Files.readAllBytes(pascalOutput.toPath());
        require(inputBytes.length == FILE_BYTES,
                caseName + " Pascal input is not a 1,590-byte .HI table.");
        require(pascalBytes.length == FILE_BYTES,
                caseName + " HighScoresAdd output is not a 1,590-byte .HI table.");

        HighScores scores = HighScores.load(input);
        int insertionPosition = scores.insertionPosition(score);
        if (insertionPosition >= 0) {
            require(scores.insert(insertionPosition, score, enteredName),
                    caseName + " production adapter rejected the Pascal name input.");
        }
        require(scores.save(expectedOutput),
                caseName + " production adapter could not write its expected table.");
        byte[] expectedBytes = Files.readAllBytes(expectedOutput.toPath());
        require(Arrays.equals(pascalBytes, expectedBytes),
                caseName + " full .HI table differs from compiled Pascal HighScoresAdd.");

        String result = insertionPosition >= 0
                ? "inserted at rank " + (insertionPosition + 1)
                : "left table unchanged";
        System.out.println("Turbo Pascal HighScoresAdd " + caseName + ": " + result
                + "; all 1,590 output bytes match production Android adapter");
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
