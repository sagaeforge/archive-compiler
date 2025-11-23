```bash
nasm -f macho64 fibonacci.asm -o fibonacci.o
clang -arch x86_64 fibonacci.o -o fibonacci -Wl,-e,_start
```