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

/** Android file and CP437 adapter for the shared original ZZT .HI table semantics. */
final class HighScores {
    private static final int ENTRY_COUNT = 30;
    private static final int DISPLAY_LINE_BYTES = 50; // TxtWind.TTextWindowLine = string[50].
    private static final Charset CP437 = Charset.forName("IBM437");
    private static final int FILE_BYTES = ENTRY_COUNT * 53;

    private final byte[] table = new byte[FILE_BYTES];

    HighScores() {
        ZztNative.highScoresInitialize(table);
    }

    void loadInto(File file) {
        byte[] contents = null;
        if (file != null && file.isFile() && file.length() >= FILE_BYTES) {
            try (DataInputStream input = new DataInputStream(new FileInputStream(file))) {
                contents = new byte[FILE_BYTES];
                input.readFully(contents);
            } catch (IOException ignored) {
                contents = null;
            }
        }
        ZztNative.highScoresLoad(table, contents);
    }

    int insertionPosition(int score) {
        return ZztNative.highScoresInsertionPosition(table, score);
    }

    boolean insert(int position, int score, String name) {
        byte[] encodedName = encodeCp437(name == null ? "" : name);
        return encodedName != null
                && ZztNative.highScoresInsert(table, position, score, encodedName) == 0;
    }

    boolean hasNamedEntries() {
        for (int i = 0; i < ENTRY_COUNT; i++) {
            if (ZztNative.highScoreNameLength(table, i) > 0) return true;
        }
        return false;
    }

    String[] displayRows() {
        List<String> rows = new ArrayList<>();
        for (int i = 0; i < ENTRY_COUNT; i++) {
            int length = ZztNative.highScoreNameLength(table, i);
            if (length > 0) {
                byte[] nameBytes = ZztNative.highScoreNameBytes(table, i);
                String name = nameBytes == null ? "" : new String(nameBytes, CP437);
                String row = String.format(Locale.ROOT, "%5d  %s",
                        ZztNative.highScoreValue(table, i), name);
                rows.add(row.length() > DISPLAY_LINE_BYTES
                        ? row.substring(0, DISPLAY_LINE_BYTES) : row);
            }
        }
        return rows.toArray(new String[0]);
    }

    boolean save(File file) {
        if (file == null) return false;
        byte[] record = ZztNative.highScoresSerialize(table);
        if (record == null || record.length != FILE_BYTES) return false;
        try (DataOutputStream output = new DataOutputStream(new FileOutputStream(file))) {
            output.write(record);
            return true;
        } catch (IOException ignored) {
            return false;
        }
    }

    static byte[] encodeCp437(String name) {
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

}
