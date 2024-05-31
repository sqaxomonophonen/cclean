It prints `.o` files which should be rebuilt according source modification
times, including those of any local `#includes` (recursively). It assumes
`foo.o` comes from `foo.c`, `foo.c` or `foo.cpp`.

You can typically use it like this:

`cclean | xargs -r rm -v && make`
