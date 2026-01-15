### Static Libs

1. get object files out of source files:

```bash

gcc -c add.c multiply.c

```

2. Create static libs

```bash

ar rcs {LIB_NAME} {SRC_FILES}
# eg:
ar rcs libmath.a add.o multiply.o


```
  Also verify contents with: `ar t libmath.a`


3. Compile with Translation unit with main program that makes use of our libs eg:


``` bash
gcc -o program main.c -L. -lmath

```

-L.: Tells the compiler to search for libraries in the current directory.
-lmath: Links the program to libmath.a (without the lib prefix and .a extension).

  Or express the library manually and link against it:

``` bash
gcc -o program main.c libmath.a

```
