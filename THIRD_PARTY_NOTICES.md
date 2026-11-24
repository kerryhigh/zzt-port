# Third-party notices

## Gradle Wrapper

`gradle/wrapper/gradle-wrapper.jar` is the Gradle Wrapper bootstrap JAR, distributed under the Apache License 2.0. The wrapper downloads the pinned Gradle 8.11.1 distribution from `services.gradle.org`; the distribution and Android Gradle Plugin are external build dependencies and are not vendored here.

## Reconstruction of ZZT

`third_party/reconstruction-of-zzt` is Adrian Siekierka's reverse-engineered Pascal reconstruction of ZZT 3.2. It is MIT-licensed under the terms in `third_party/reconstruction-of-zzt/LICENSE.TXT`, with the exception for the separately documented LZEXE files. The Android app bundles `RES/DEMO.ZZT` and the original `DOC/*.HLP` help files from this repository.

## libzoo

`third_party/libzoo` is Adrian Siekierka's portable C reimplementation based on the ZZT reconstruction. Its MIT license is in `third_party/libzoo/LICENSE`. The Android and Windows frontends share its game logic and world I/O. The `.BRD` codec is now part of this shared source tree. This checkout has local compatibility edits across state, game, element, I/O, input, OOP, sound, and UI code. These preserve Pascal short-string tails, `TStat` pointer/reserved bytes, board padding, world-header bytes, legacy-header offsets, Pascal input and tick behavior, and 16-bit arithmetic; the Android PCM adapter also retains Pascal's immediate movement-sound pulse.

`app/src/main/cpp/zzt_sound_pcm.c` adapts libzoo's MIT-licensed PCM speaker driver for Android's callback type; the upstream driver copy remains unchanged.

`app/src/main/assets/DEMO.ZZT` and `*.HLP` files are from the reconstruction repository and are included under its source-release license and attribution terms. They are not relicensed by the root project license.

Upstream repositories:

- https://github.com/asiekierka/reconstruction-of-zzt
- https://github.com/asiekierka/libzoo
