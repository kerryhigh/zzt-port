package com.kerry.zzt;

/** Attribute conversion used by ZZT's monochrome DOS video writer. */
final class VideoAttributes {
    private VideoAttributes() {}

    static int forDisplay(int attribute, boolean monochrome) {
        int color = attribute & 0xff;
        if (!monochrome) return color;

        // Matches VIDEO.PAS VideoWriteTextBW80: collapse VGA color attributes
        // to the four intensities available on the original monochrome display.
        if ((color & 0x08) != 0) {
            return (color & 0xf0) == 0 ? 0x0f : 0x7f;
        }
        return (color & 0x07) != 0 ? 0x07 : 0x70;
    }
}
