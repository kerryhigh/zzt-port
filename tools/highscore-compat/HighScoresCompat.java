package com.kerry.zzt;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.Random;

/** Checks the Android .HI adapter against the Pascal typed-file record layout. */
final class HighScoresCompat {
    private static final int ENTRY_COUNT = 30;
    private static final int ENTRY_BYTES = 53; // TString50 (51) followed by Integer (2).
    private static final int FILE_BYTES = ENTRY_COUNT * ENTRY_BYTES;
    private static final int SCORE_OFFSET = 51;

    public static void main(String[] args) throws Exception {
        File directory = Files.createTempDirectory("zzt-highscore-compat-").toFile();
        try {
            File missing = new File(directory, "missing.HI");
            HighScores blank = HighScores.load(missing);
            require(blank.insertionPosition(1) == 0, "Missing .HI did not initialize empty scores.");

            File blankFile = new File(directory, "blank.HI");
            require(blank.save(blankFile), "Could not save a new .HI file.");
            byte[] blankBytes = Files.readAllBytes(blankFile.toPath());
            require(blankBytes.length == FILE_BYTES, "New .HI file is not 30 records of 53 bytes.");
            for (int i = 0; i < ENTRY_COUNT; i++) {
                int offset = i * ENTRY_BYTES;
                require(blankBytes[offset] == 0, "Empty Pascal shortstring has a nonzero length.");
                for (int j = 1; j <= 50; j++) {
                    require(blankBytes[offset + j] == 0, "Empty Pascal shortstring has nonzero payload bytes.");
                }
                require((blankBytes[offset + SCORE_OFFSET] & 0xff) == 0xff
                                && (blankBytes[offset + SCORE_OFFSET + 1] & 0xff) == 0xff,
                        "Empty score is not signed Integer -1 in little-endian order.");
            }

            byte[] fixture = blankBytes.clone();
            fixture[0] = 4;
            fixture[1] = (byte) 0x82;
            fixture[2] = (byte) 0xff;
            fixture[3] = 0;
            fixture[4] = 'X';
            fixture[SCORE_OFFSET] = 0;
            fixture[SCORE_OFFSET + 1] = (byte) 0x80; // -32768
            File fixtureFile = new File(directory, "fixture.HI");
            File roundTripFile = new File(directory, "roundtrip.HI");
            Files.write(fixtureFile.toPath(), fixture);
            require(HighScores.load(fixtureFile).save(roundTripFile), "Could not round-trip a .HI fixture.");
            require(Arrays.equals(fixture, Files.readAllBytes(roundTripFile.toPath())),
                    "Unchanged .HI records did not preserve raw bytes.");

            byte[] malformedLengthFixture = fixture.clone();
            malformedLengthFixture[0] = (byte) 0xff;
            File malformedLengthFile = new File(directory, "malformed-length.HI");
            File malformedLengthRoundTrip = new File(directory, "malformed-length-roundtrip.HI");
            Files.write(malformedLengthFile.toPath(), malformedLengthFixture);
            require(HighScores.load(malformedLengthFile).save(malformedLengthRoundTrip),
                    "Could not read a .HI file with a noncanonical shortstring length byte.");
            require(Arrays.equals(malformedLengthFixture, Files.readAllBytes(malformedLengthRoundTrip.toPath())),
                    "A noncanonical Pascal shortstring length byte was normalized during a raw .HI round-trip.");
            String[] malformedLengthRows = HighScores.load(malformedLengthFile).displayRows();
            require(malformedLengthRows.length == 1 && malformedLengthRows[0].length() == 50,
                    "A noncanonical Pascal name length did not respect TTextWindowLine's 50-byte limit.");

            byte[] extendedFixture = Arrays.copyOf(fixture, FILE_BYTES + 7);
            Arrays.fill(extendedFixture, FILE_BYTES, extendedFixture.length, (byte) 0xa5);
            File extendedFile = new File(directory, "extended.HI");
            File extendedRoundTrip = new File(directory, "extended-roundtrip.HI");
            Files.write(extendedFile.toPath(), extendedFixture);
            require(HighScores.load(extendedFile).save(extendedRoundTrip), "Could not read an extended .HI file.");
            require(Arrays.equals(fixture, Files.readAllBytes(extendedRoundTrip.toPath())),
                    "Trailing bytes after Pascal's single typed high-score record were not ignored.");

            HighScores carriedScores = HighScores.load(fixtureFile);
            carriedScores.loadInto(missing);
            File clearedScoresFile = new File(directory, "cleared-after-missing.HI");
            require(carriedScores.save(clearedScoresFile), "Could not save scores after a missing-file reset.");
            byte[] clearedScoresBytes = Files.readAllBytes(clearedScoresFile.toPath());
            byte[] expectedClearedScores = fixture.clone();
            for (int i = 0; i < ENTRY_COUNT; i++) {
                int offset = i * ENTRY_BYTES;
                expectedClearedScores[offset] = 0;
                expectedClearedScores[offset + SCORE_OFFSET] = (byte) 0xff;
                expectedClearedScores[offset + SCORE_OFFSET + 1] = (byte) 0xff;
            }
            require(Arrays.equals(expectedClearedScores, clearedScoresBytes),
                    "Missing .HI reset cleared payload bytes instead of preserving the Pascal global table's stale name data.");

            HighScores inserted = HighScores.load(missing);
            int position = inserted.insertionPosition(500);
            require(position == 0 && inserted.insert(position, 500, "Böb"), "Could not insert a high score.");
            position = inserted.insertionPosition(500);
            require(position == 0 && inserted.insert(position, 500, "T"), "Equal scores did not insert before ties.");
            require(!inserted.insert(0, 1, "😀"), "Unrepresentable text was silently accepted.");

            File insertedFile = new File(directory, "inserted.HI");
            require(inserted.save(insertedFile), "Could not save inserted scores.");
            byte[] insertedBytes = Files.readAllBytes(insertedFile.toPath());
            require(insertedBytes.length == FILE_BYTES, "Inserted .HI file has the wrong size.");
            require(insertedBytes[0] == 1 && insertedBytes[1] == 'T', "Newest tied score was not placed first.");
            require((insertedBytes[SCORE_OFFSET] & 0xff) == 0xf4
                            && (insertedBytes[SCORE_OFFSET + 1] & 0xff) == 0x01,
                    "Score 500 was not written as a little-endian Integer.");
            int second = ENTRY_BYTES;
            require(insertedBytes[second] == 3 && (insertedBytes[second + 1] & 0xff) == 0x42
                            && (insertedBytes[second + 2] & 0xff) == 0x94
                            && (insertedBytes[second + 3] & 0xff) == 0x62,
                    "CP437 player name bytes were not preserved.");
            byte[] placeholder = "-- You! --".getBytes("IBM437");
            require(Arrays.equals(Arrays.copyOfRange(insertedBytes, 2, 11),
                            Arrays.copyOfRange(placeholder, 1, placeholder.length)),
                    "The Pascal placeholder assignment's unused name bytes were not retained.");
            require(Arrays.equals(Arrays.copyOfRange(insertedBytes, second + 4, second + 11),
                            Arrays.copyOfRange(placeholder, 3, placeholder.length)),
                    "The shifted player's unused name bytes changed during insertion.");

            verifyInsertionsAgainstPascalModel(directory);

            char[] overlongNameChars = new char[51];
            Arrays.fill(overlongNameChars, 'N');
            HighScores truncatedName = HighScores.load(missing);
            require(truncatedName.insert(0, 2, new String(overlongNameChars)),
                    "A Pascal TString50 high-score name was rejected instead of truncated.");
            File truncatedNameFile = new File(directory, "truncated-name.HI");
            require(truncatedName.save(truncatedNameFile), "Could not save a truncated high-score name.");
            byte[] truncatedNameBytes = Files.readAllBytes(truncatedNameFile.toPath());
            require(truncatedNameBytes[0] == 50, "An overlong high-score name was not truncated to 50 bytes.");
            for (int i = 1; i <= 50; i++) {
                require(truncatedNameBytes[i] == 'N', "A truncated high-score name has incorrect CP437 bytes.");
            }
            String[] truncatedDisplayRows = truncatedName.displayRows();
            String expectedDisplayRow = String.format(java.util.Locale.ROOT, "%5d  ", 2)
                    + "N".repeat(43);
            require(truncatedDisplayRows.length == 1
                            && truncatedDisplayRows[0].equals(expectedDisplayRow),
                    "A high-score display row did not truncate to Pascal's 50-byte TTextWindowLine.");

            File shortFile = new File(directory, "short.HI");
            Files.write(shortFile.toPath(), Arrays.copyOf(fixture, FILE_BYTES - 1));
            File shortRoundTrip = new File(directory, "short-roundtrip.HI");
            require(HighScores.load(shortFile).save(shortRoundTrip), "Short .HI file did not reset to defaults.");
            require(Arrays.equals(blankBytes, Files.readAllBytes(shortRoundTrip.toPath())),
                    "Short .HI file did not reset all 30 records like Pascal.");

            HighScores reusedShort = HighScores.load(fixtureFile);
            reusedShort.loadInto(shortFile);
            File reusedShortRoundTrip = new File(directory, "short-reused-roundtrip.HI");
            require(reusedShort.save(reusedShortRoundTrip), "Could not save scores after a short-file reset.");
            require(Arrays.equals(expectedClearedScores, Files.readAllBytes(reusedShortRoundTrip.toPath())),
                    "Short .HI reset cleared payload bytes instead of preserving the Pascal global table's stale name data.");

            System.out.println(".HI records: 30 x (51-byte TString50 + 16-bit Integer) = 1,590 bytes");
            System.out.println(".HI behavior: raw-byte round-trip, raw noncanonical lengths, ignored trailing bytes, signed little-endian scores, CP437 encoding, 50-byte name and display-line truncation, tied-score order, placeholder tails, reused-table reset, and short-file reset pass");
            System.out.println(".HI insertion: 256 full-table outputs match the Pascal HighScoresAdd source model, including tie placement and preserved shortstring tails");
        } finally {
            File[] files = directory.listFiles();
            if (files != null) for (File file : files) file.delete();
            directory.delete();
        }
    }

    private static void verifyInsertionsAgainstPascalModel(File directory) throws Exception {
        final int trials = 256;
        final byte[] placeholder = "-- You! --".getBytes("IBM437");
        final byte[] lengths = {0, 1, 9, 34, 50, 51, 127, (byte) 0xff};
        final String[] names = {"", "T", "Böb", "Kerry 49", "N".repeat(55)};
        final int[] scores = {Short.MIN_VALUE, -1, 0, 1, 2, 500, Short.MAX_VALUE};
        Random random = new Random(0x5a17c9e3L);
        File input = new File(directory, "model-input.HI");
        File output = new File(directory, "model-output.HI");

        for (int trial = 0; trial < trials; trial++) {
            byte[] fixture = new byte[FILE_BYTES];
            random.nextBytes(fixture);
            for (int i = 0; i < ENTRY_COUNT; i++) {
                int offset = i * ENTRY_BYTES;
                fixture[offset] = lengths[(trial + i) % lengths.length];
                int score = random.nextInt(65536) - 32768;
                writeScore(fixture, offset, score);
            }

            int score = trial < scores.length ? scores[trial] : random.nextInt(32768);
            String name = names[trial % names.length];
            int expectedPosition = pascalInsertionPosition(fixture, score);
            int actualPosition = HighScores.load(write(fixture, input)).insertionPosition(score);
            require(actualPosition == expectedPosition,
                    "HighScoresAdd insertion position differed from Pascal on trial " + trial + ".");

            byte[] expected = fixture.clone();
            if (expectedPosition >= 0) {
                pascalInsert(expected, expectedPosition, score, placeholder, name.getBytes("IBM437"));
                HighScores actual = HighScores.load(input);
                require(actual.insert(actualPosition, score, name),
                        "Android rejected a CP437 Pascal high-score name on trial " + trial + ".");
                require(actual.save(output), "Could not save a modeled insertion on trial " + trial + ".");
            } else {
                HighScores actual = HighScores.load(input);
                require(actual.save(output), "Could not save an unchanged table on trial " + trial + ".");
            }
            require(Arrays.equals(expected, Files.readAllBytes(output.toPath())),
                    "Full high-score table differed from Pascal's insertion model on trial " + trial + ".");
        }
    }

    private static File write(byte[] bytes, File file) throws Exception {
        Files.write(file.toPath(), bytes);
        return file;
    }

    private static int pascalInsertionPosition(byte[] table, int score) {
        int position = 0;
        while (position < ENTRY_COUNT && score < readScore(table, position * ENTRY_BYTES)) position++;
        return score > 0 && position < ENTRY_COUNT ? position : -1;
    }

    private static void pascalInsert(byte[] table, int position, int score,
                                     byte[] placeholder, byte[] name) {
        for (int i = ENTRY_COUNT - 1; i > position; i--) {
            System.arraycopy(table, (i - 1) * ENTRY_BYTES, table, i * ENTRY_BYTES, ENTRY_BYTES);
        }
        int offset = position * ENTRY_BYTES;
        writeScore(table, offset, score);
        assignShortString(table, offset, placeholder);
        assignShortString(table, offset, name);
    }

    private static int readScore(byte[] table, int offset) {
        return (short) ((table[offset + SCORE_OFFSET] & 0xff)
                | ((table[offset + SCORE_OFFSET + 1] & 0xff) << 8));
    }

    private static void writeScore(byte[] table, int offset, int score) {
        short packed = (short) score;
        table[offset + SCORE_OFFSET] = (byte) (packed & 0xff);
        table[offset + SCORE_OFFSET + 1] = (byte) ((packed >> 8) & 0xff);
    }

    private static void assignShortString(byte[] table, int offset, byte[] value) {
        int length = Math.min(50, value.length);
        table[offset] = (byte) length;
        System.arraycopy(value, 0, table, offset + 1, length);
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new IllegalStateException(message);
    }

    private HighScoresCompat() {}
}
