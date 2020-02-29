# vios
Experimental PlayStation Vita Emulator

Highly experimental PlayStation Vita emulator and debugger. Written in C++11 and with portability in mind.

# Building
Build with a standard ``make`` command. Set DEBUG=1 for debug build.

Requires Unicorn Engine, pthread, and a C++11 and C99 (for tests) compiler.

Currently builds on Linux only (theoretically it should with other Unices as well). Tests build in the same way as base the project. They currently require gcc-arm-none-eabi, though they'd likely build with clang and some other toolchain.
