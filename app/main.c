/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef WIN32
#ifdef _DEBUG
#define HEAP_DEBUG
#ifdef HEAP_DEBUG
#include <crtdbg.h>
#endif
#endif
#endif

#include "config.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
/* check for directories */
#include <sys/stat.h>
#include <unistd.h>


#ifdef WIN32
extern int _fmode;
#else
#include <sys/wait.h>
#endif

#include <unistd.h>
#include "../lib/version.h"
#include "tag.h"
#include "rc.h"
#include "libgimp/gimpintl.h"

#ifndef  WAIT_ANY
#define  WAIT_ANY -1
#endif   /*  WAIT_ANY  */

#ifdef HAVE_OY
#include <oyranos/oyranos.h>
#include <oyranos/oyranos_config.h>
#include <oyranos/oyranos_version.h>
char *oyranos_temp_path = NULL;
void *myAlloc(size_t n) { return calloc(sizeof(char), n); }
#endif

#define  CP_CHANGE_CODESET 0
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "appenv.h"
#include "app_procs.h"
#include "errors.h"
#include "install.h"

#include "gdisplay.h"
static RETSIGTYPE on_signal (int);
static RETSIGTYPE on_sig_child (int);
static RETSIGTYPE on_sig_refresh (int);
static void       init (void);

/* GLOBAL data */
int no_interface;
int no_data;
int no_splash;
int no_splash_image;
int be_verbose;
int use_debug_handler;
int console_messages;
int start_with_sfm;
int initial_frames_loaded;

MessageHandlerType message_handler;

char *prog_name;		/* The path name we are invoked with */
char **batch_cmds;
int parentPID = 0;		/* If invoked using share memory intf from another process.. who parent is */


/* LOCAL data */
static int gimp_argc;
static char **gimp_argv;
static int useSharedMem = FALSE;



/* added by IMAGEWORKS (02/21/02) */
static int serverPort;
static char *serverLog;

/* beku */
void cinepaint_init_i18n (const char *exec_path);
#ifdef WIN32
#define DIR_SEPARATOR "\\"
#define DIR_SEPARATOR_C '\''
#else
#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_C '/'
#endif

/*
 *  argv processing:
 *      Arguments are either switches, their associated
 *      values, or image files.  As switches and their
 *      associated values are processed, those slots in
 *      the argv[] array are NULLed. We do this because
 *      unparsed args are treated as images to load on
 *      startup.
 *
 *      The GTK switches are processed first (X switches are
 *      processed here, not by any X routines).  Then the
 *      general GIMP switches are processed.  Any args
 *      left are assumed to be image files the GIMP should
 *      display.
 *
 *      The exception is the batch switch.  When this is
 *      encountered, all remaining args are treated as batch
 *      commands.
 */

#ifdef WIN32

#define argv __argv
#define argc __argc

int WINAPI WinMain(
  HINSTANCE hInstance,  // handle to current instance
  HINSTANCE hPrevInstance,  // handle to previous instance
  LPSTR lpCmdLine,      // pointer to command line
  int nCmdShow          // show state of window
)
{

#else

int main (int argc, char **argv)
{
#endif
  int show_version;
  int show_help;
  int i, j;
  char *path = "";
#ifdef HAVE_PUTENV
  gchar *display_name, *display_env;
#endif
#ifdef WIN32
	_fmode = _O_BINARY; /* doesn't seem to work, had to reset open calls */
#ifdef _DEBUG
/* This creates a console window and joins it to a GUI process: */
	AllocConsole();
	freopen("CONIN$","rb",stdin);
	freopen("CONOUT$","wb",stdout);
	freopen("CONOUT$","wb",stderr);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
#endif
  /* ATEXIT (g_mem_profile); */

  /* Initialize variables */
  prog_name = argv[0];

  /* Where are we? */
  {
    int len = strlen(prog_name) * 2 + 128;
    char *text = GetDirAbsoluteExec( prog_name );

    /* remove the expected bin/ dir */
    char *tmp = strrchr(text, DIR_SEPARATOR_C);
    if(tmp)
      *tmp = 0;

    /* look for something known */
    {
      char *t = malloc((strlen(text)+1024)*2);
      FILE *fp;

      sprintf( t,"%s%slib%scinepaint%s%s%splug-ins%sblur",
               text, DIR_SEPARATOR, DIR_SEPARATOR,
               DIR_SEPARATOR, PROGRAM_VERSION, DIR_SEPARATOR, DIR_SEPARATOR);
      fp = fopen(t,"r");
      if(fp)
      {
#ifdef HAVE_OY
        /* set a profile temporary path */
        /* this is obsolete with the XDG paths being supported since v0.1.8 */
#if (OYRANOS_VERSION < 108)
        if(strcmp(text, GetDirPrefix()) != 0)
        {
          int n = oyPathsCount(), i, has_path = 0;

          oyranos_temp_path = myAlloc (strlen(GetDirPrefix()) +
                     sizeof(OY_ICCDIRNAME) + strlen(text) + 64);
          sprintf(oyranos_temp_path, "%s%sshare%scolor%s%s",
              text, DIR_SEPARATOR, DIR_SEPARATOR, DIR_SEPARATOR, OY_ICCDIRNAME);
          for(i = 0; i < n; ++i)
          {
            char *temp = oyPathName(i, myAlloc);
            if(strcmp(oyranos_temp_path, temp) == 0)
              has_path = 1;
            if(temp) free(temp);
          }
          if(!has_path)
          {
            struct stat status;
            int r = 0;

            status.st_mode = 0;
            r = stat (oyranos_temp_path, &status);
            r = !r &&
              ((status.st_mode & S_IFMT) & S_IFDIR);
            if(!r)
              oyPathAdd( oyranos_temp_path );
          } else {
            free( oyranos_temp_path );
            oyranos_temp_path = NULL;
          }
        }
#endif
#endif
        SetDirPrefix( text );
        fclose(fp);
      }
      free(t);
    }

    path = (char*) malloc( len + strlen(text) + 1024 );
    sprintf (path, "%s%s%s", GetDirPrefix(), DIR_SEPARATOR, "share/locale");
  }

#if GTK_MAJOR_VERSION > 1
  setlocale(LC_ALL,"");
  cinepaint_init_i18n(path);
# ifdef DEBUG
  printf("Translation test: %s\n",gettext("About"));
# endif
#endif

  /* Initialize Gtk toolkit */
  gtk_init (&argc, &argv);

#if GTK_MAJOR_VERSION < 2
  setlocale(LC_NUMERIC, "C");/* must use dot, not comma, as decimal separator */
#endif

#ifdef HAVE_PUTENV
  display_name = gdk_get_display ();
  display_env = g_new (gchar, strlen (display_name) + 9);
  *display_env = 0;
  strcat (display_env, "DISPLAY=");
  strcat (display_env, display_name);
  putenv (display_env);
#endif

  no_interface = FALSE;
  no_data = FALSE;
  no_splash = FALSE;
  no_splash_image = FALSE;
  use_debug_handler = FALSE;
  console_messages = FALSE;
  start_with_sfm = FALSE;
  initial_frames_loaded = -1;
  message_handler = CONSOLE;

  batch_cmds = g_new (char*, argc);
  batch_cmds[0] = NULL;

  show_version = FALSE;
  show_help = FALSE;

  for (i = 1; i < argc; i++)
    {
      /* added by IMAGEWORKS (02/21/02) */
      if( strcmp( argv[i], "-server" ) == 0 ) {
          serverPort = atoi( argv[++i] );
          serverLog = argv[++i];
      }
      else if ((strcmp (argv[i], "--no-interface") == 0) ||
	  (strcmp (argv[i], "-n") == 0))
	{
	  no_interface = TRUE;
	}
      else if ((strcmp (argv[i], "--batch") == 0) ||
	       (strcmp (argv[i], "-b") == 0))
	{
	  for (j = 0, i++ ; i < argc; j++, i++)
	      batch_cmds[j] = argv[i];
	  batch_cmds[j] = NULL;
	  if (batch_cmds[0] == NULL)	/* We need at least one batch command */
		 show_help = TRUE;
	  if (argv[i-1][0] != '-')		/* Did loop end due to a new argument? */
		 --i;						/* Ensure new argument gets processed */
	}
      else if (strcmp (argv[i], "-seq") == 0)
      {
	start_with_sfm=TRUE;
	if(argv[i+1]!=NULL)
	  {
	    initial_frames_loaded=atoi(argv[i+1]);
	  }
	if(initial_frames_loaded<1)
	  {
	    show_help = TRUE;
	    argv[i] = NULL;
	  }
      }
      else if ((strcmp (argv[i], "--help") == 0) ||
	       (strcmp (argv[i], "-h") == 0))
	{
	  show_help = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--version") == 0 ||
	       strcmp (argv[i], "-v") == 0)
	{
	  show_version = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-data") == 0)
	{
	  no_data = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-splash") == 0)
	{
	  no_splash = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--no-splash-image") == 0)
	{
	  no_splash_image = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--verbose") == 0)
	{
	  be_verbose = TRUE;
	  argv[i] = NULL;
	}
#ifdef BUILD_SHM
      else if (strcmp (argv[i], "--no-shm") == 0)
	{
	  use_shm = FALSE;
	  argv[i] = NULL;
	}
#endif
      else if (strcmp (argv[i], "--debug-handlers") == 0)
	{
	  use_debug_handler = TRUE;
	  argv[i] = NULL;
	}
      else if (strcmp (argv[i], "--console-messages") == 0)
        {
          console_messages = TRUE;
	  argv[i] = NULL;
        }
      else if( strcmp( argv[i], "--sharedmem" ) == 0 )
	{
#ifdef BUILD_SHM
	  if( i+6 >= argc )
		  show_help = TRUE;
	  else
	  {
		  useSharedMem = TRUE;
		  parentPID = atoi( argv[++i] );
		  shmid = atoi( argv[++i] );
		  offset = atoi( argv[++i] );
		  chans = atoi( argv[++i] );
		  xSize = atoi( argv[++i] );
		  ySize = atoi( argv[++i] );
		  /*
		  d_printf( "Parent PID: %d, shmid: %d, off: %d, chans: %d, (%d,%d)\n", 
				  parentPID, shmid, offset, chans, xSize, ySize );
		  */
	  }
#else
		d_puts("Built without shared memory -- can't enable!");
#endif
	}
#ifdef __APPLE__
      else if (strstr (argv[i], "-psn_") != NULL)
        {
	  argv[i] = NULL;
        }
#endif
/*
 *    ANYTHING ELSE starting with a '-' is an error.
 */

      else if (argv[i][0] == '-')
	{
	  show_help = TRUE;
	}
    }

  if (show_version)
    g_print (PROGRAM_NAME " version " PROGRAM_VERSION "\n");

  if (show_help)
    {
      g_print ("\007Usage: %s [option ...] [files ...]\n", argv[0]);
      g_print ("Valid options are:\n");
      g_print ("  -h --help              Output this help.\n");
      g_print ("  -v --version           Output version info.\n");
      g_print ("  -b --batch <commands>  Run in batch mode.\n");
      g_print ("  -n --no-interface      Run without a user interface.\n");
      g_print ("  --no-data              Do not load patterns, gradients, palettes, brushes.\n");
      g_print ("  --verbose              Show startup messages.\n");
      g_print ("  --no-splash            Do not show the startup window.\n");
      g_print ("  --no-splash-image      Do not add an image to the startup window.\n");
      g_print ("  --no-shm               Do not use shared memory between GIMP and its plugins.\n");
      g_print ("  --no-xshm              Do not use the X Shared Memory extension.\n");
      g_print ("  --console-messages     Display warnings to console instead of a dialog box.\n");
      g_print ("  --debug-handlers       Enable debugging signal handlers.\n");
      g_print ("  --display <display>    Use the designated X display.\n");
      g_print ("  -seq <number>          Open frame manager and load a number of frames into\n");
      g_print ("                         a film sequence.\n\n");

    }

  if (show_version || show_help)
    exit (0);

#if GTK_MAJOR_VERSION > 1
  g_log_set_handler(NULL,
                    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                    message_func_cb, NULL);
#else
  g_set_message_handler (&message_func);
#endif

#ifdef WIN32

#else
# ifndef DEBUG_
  /* Handle some signals */
  signal (SIGHUP, on_signal);
  signal (SIGINT, on_signal);
  signal (SIGQUIT, on_signal);
  signal (SIGABRT, on_signal);
  signal (SIGBUS, on_signal);
  signal (SIGSEGV, on_signal);
  signal (SIGPIPE, on_signal);
  signal (SIGTERM, on_signal);
  signal (SIGFPE, on_signal);

  /* Handle child exits */
  signal (SIGCHLD, on_sig_child);
# ifdef BUILD_SHM
  /* Handle shmem reload */
  signal( SIGUSR2, on_sig_refresh);
# endif
# endif /* DEBUG */
#endif
  /* Keep the command line arguments--for use in gimp_init */
  gimp_argc = argc - 1;
  gimp_argv = argv + 1;

  /* Check the installation */
#ifdef BUILD_SHM
  if( useSharedMem )
	install_verify( init_shmem );
  else
#endif
  install_verify (init);


  /* Main application loop */
  if (!app_exit_finish_done ())
    gtk_main ();

  {
#if 0
    extern int ref_ro, ref_rw,ref_un, ref_fa , ref_uf;
    
    g_warning ("Refs:   %d+%d = %d", ref_ro, ref_rw, (ref_ro+ref_rw));
    g_warning ("Unrefs: %d", ref_un);
    
    g_warning ("Frefs:   %d", ref_fa);
    g_warning ("Funrefs: %d", ref_uf);
#endif
  }

#ifdef HAVE_OY
  /* remove a profile temporary path */
  if(oyranos_temp_path)
  {
#if (OYRANOS_VERSION < 108)
    oyPathRemove( oyranos_temp_path );
#endif
    free( oyranos_temp_path );
  }
#endif

  return 0;
}

static void
init ()
{
  /*  Continue initializing  */

  /* Initialize the application */
  app_init ();

#if GTK_MAJOR_VERSION > 1
  /* Why does gtk change LC_NUMERIC with the first window in gtk-2.4 ? beku */
  {
    char settxt[64];
    if(setlocale (LC_NUMERIC, NULL))
    {
      snprintf( settxt,63, "export LC_NUMERIC=%s", setlocale(LC_NUMERIC, NULL));
      putenv( settxt );
      setenv("LC_NUMERIC", setlocale (LC_NUMERIC, NULL), 1);
    } else
      printf("%s:%d %s() no LC_NUMERIC set\n",__FILE__,__LINE__,__func__);
  }
#endif

  gimp_init (gimp_argc, gimp_argv);
}

static int caught_fatal_sig = 0;

static RETSIGTYPE
on_signal (int sig_num)
{
  if (caught_fatal_sig)
/*    raise (sig_num);*/
    kill (getpid (), sig_num);
  caught_fatal_sig = 1;

  switch (sig_num)
    {
    case SIGHUP:
      terminate ("sighup caught");
      break;
    case SIGINT:
      terminate ("sigint caught");
      break;
    case SIGQUIT:
      terminate ("sigquit caught");
      break;
    case SIGABRT:
      terminate ("sigabrt caught");
      break;
    case SIGBUS:
      fatal_error ("sigbus caught");
      break;
    case SIGSEGV:
      fatal_error ("sigsegv caught");
      break;
    case SIGPIPE:
      /*terminate*/printf ("sigpipe caught\n");
      break;
    case SIGTERM:
      terminate ("sigterm caught");
      break;
    case SIGFPE:
      fatal_error ("sigfpe caught");
      break;
    default:
      fatal_error ("unknown signal");
      break;
    }
}

static RETSIGTYPE
on_sig_child (int sig_num)
{
  int pid;
  int status;

  while (1)
    {
      pid = waitpid (WAIT_ANY, &status, WNOHANG);
      if (pid <= 0)
	break;
    }
}



#define TEXTLEN 128

void
fl_set_codeset_    ( const char* lang, const char* codeset_,
                     char* locale, char* codeset,
                     int set_locale )
{
    if( strlen(locale) )
    {
      char* pos = strstr(locale, lang);
      if(pos != 0)
      {
        /* 1 a. select an appropriate charset (needed for non UTF-8 fltk/gtk1)*/
        sprintf (codeset, codeset_);
 
          /* merge charset with locale string */
        if(set_locale)
        {
          if((pos = strrchr(locale,'.')) != 0)
          {
            *pos = 0;
          } else
            pos = & locale[strlen(locale)];
          snprintf(pos, TEXTLEN-strlen(locale), ".%s",codeset);
        }

        /* 1b. set correct environment variable LANG */
        {
          char settxt[64];
          snprintf( settxt, 63, "LANG=%s", locale );
          putenv( settxt );
        }
        setenv("LANG", locale, 1); /* setenv is not standard C */

        /* 1c. set the locale info after LANG */
        if(set_locale)
        {
#if GTK_MAJOR_VERSION < 2
          char *ptr = setlocale (LC_MESSAGES, "");
#else
          char *ptr = setlocale (LC_ALL, "");
#endif
          if(ptr) snprintf( locale, TEXTLEN, ptr);
        }
      }
    }
}

#ifdef WIN32
#define DIR_SEPARATOR "\\"
#else
#define DIR_SEPARATOR "/"
#endif

signed int
fl_search_locale_path (int n_places, const char **locale_paths,
                    const char *search_lang, const char *app_name)
{
  int i;
  /* search in a given set of locale paths for a valid locale file */
  for (i = 0; i < n_places; ++i)
  {
    if(locale_paths[i])
    {
      char test[1024];
      FILE *fp = 0;
      /* construct the full path to a possibly valid locale file */
      snprintf(test, 1024, "%s%s%s%sLC_MESSAGES%s%s.mo",
                           locale_paths[i], DIR_SEPARATOR,
                           search_lang, DIR_SEPARATOR, DIR_SEPARATOR, app_name);
      /* test the file for existence */
      fp = fopen(test, "r");
      if(fp)
      {
        fclose(fp);
        /* tell about the hit place */
        return i;
      }
    }
  }
  return -1;
}

void
fl_initialise_locale( const char *domain, const char *locale_path,
                      int set_locale )
{
  char locale[TEXTLEN];
  char codeset[24] = "ISO-8859-1";
  const char* tmp = 0;
  const char *loc = NULL;
  char* bdtd = 0;

# ifdef __APPLE__
  // 1. get the locale info
  CFLocaleRef userLocaleRef = CFLocaleCopyCurrent();
  CFStringRef cfstring = CFLocaleGetIdentifier( userLocaleRef );
  CFIndex gr = 36;
  char text[36];
  Boolean fehler = 0;
 
  CFShow( cfstring );

    // copy to a C buffer
  fehler = CFStringGetCString( cfstring, text, gr, kCFStringEncodingISOLatin1 );

  if(fehler) {
      d_printf( "osX locale obtained: %s", text );
    snprintf(locale,TEXTLEN, text);
  } else {
      d_printf( "osX locale not obtained: %s", text );
  }

  // set the locale info
  if(strlen(locale) && set_locale)
  {
#if GTK_MAJOR_VERSION < 2
     setlocale (LC_MESSAGES, locale);
#else
     setlocale (LC_ALL, locale);
#endif
  }
  if (tmp)
    snprintf(locale,TEXTLEN, tmp);
  set_locale = 0;
# else

  // 1. get default locale info ..
  // this is dangerous
  /*const char *tmp = setlocale (LC_MESSAGES, NULL);
  if(tmp) {
    snprintf(locale,TEXTLEN, tmp);
    DBG_PROG_V( locale )
  }*/

    // .. or take locale info from environment
  if(getenv("LANG"))
    snprintf(locale,TEXTLEN, getenv("LANG"));
# endif


      // add more LINGUAS here
      // borrowed from http://czyborra.com/charsets/iso8859.html
    fl_set_codeset_( "af", "ISO-8859-1", locale, codeset, set_locale ); // Afrikaans
    fl_set_codeset_( "ca", "ISO-8859-1", locale, codeset, set_locale ); // Catalan
    fl_set_codeset_( "da", "ISO-8859-1", locale, codeset, set_locale ); // Danish
    fl_set_codeset_( "de", "ISO-8859-1", locale, codeset, set_locale ); // German
    fl_set_codeset_( "en", "ISO-8859-1", locale, codeset, set_locale ); // English
    fl_set_codeset_( "es", "ISO-8859-1", locale, codeset, set_locale ); // Spanish
    fl_set_codeset_( "eu", "ISO-8859-1", locale, codeset, set_locale ); // Basque
    fl_set_codeset_( "fi", "ISO-8859-1", locale, codeset, set_locale ); // Finnish
    fl_set_codeset_( "fo", "ISO-8859-1", locale, codeset, set_locale ); // Faroese
    fl_set_codeset_( "fr", "ISO-8859-1", locale, codeset, set_locale ); // French
    fl_set_codeset_( "ga", "ISO-8859-1", locale, codeset, set_locale ); // Irish
    fl_set_codeset_( "gd", "ISO-8859-1", locale, codeset, set_locale ); // Scottish
    fl_set_codeset_( "is", "ISO-8859-1", locale, codeset, set_locale ); // Icelandic
    fl_set_codeset_( "it", "ISO-8859-1", locale, codeset, set_locale ); // Italian
    fl_set_codeset_( "nl", "ISO-8859-1", locale, codeset, set_locale ); // Dutch
    fl_set_codeset_( "no", "ISO-8859-1", locale, codeset, set_locale ); // Norwegian
    fl_set_codeset_( "pt", "ISO-8859-1", locale, codeset, set_locale ); // Portuguese
    fl_set_codeset_( "rm", "ISO-8859-1", locale, codeset, set_locale ); // Rhaeto-Romanic
    fl_set_codeset_( "sq", "ISO-8859-1", locale, codeset, set_locale ); // Albanian
    fl_set_codeset_( "sv", "ISO-8859-1", locale, codeset, set_locale ); // Swedish
    fl_set_codeset_( "sw", "ISO-8859-1", locale, codeset, set_locale ); // Swahili

    fl_set_codeset_( "cs", "ISO-8859-2", locale, codeset, set_locale ); // Czech
    fl_set_codeset_( "hr", "ISO-8859-2", locale, codeset, set_locale ); // Croatian
    fl_set_codeset_( "hu", "ISO-8859-2", locale, codeset, set_locale ); // Hungarian
    fl_set_codeset_( "pl", "ISO-8859-2", locale, codeset, set_locale ); // Polish
    fl_set_codeset_( "ro", "ISO-8859-2", locale, codeset, set_locale ); // Romanian
    fl_set_codeset_( "sk", "ISO-8859-2", locale, codeset, set_locale ); // Slovak
    fl_set_codeset_( "sl", "ISO-8859-2", locale, codeset, set_locale ); // Slovenian

    fl_set_codeset_( "eo", "ISO-8859-3", locale, codeset, set_locale ); // Esperanto
    fl_set_codeset_( "mt", "ISO-8859-3", locale, codeset, set_locale ); // Maltese

    fl_set_codeset_( "et", "ISO-8859-4", locale, codeset, set_locale ); // Estonian
    fl_set_codeset_( "lv", "ISO-8859-4", locale, codeset, set_locale ); // Latvian
    fl_set_codeset_( "lt", "ISO-8859-4", locale, codeset, set_locale ); // Lithuanian
    fl_set_codeset_( "kl", "ISO-8859-4", locale, codeset, set_locale ); // Greenlandic

    fl_set_codeset_( "be", "ISO-8859-5", locale, codeset, set_locale ); // Byelorussian
    fl_set_codeset_( "bg", "ISO-8859-5", locale, codeset, set_locale ); // Bulgarian
    fl_set_codeset_( "mk", "ISO-8859-5", locale, codeset, set_locale ); // Macedonian
    fl_set_codeset_( "ru", "ISO-8859-5", locale, codeset, set_locale ); // Russian
    fl_set_codeset_( "sr", "ISO-8859-5", locale, codeset, set_locale ); // Serbian
    fl_set_codeset_( "uk", "ISO-8859-5", locale, codeset, set_locale ); // Ukrainian

    fl_set_codeset_( "ar", "ISO-8859-6", locale, codeset, set_locale ); // Arabic
    fl_set_codeset_( "fa", "ISO-8859-6", locale, codeset, set_locale ); // Persian
    fl_set_codeset_( "ur", "ISO-8859-6", locale, codeset, set_locale ); // Pakistani Urdu

    fl_set_codeset_( "el", "ISO-8859-7", locale, codeset, set_locale ); // Greek

    fl_set_codeset_( "iw", "ISO-8859-8", locale, codeset, set_locale ); // Hebrew
    fl_set_codeset_( "ji", "ISO-8859-8", locale, codeset, set_locale ); // Yiddish

    fl_set_codeset_( "tr", "ISO-8859-9", locale, codeset, set_locale ); // Turkish

    fl_set_codeset_( "th", "ISO-8859-11", locale, codeset, set_locale ); // Thai

    fl_set_codeset_( "zh", "ISO-8859-15", locale, codeset, set_locale ); // Chinese

    fl_set_codeset_( "ja", "EUC", locale, codeset, set_locale ); // Japan ; eucJP, ujis, EUC, PCK, jis7, SJIS

    fl_set_codeset_( "hy", /*"UTF-8"*/"ARMSCII-8", locale, codeset, set_locale ); // Armenisch


  // 2. for GNU gettext, the locale info is usually stored in the LANG variable
  loc = getenv("LANG");

  if(loc) {

      // good

  } else {

      // set LANG
#   ifdef __APPLE__
    if (strlen(locale))
      setenv("LANG", locale, 0);
#   endif

      // good?
    if(getenv("LANG"))
      d_printf( getenv("LANG") );
  }

  if(strlen(locale))
    d_printf(locale);

  // 3. where to find the MO file? select an appropriate directory
  bdtd = bindtextdomain (domain, locale_path);

  // 4. set our charset
  //char* cs = bind_textdomain_codeset(domain, codeset);

  // 5. our translations
  textdomain (domain);

  // gettext initialisation end

}


void
cinepaint_init_i18n (const char *exec_path)
{
  const char *locale_paths[3];
  const char *domain[] = {"cinepaint","cinepaint-std-plugins"};
  int         paths_n = 3, i;

  int path_nr = 0;

#if GTK_MAJOR_VERSION < 2
  int set_locale = 1;
#else
  int set_locale = 0;
#endif

  locale_paths[0] = LOCALEDIR;
  locale_paths[1] = "../po";
  locale_paths[2] = exec_path;
  path_nr = fl_search_locale_path  ( paths_n,
                                     locale_paths,
                                     "de",
                                     domain[0]);

  if ( path_nr >= 0 )
  {
    fl_initialise_locale( domain[0], locale_paths[path_nr], set_locale );
    fl_initialise_locale( domain[1], locale_paths[path_nr], set_locale );
    textdomain (domain[0]);
    printf("Locale found in %s\n", locale_paths[path_nr]);
    setenv("TEXTDOMAINDIR", locale_paths[path_nr], 1);
  } else
  {
    for (i=0; i < paths_n; ++i)
      printf("Locale not found in %s\n", locale_paths[i]);
  }
}
