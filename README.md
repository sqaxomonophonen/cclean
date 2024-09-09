It prints/deletes `.o` files which should be rebuilt according to source
modification times, including those of any local `#include`s (recursively). It
assumes a flat directory structure (no `include/`+`src/`), and that
object→source mapping is simple, i.e. that `foo.o` comes from
`foo.c`, `foo.cc` or `foo.cpp` in the same directory.
It's only ~200 lines of code if you feel like hacking it.
