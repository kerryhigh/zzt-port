package com.kerry.zzt;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CodingErrorAction;

/** Maps Android Unicode text events to the key values used by ZZT's DOS input code. */
final class DosKeyboardMapping {
    private static final int CP437_KEY_BASE = 0x0200;
    private static final Charset CP437 = Charset.forName("IBM437");

    private DosKeyboardMapping() {}

    static int fromUnicode(int codePoint) {
        if (!Character.isValidCodePoint(codePoint) || codePoint < 32) return 0;
        ByteBuffer encoded;
        try {
            encoded = CP437.newEncoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .encode(CharBuffer.wrap(Character.toChars(codePoint)));
        } catch (CharacterCodingException failure) {
            return 0;
        }
        if (encoded.remaining() != 1) return 0;
        int value = encoded.get() & 0xff;
        return value == 0 ? 0 : keyForByte(value);
    }

    static int keyForByte(int value) {
        if (value < 0 || value > 0xff) return 0;
        return value < 127 ? value : CP437_KEY_BASE + value;
    }
}
