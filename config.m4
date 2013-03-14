PHP_ARG_ENABLE(proj4, whether to enable Proj.4 support,
[ --enable-proj4   Enable Proj.4 support])

if test "$PHP_PROJ4" = "yes"; then
  PHP_ADD_LIBPATH(/usr/lib)
  AC_DEFINE(HAVE_PROJ4, 1, [Whether you have Proj.4])
  PHP_NEW_EXTENSION(proj4, proj4.c, $ext_shared)
  PHP_SUBST(PROJ4_SHARED_LIBADD)
  PHP_ADD_LIBRARY(proj, 1, PROJ4_SHARED_LIBADD)
fi
