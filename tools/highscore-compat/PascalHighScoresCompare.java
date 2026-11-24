package com.kerry.zzt;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;

/** Checks an actual Turbo Pascal typed-file .HI output through the production Android adapter. */
final class PascalHighScoresCompare {
    private static final int ENTRY_COUNT = 30;
    private static final int ENTRY_BYTES = 53;
    private static final int FILE_BYTES = ENTRY_COUNT * ENTRY_BYTES;
    private static final int SCORE_OFFSET = 51;

    public static void main(String[] args) throws Exception {
        if (args.length != 2) throw new IllegalArgumentException("Expected Pascal .HI path and output path.");
        File input = new File(args[0]);
        File output = new File(args[1]);
        byte[] bytes = Files.readAllBytes(input.toPath());
        require(bytes.length == FILE_BYTES, "Turbo Pascal .HI file is not 30 records of 53 bytes.");
        require((bytes[0] & 0xff) == 1 && bytes[1] == 'X', "Pascal short name prefix or length differs.");
        for (int i = 2; i <= 50; i++) {
            require(bytes[i] == 'A', "Pascal shortstring assignment did not preserve stale payload byte " + i + ".");
        }
        require((bytes[SCORE_OFFSET] & 0xff) == 0 && (bytes[SCORE_OFFSET + 1] & 0xff) == 0x80,
                "Signed Integer -32768 is not in little-endian form.");
        int second = ENTRY_BYTES;
        require((bytes[second] & 0xff) == 3 && bytes[second + 1] == 0
                        && (bytes[second + 2] & 0xff) == 0x82 && bytes[second + 3] == 'X',
                "Pascal shortstring did not preserve embedded NUL and CP437 byte 0x82.");
        require((bytes[second + SCORE_OFFSET] & 0xff) == 0xff
                        && (bytes[second + SCORE_OFFSET + 1] & 0xff) == 0x7f,
                "Signed Integer 32767 is not in little-endian form.");

        File roundTrip = new File(output.getPath());
        HighScores scores = HighScores.load(input);
        require(scores.save(roundTrip), "Production Android high-score adapter could not save the Pascal table.");
        require(Arrays.equals(bytes, Files.readAllBytes(roundTrip.toPath())),
                "Production Android high-score adapter did not preserve the Pascal .HI bytes.");
        System.out.println("Turbo Pascal 5.5 HighScoresSave versus production Android adapter: all 1,590 bytes match, including CP437/NUL names, stale name tails, and signed scores.");
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
