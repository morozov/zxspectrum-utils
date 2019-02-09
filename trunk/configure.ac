AC_INIT([zxspectrum-utils], [1.0], [https://sourceforge.net/projects/zxspectrumutils/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_CXX

LIBJPEG=""
libjpeg_msg="no"
AC_SUBST(LIBJPEG)
AC_CHECK_LIB(jpeg,jpeg_start_decompress,[
	AC_CHECK_HEADER(jpeglib.h,[
		AC_DEFINE(HAVE_LIBJPEG,1,[define if we found LIBJPEG and its headers])
			LIBJPEG="-ljpeg"
			libjpeg_msg="yes"
		])
])

AC_PATH_XTRA
HAVE_DITHVIDEO="no"
AS_IF([test "x$libjpeg_msg" = "xyes"], [
       AS_IF([test "x$have_x" = "xyes"], [
	      HAVE_DITHVIDEO="yes"
	])
])
AM_CONDITIONAL([COMPILE_DITHVIDEO], [test "x$HAVE_DITHVIDEO" = "xyes"])

LIBPNG=""
libpng_msg="no"
AC_SUBST(LIBPNG)
AC_CHECK_LIB(png,png_create_write_struct_2,[
	AC_CHECK_HEADER(png.h,[
		AC_DEFINE(HAVE_LIBPNG,1,[define if we found LIBPNG and its headers])
			LIBPNG="-lpng"
			libpng_msg="yes"
		])
])
AM_CONDITIONAL([COMPILE_DIVIDEOGEN], [test "x$libpng_msg" = "xyes"])


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
