package com.kerry.zzt;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;

/** Compares Android .HI load/reset behavior with a direct Turbo Pascal HighScoresLoad capture. */
final class PascalHighScoresLoadCompare {
    private static final String[] CASES = {
            "HIMISS", "HIEMPT", "HIC001", "HIC052", "HIC053", "HIC1589", "HIFULL", "HILONG"
    };

    public static void main(String[] args) throws Exception {
        if (args.length != 1) throw new IllegalArgumentException("Expected the Pascal fixture directory.");
        File directory = new File(args[0]);
        File seedFile = new File(directory, "HISEED.HI");
        byte[] seed = Files.readAllBytes(seedFile.toPath());
        require(seed.length == 30 * 53, "The Pascal load seed is not a complete .HI table.");

        for (String caseName : CASES) {
            HighScores scores = HighScores.load(seedFile);
            File input = new File(directory, caseName + ".HI");
            File output = new File(directory, "PROD-" + caseName + ".HI");
            scores.loadInto(input);
            require(scores.save(output), caseName + " production .HI load result could not be saved.");

            byte[] expected = Files.readAllBytes(new File(directory, caseName + ".OUT").toPath());
            byte[] actual = Files.readAllBytes(output.toPath());
            require(expected.length == 30 * 53, caseName + " Pascal output has the wrong record size.");
            require(Arrays.equals(expected, actual), caseName
                    + " full .HI table differs after HighScoresLoad (including preserved name payload tails).");
            System.out.println("Turbo Pascal HighScoresLoad " + caseName + ": all 1,590 bytes match production");
        }
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
