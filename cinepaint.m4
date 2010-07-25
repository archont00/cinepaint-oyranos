# Configure paths for CINEPAINT
# Manish Singh    98-6-11
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_CINEPAINT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for CINEPAINT, and define CINEPAINT_CFLAGS and CINEPAINT_LIBS
dnl
AC_DEFUN([AM_PATH_CINEPAINT],
[dnl 
dnl Get the cflags and libraries from the cinepainttool script
dnl
AC_ARG_WITH(cinepaint-prefix,[  --with-cinepaint-prefix=PFX   Prefix where CINEPAINT is installed (optional)],
            cinepainttool_prefix="$withval", cinepainttool_prefix="")
AC_ARG_WITH(cinepaint-exec-prefix,[  --with-cinepaint-exec-prefix=PFX Exec prefix where CINEPAINT is installed (optional)],
            cinepainttool_exec_prefix="$withval", cinepainttool_exec_prefix="")
AC_ARG_ENABLE(cinepainttest, [  --disable-cinepainttest       Do not try to compile and run a test CINEPAINT program],
		    , enable_cinepainttest=yes)

  if test x$cinepainttool_exec_prefix != x ; then
     cinepainttool_args="$cinepainttool_args --exec-prefix=$cinepainttool_exec_prefix"
     if test x${CINEPAINTTOOL+set} != xset ; then
        CINEPAINTTOOL=$cinepainttool_exec_prefix/bin/cinepainttool
     fi
  fi
  if test x$cinepainttool_prefix != x ; then
     cinepainttool_args="$cinepainttool_args --prefix=$cinepainttool_prefix"
     if test x${CINEPAINTTOOL+set} != xset ; then
        CINEPAINTTOOL=$cinepainttool_prefix/bin/cinepainttool
     fi
  fi

  AC_PATH_PROG(CINEPAINTTOOL, cinepainttool, no)
  min_cinepaint_version=ifelse([$1], ,1.0.0,$1)
  AC_MSG_CHECKING(for CINEPAINT - version >= $min_cinepaint_version)
  no_cinepaint=""
  if test "$CINEPAINTTOOL" = "no" ; then
    no_cinepaint=yes
  else
    CINEPAINT_CFLAGS=`$CINEPAINTTOOL $cinepainttool_args --cflags`
    CINEPAINT_LIBS=`$CINEPAINTTOOL $cinepainttool_args --libs`
    cinepainttool_major_version=`$CINEPAINTTOOL $cinepainttool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    cinepainttool_minor_version=`$CINEPAINTTOOL $cinepainttool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    cinepainttool_micro_version=`$CINEPAINTTOOL $cinepainttool_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_cinepainttest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $CINEPAINT_CFLAGS"
      LIBS="$LIBS $CINEPAINT_LIBS"
dnl
dnl Now check if the installed CINEPAINT is sufficiently new. (Also sanity
dnl checks the results of cinepainttool to some extent
dnl
      rm -f conf.cinepainttest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <lib/plugin_main.h>

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  NULL,  /* query_proc */
  NULL   /* run_proc */
};

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.cinepainttest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_cinepaint_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_cinepaint_version");
     exit(1);
   }

    if (($cinepainttool_major_version > major) ||
        (($cinepainttool_major_version == major) && ($cinepainttool_minor_version > minor)) ||
        (($cinepainttool_major_version == major) && ($cinepainttool_minor_version == minor) && ($cinepainttool_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'cinepainttool --version' returned %d.%d.%d, but the minimum version\n", $cinepainttool_major_version, $cinepainttool_minor_version, $cinepainttool_micro_version);
      printf("*** of CINEPAINT required is %d.%d.%d. If cinepainttool is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If cinepainttool was wrong, set the environment variable CINEPAINTTOOL\n");
      printf("*** to point to the correct copy of cinepainttool, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_cinepaint=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_cinepaint" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$CINEPAINTTOOL" = "no" ; then
       echo "*** The cinepainttool script installed by CINEPAINT could not be found"
       echo "*** If CINEPAINT was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the CINEPAINTTOOL environment variable to the"
       echo "*** full path to cinepainttool."
     else
       if test -f conf.cinepainttest ; then
        :
       else
          echo "*** Could not run CINEPAINT test program, checking why..."
          CFLAGS="$CFLAGS $CINEPAINT_CFLAGS"
          LIBS="$LIBS $CINEPAINT_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <lib/plugin_main.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding CINEPAINT or finding the wrong"
          echo "*** version of CINEPAINT. If it is not finding CINEPAINT, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means CINEPAINT was incorrectly installed"
          echo "*** or that you have moved CINEPAINT since it was installed. In the latter case, you"
          echo "*** may want to edit the cinepainttool script: $CINEPAINTTOOL" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     CINEPAINT_CFLAGS=""
     CINEPAINT_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(CINEPAINT_CFLAGS)
  AC_SUBST(CINEPAINT_LIBS)
  rm -f conf.cinepainttest
])



