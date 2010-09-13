all: library all_samples 

CFLAGS=-O4  -I. -Wall -Wno-sign-compare

# OpenMP support
#CFLAGS=-O4  -I. -w -fopenmp

# LCMS support
#LCMS_DEF=-DUSE_LCMS -I/usr/local/include
#LCMS_LIB=-L/usr/local/lib -llcms


DCRAW_LIB_OBJECTS=object/dcraw_common.o object/libraw_cxx.o object/libraw_c_api.o object/dcraw_fileio.o
DCRAW_LIB_MT_OBJECTS=object/dcraw_common_mt.o object/libraw_cxx_mt.o object/libraw_c_api_mt.o object/dcraw_fileio_mt.o

library: lib/libraw.a lib/libraw_r.a

all_samples: bin/raw-identify bin/simple_dcraw  bin/dcraw_emu bin/dcraw_half bin/half_mt bin/mem_image bin/unprocessed_raw bin/4channels

install: library
	@if [ -d /usr/local/include ] ; then cp -R libraw /usr/local/include/ ; else echo 'no /usr/local/include' ; fi
	@if [ -d /usr/local/lib ] ; then cp lib/libraw.a lib/libraw_r.a /usr/local/lib/ ; else echo 'no /usr/local/lib' ; fi

install-binaries: all_samples
	@if [ -d /usr/local/bin ] ; then cp bin/[a-z]* /usr/local/bin/ ; else echo 'no /usr/local/bin' ; fi


#binaries

bin/raw-identify: lib/libraw.a samples/raw-identify.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/raw-identify samples/raw-identify.cpp -L./lib -lraw  -lm ${LCMS_LIB}

bin/unprocessed_raw: lib/libraw.a samples/unprocessed_raw.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/unprocessed_raw samples/unprocessed_raw.cpp -L./lib -lraw  -lm  ${LCMS_LIB}

bin/4channels: lib/libraw.a samples/4channels.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/4channels samples/4channels.cpp -L./lib -lraw  -lm  ${LCMS_LIB}

bin/simple_dcraw: lib/libraw.a samples/simple_dcraw.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF}  ${CFLAGS} -o bin/simple_dcraw samples/simple_dcraw.cpp -L./lib -lraw  -lm  ${LCMS_LIB}

bin/mem_image: lib/libraw.a samples/mem_image.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/mem_image samples/mem_image.cpp -L./lib -lraw  -lm  ${LCMS_LIB}

bin/dcraw_half: lib/libraw.a object/dcraw_half.o
	gcc -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/dcraw_half object/dcraw_half.o -L./lib -lraw  -lm -lstdc++  ${LCMS_LIB}

bin/half_mt: lib/libraw_r.a object/half_mt.o
	gcc ${LCMS_DEF}  -pthread ${CFLAGS} -o bin/half_mt object/half_mt.o -L./lib -lraw_r  -lm -lstdc++  ${LCMS_LIB}

bin/dcraw_emu: lib/libraw.a samples/dcraw_emu.cpp
	g++ -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o bin/dcraw_emu samples/dcraw_emu.cpp -L./lib -lraw  -lm  ${LCMS_LIB}

#objects

object/dcraw_common.o: internal/dcraw_common.cpp internal/dcraw_cbrt.cpp
	g++ -c -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o object/dcraw_common.o internal/dcraw_common.cpp

object/dcraw_fileio.o: internal/dcraw_fileio.cpp
	g++ -c -DLIBRAW_NOTHREADS ${CFLAGS} ${LCMS_DEF} -o object/dcraw_fileio.o internal/dcraw_fileio.cpp

object/libraw_cxx.o: src/libraw_cxx.cpp
	g++ -c -DLIBRAW_NOTHREADS ${LCMS_DEF} ${CFLAGS} -o object/libraw_cxx.o src/libraw_cxx.cpp

object/libraw_c_api.o: src/libraw_c_api.cpp
	g++ -c -DLIBRAW_NOTHREADS  ${LCMS_DEF} ${CFLAGS} -o object/libraw_c_api.o src/libraw_c_api.cpp

object/dcraw_half.o: samples/dcraw_half.c
	gcc -c -DLIBRAW_NOTHREADS  ${LCMS_DEF} ${CFLAGS} -o object/dcraw_half.o samples/dcraw_half.c

object/half_mt.o: samples/half_mt.c
	gcc -c -pthread ${LCMS_DEF}  ${CFLAGS} -o object/half_mt.o samples/half_mt.c


lib/libraw.a: ${DCRAW_LIB_OBJECTS}
	rm -f lib/libraw.a
	ar crv lib/libraw.a ${DCRAW_LIB_OBJECTS}
	ranlib lib/libraw.a

lib/libraw_r.a: ${DCRAW_LIB_MT_OBJECTS}
	rm -f lib/libraw_r.a
	ar crv lib/libraw_r.a ${DCRAW_LIB_MT_OBJECTS}
	ranlib lib/libraw_r.a

object/dcraw_common_mt.o: internal/dcraw_common.cpp
	g++ -c -pthread ${LCMS_DEF}  ${CFLAGS} -o object/dcraw_common_mt.o internal/dcraw_common.cpp

object/dcraw_fileio_mt.o: internal/dcraw_fileio.cpp
	g++ -c -pthread ${LCMS_DEF} ${CFLAGS} -o object/dcraw_fileio_mt.o internal/dcraw_fileio.cpp

object/libraw_cxx_mt.o: src/libraw_cxx.cpp
	g++ -c ${LCMS_DEF}  -pthread ${CFLAGS} -o object/libraw_cxx_mt.o src/libraw_cxx.cpp

object/libraw_c_api_mt.o: src/libraw_c_api.cpp
	g++ -c ${LCMS_DEF}  -pthread ${CFLAGS} -o object/libraw_c_api_mt.o src/libraw_c_api.cpp

internal/dcraw_cbrt.cpp: internal/gen_dcraw_cbrt.py
	python internal/gen_dcraw_cbrt.py > internal/dcraw_cbrt.cpp

clean:
	rm -fr bin/*.dSYM
	rm -f *.o *~ src/*~ samples/*~ internal/*~ libraw/*~ lib/lib*.a bin/[4a-z]* object/*o dcraw/*~ doc/*~ bin/*~

