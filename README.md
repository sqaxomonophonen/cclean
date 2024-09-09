It prints/deletes `.o` files which should be rebuilt according to source
modification times, including those of any local `#includes` (recursively). It
assumes `foo.o` comes from `foo.c`, `foo.cc` or `foo.cpp`.
