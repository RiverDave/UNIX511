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

------------------------------------------------------------------------

### Shared Libs

1. The main differentiator is that we generate our objfiles with: `-fPIC` eg:

```bash
gcc -fPIC -c add.c multiply.c

```
*-fPIC* : Position-Independent Code (PIC), This is intrinsic to the fact that the linker can dynamically call externally defined library code.
This is important given that since each program has a different memory layout.


2. Create the shared lib in the following manner:

```bash
gcc -shared -o {LIB_NAME} {SRC_FILES}

```
for instance:

``` bash

gcc -shared -o libmath.dylib add.o multiply.o

```
note that I'm using `.dylib` as an extension since that's the library file extension in macos, **for linux:** use `.so`


3. Compile src files with directive to look for libs: 

```bash
gcc -o program main.c -L. -lmath

```

4. Include the current directory so the runtime linker can find the shared library:

```bash
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

```
(For some reason I was able to run the program before doing the above mentioned export, might need to look into that further.)





