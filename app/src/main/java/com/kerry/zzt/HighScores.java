package com.kerry.zzt;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** The original ZZT .HI file: 30 packed entries of 51-byte Pascal string + int16 score. */
final class HighScores {
    private static final int ENTRY_COUNT = 30;
    private static final int NAME_BYTES = 50;
    private static final int ENTRY_BYTES = 53;
    private static final int FILE_BYTES = ENTRY_COUNT * ENTRY_BYTES;
    private static final int SCORE_OFFSET = 51;
    private static final int DISPLAY_LINE_BYTES = 50; // TxtWind.TTextWindowLine = string[50].
    private static final Charset CP437 = Charset.forName("IBM437");
    private static final byte[] NEW_SCORE_PLACEHOLDER = "-- You! --".getBytes(CP437);

    private final byte[][] entries = new byte[ENTRY_COUNT][ENTRY_BYTES];

    HighScores() {
        for (byte[] entry : entries) setScore(entry, -1);
    }

    static HighScores load(File file) {
        HighScores result = new HighScores();
        result.loadInto(file);
        return result;
    }

    void loadInto(File file) {
        if (file == null || !file.isFile() || file.length() < FILE_BYTES) {
            resetAfterLoadError();
            return;
        }
        byte[] contents = new byte[FILE_BYTES];
        try (DataInputStream input = new DataInputStream(new FileInputStream(file))) {
            input.readFully(contents);
            for (int i = 0; i < ENTRY_COUNT; i++) {
                System.arraycopy(contents, i * ENTRY_BYTES, entries[i], 0, ENTRY_BYTES);
            }
        } catch (IOException ignored) {
            resetAfterLoadError();
        }
    }

    private void resetAfterLoadError() {
        for (byte[] entry : entries) {
            entry[0] = 0;
            setScore(entry, -1);
        }
    }

    int insertionPosition(int score) {
        int position = 0;
        while (position < ENTRY_COUNT && score < scoreAt(position)) position++;
        return score > 0 && position < ENTRY_COUNT ? position : -1;
    }

    boolean insert(int position, int score, String name) {
        if (position < 0 || position >= ENTRY_COUNT) return false;
        byte[] encodedName = encodeCp437(name == null ? "" : name);
        if (encodedName == null) return false;
        for (int i = ENTRY_COUNT - 1; i > position; i--) {
            System.arraycopy(entries[i - 1], 0, entries[i], 0, ENTRY_BYTES);
        }
        setScore(entries[position], score);
        // Pascal assigns this placeholder before opening the name prompt, then
        // assigns the entered shortstring over it. Keep both writes so bytes
        // beyond the final string length retain the same prior contents.
        setName(entries[position], NEW_SCORE_PLACEHOLDER);
        setName(entries[position], encodedName);
        return true;
    }

    boolean hasNamedEntries() {
        for (byte[] entry : entries) {
            if (nameLength(entry) > 0) return true;
        }
        return false;
    }

    String[] displayRows() {
        List<String> rows = new ArrayList<>();
        for (byte[] entry : entries) {
            int length = nameLength(entry);
            if (length > 0) {
                String name = new String(entry, 1, length, CP437);
                String row = String.format(Locale.ROOT, "%5d  %s", score(entry), name);
                rows.add(row.length() > DISPLAY_LINE_BYTES
                        ? row.substring(0, DISPLAY_LINE_BYTES) : row);
            }
        }
        return rows.toArray(new String[0]);
    }

    boolean save(File file) {
        if (file == null) return false;
        try (DataOutputStream output = new DataOutputStream(new FileOutputStream(file))) {
            for (byte[] entry : entries) output.write(entry);
            return true;
        } catch (IOException ignored) {
            return false;
        }
    }

    private static int nameLength(byte[] entry) {
        return Math.min(entry[0] & 0xff, NAME_BYTES);
    }

    private static int score(byte[] entry) {
        return (short) ((entry[SCORE_OFFSET] & 0xff) | ((entry[SCORE_OFFSET + 1] & 0xff) << 8));
    }

    private int scoreAt(int index) {
        return score(entries[index]);
    }

    private static void setScore(byte[] entry, int score) {
        short packed = (short) score;
        entry[SCORE_OFFSET] = (byte) (packed & 0xff);
        entry[SCORE_OFFSET + 1] = (byte) ((packed >> 8) & 0xff);
    }

    private static byte[] encodeCp437(String name) {
        try {
            ByteBuffer encoded = CP437.newEncoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .encode(CharBuffer.wrap(name));
            byte[] bytes = new byte[encoded.remaining()];
            encoded.get(bytes);
            return bytes;
        } catch (CharacterCodingException e) {
            return null;
        }
    }

    private static void setName(byte[] entry, byte[] encoded) {
        int length = Math.min(encoded.length, NAME_BYTES);
        entry[0] = (byte) length;
        System.arraycopy(encoded, 0, entry, 1, length);
    }
}
