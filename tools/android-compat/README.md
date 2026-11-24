# Android renderer compatibility checks

Run the host-side 60×25 cell raster checks with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\android-compat\verify.ps1
```

The check compiles the Android renderer and key mapper's pure Java paths with the JDK; it does not require an emulator. Synthetic glyph rows make all 256 CP437 byte indices and every color attribute observable. It also checks both DOS blink phases, all 256 monochrome attribute mappings from the reconstructed `VIDEO.PAS`, and the Unicode-to-CP437 key conversion for every round-trippable byte plus unmappable characters. The expected extended-key base is read from libzoo's `ZOO_KEY_CP437_BASE`, so a frontend/engine constant mismatch fails the check. The app still draws its same 60×25 board into the same 8×14 bitmap and uses the same nearest-neighbor Canvas paint.
