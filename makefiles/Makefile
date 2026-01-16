C=g++

CFLAGS=-I
CFLAGS+=-Wall
CFLAGS+=-c


FILES+=Math.cpp
FILES+=Conversions.cpp
FILES+=General.cpp
FILES+=Geometry.cpp

OBJFILES=Conversions.o
OBJFILES+=General.o
OBJFILES+=Geometry.o


AR=ar
ARFLAGS=rcs


Math: $(FILES)
	$(CC) $(CFLAGS) $(FILES)


# Create static libs

lib: $(OBJFILES)
	$(AR) $(ARFLAGS) libmath.a $(OBJFILES)

clean: 
	rm -rf *.o *.a *.out

all: Math lib


