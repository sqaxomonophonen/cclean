It prints/deletes `.o` files which should be rebuilt according to source
modification times, including those of any local `#include`s (recursively). It
assumes a flat directory structure (no `include/`+`src/`), and that
object→source mapping is simple, i.e. that `foo.o` comes from
`foo.c`, `foo.cc` or `foo.cpp` in the same directory.
It's only ~200 lines of code if you feel like hacking it.
It doesn't have a full featured pre-processor, it just looks for include
lines. So it doesn't understand `#ifdef` or defines which is a problem if you have
optional includes. The `-m` option, which skips "missing include files", may work
for you. (See also `-h` for help).
