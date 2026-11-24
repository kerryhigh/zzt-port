#ifndef ZZT_RUNTIME_COMPAT_MATH_H
#define ZZT_RUNTIME_COMPAT_MATH_H

/* The timing-only WASM harness uses the Android double-precision clock
   configuration but does not link zoo.c's ceil-based conversion helpers. */
double ceil(double value);

#endif
