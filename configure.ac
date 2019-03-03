AC_INIT([zxspectrum-utils], [1.0], [https://sourceforge.net/projects/zxspectrumutils/])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O

libjpeg_msg="no"
libpng_msg="no"
have_x="no"

AC_PATH_XTRA
AC_SEARCH_LIBS([pow],[m])
#AC_SEARCH_LIBS([XOpenDisplay],[X11],[have_x="yes"])
AC_SEARCH_LIBS([jpeg_start_decompress],[jpeg],[libjpeg_msg="yes"])
AC_SEARCH_LIBS([png_create_write_struct_2],[png],[libpng_msg="yes"])

HAVE_DITHVIDEO="no"
AS_IF([test "x$libjpeg_msg" = "xyes"], [
       AS_IF([test "x$have_x" = "xyes"], [
	      HAVE_DITHVIDEO="yes"
	])
])
AM_CONDITIONAL([COMPILE_DITHVIDEO], [test "x$HAVE_DITHVIDEO" = "xyes"])
AM_CONDITIONAL([COMPILE_DIVIDEOGEN], [test "x$libpng_msg" = "xyes"])

AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strerror])
AC_CHECK_HEADERS([fcntl.h])
AC_FUNC_MALLOC
AC_TYPE_SIZE_T

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 man/Makefile
])
AC_OUTPUT
