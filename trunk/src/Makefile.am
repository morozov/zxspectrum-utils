bin_PROGRAMS = 0tobin 0tohob 0totap bin2mbd bin2tap binto0 breplace createtrd \
	       d802tap dir0 dirhob dirtap hobeta2trd hobto0 lstbas lstrd \
	       makesna mb2tap mbdcheck mbdir mbdidfix mbdirsec mbload mbdnew \
	       mbrokfix permutor tap2d80 tap2mbd tap2mbhdd \
	       tap2tzx tapto0 tsttap tzx2tap 
if COMPILE_DITHVIDEO
bin_PROGRAMS += dithvide
endif
if COMPILE_DIVIDEOGEN
bin_PROGRAMS += divideogen
endif

0tobin_SOURCES = 0tobin.c endian-compat.h
0tohob_SOURCES = 0tohob.c endian-compat.h
0totap_SOURCES = 0totap.c endian-compat.h
bin2mbd_SOURCES = bin2mbd.c
bin2tap_SOURCES = bin2tap.c
binto0_SOURCES = binto0.c
breplace_SOURCES = breplace.c
createtrd_SOURCES = createtrd.c trdos_structure.h
d802tap_SOURCES = d802tap.cpp
dir0_SOURCES = dir0.c endian-compat.h
dirhob_SOURCES = dirhob.c endian-compat.h
dirtap_SOURCES = dirtap.c endian-compat.h

if COMPILE_DITHVIDEO
dithvide_SOURCES = dithvide.c
dithvide_CFLAGS = @X_CFLAGS@
dithvide_LDADD=@X_LIBS@ -lX11
endif

if COMPILE_DIVIDEOGEN
divideogen_SOURCES = divideogen.c
endif

hobeta2trd_SOURCES = hobeta2trd.c
hobto0_SOURCES = hobto0.c endian-compat.h
lstbas_SOURCES = lstbas.c endian-compat.h
lstrd_SOURCES = lstrd.c
makesna_SOURCES = makesna.cpp
mb2tap_SOURCES = mb2tap.c endian-compat.h
mbdir_SOURCES = mbdir.c endian-compat.h
mbload_SOURCES = mbload.c endian-compat.h
permutor_SOURCES = permutor.c
tap2d80_SOURCES = tap2d80.cpp
tap2mbd_SOURCES = tap2mbd.cpp
tap2mbhdd_SOURCES = tap2mbhdd.cpp
tap2tzx_SOURCES = tap2tzx.c
tapto0_SOURCES = tapto0.c endian-compat.h
tsttap_SOURCES = tsttap.c endian-compat.h
tzx2tap_SOURCES = tzx2tap.c
