/* wire/datadir.c
// GetDirHome
// Copyright Dec 2, 2002, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)
*/

#ifdef WIN32

#include "../../win32/GetWinUserDir.h"
#include "datadir.h"

#define DOT_DIR ".cinepaint"
#define DATA_DIR "share"

ReplaceBackslashWithSlash(char* path)
{	char* c=strchr(path,'\\');
	while(c)
	{	*c='/';
		c=strchr(c,'\\');
}	}

const char* GetDirHome()
{	static TCHAR* path;
	if(!path)
	{	path=(TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
		if(!path)
		{	return "error";
		}
		path[0]=0;
		GetWinUserDir(path,MAX_PATH);
		ReplaceBackslashWithSlash(path);
	}
	return path;
}

const char* GetDirDot()
{	return DOT_DIR;
}

const char* GetDirData()
{	return DATA_DIR;
}


const char* gimp_personal_rc_file (const char* rcfile)
{
    static TCHAR* rc_file;
    rc_file = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
               if(!path)
               {       return "error";
               }
    sprintf( rc_file, "%s\\%s\\%s",GetDirHome(), GetDirDot(), rcfile );
    return rc_file;
}

/*
const char* GetDirHome()
{	static char username[33];
	DWORD len=33;
	if(!username[0])
	{	if(!GetUserName(username,&len))
		{	strcpy(username,"unknown");
	}	}
	return username;
}
*/
#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iodebug.h"

/*
#define DATA_DIR "share"
#define GetDirData() DATA_DIR
#define GetDirDot() ".cinepaint"
 getenv("GIMP_DIRECTORY") 
#define GetDirHome() getenv("HOME")
#define DOT_DIR ".cinepaint"


 -DLIBDIR=\""/opt/lib/filmgimp/0.16"\" -DDATADIR=\""/opt/share/filmgimp/0.16"\" -DGIMPDIR=\"".filmgimp"\"

programdatadir=$datadir/cinepaint/$VERSION
programplugindir=$libdir/cinepaint/$VERSION
programincludedir=${prefix}/include/cinepaint-$VERSION

*/

const char* GetDirHome()
{	const char* home=getenv("HOME");
	d_printf("GetDirHome: %s\n",home);
	return home;
}

const char* GetDirDot()
{	d_printf("GetDirDot: %s\n",DOTDIR);
	return DOTDIR;
}

const char* prefix_ = PREFIX;
const char* GetDirStaticPrefix()
{	d_printf("GetDirStaticPrefix: %s\n",PREFIX);
	return PREFIX;
}

void SetDirPrefix(const char* prefix)
{	d_printf("SetDirPrefix: %s\n",prefix);
        prefix_ = prefix;
}
const char* GetDirPrefix()
{	d_printf("GetDirPrefix: %s\n",prefix_);
	return prefix_;
}

# ifdef WIN32
# define  DIR_SEPARATOR "\\"
# define  DIR_SEPARATOR_C '\\'
# else
# define  DIR_SEPARATOR "/"
# define  DIR_SEPARATOR_C '/'
# endif
const char* GetDirSeparator()
{ return DIR_SEPARATOR; }

char *plugin_suffix_=NULL;
const char* GetDirPluginSuffix()
{
  if(plugin_suffix_)
    return (const char*)plugin_suffix_;

  plugin_suffix_ = malloc (strlen("lib" "cinepaint" VERSION) * 2);
  sprintf( plugin_suffix_,"%s%s%s%s%s", "lib", DIR_SEPARATOR, "cinepaint",
           DIR_SEPARATOR, VERSION);
  d_printf("GetDirPluginSuffix: %s\n",plugin_suffix_);
  return (const char*)plugin_suffix_;
}

char *data_suffix_=NULL;
const char* GetDirDataSuffix()
{
  if(data_suffix_)
    return data_suffix_;

  data_suffix_ = malloc (strlen("share" "cinepaint" VERSION) * 2);
  sprintf( data_suffix_,"%s%s%s%s%s", "share", DIR_SEPARATOR, "cinepaint",
           DIR_SEPARATOR, VERSION);
  d_printf("GetDirDataSuffix: %s\n",data_suffix_);
  return (const char*)data_suffix_;
}

char *data_=NULL;
const char* GetDirData()
{
  if(data_)
    return data_;

  data_ = malloc (( strlen(GetDirPrefix()) + 512) * 2);
  sprintf( data_,"%s%s%s", GetDirPrefix(), DIR_SEPARATOR,
           GetDirDataSuffix());

  d_printf("GetDirData: %s\n",data_);
  return data_;
}

char* GetDirAbsoluteExec(const char *filename)
{
  char *path = NULL;

  /* Where are we? beku */
  if(filename)
  {
    int len = strlen(filename) * 2 + 1024;
    char *text = (char*) calloc( sizeof(char), len );
    text[0] = 0;
    /* whats the path for the executeable ? */
    snprintf (text, len-1, filename);

    if(text[0] == '~')
    {
      /* home directory */
      sprintf( text, "%s%s", GetDirHome(), &filename[0]+1 );
    }

    /* relative names - where the first sign is no directory separator */
    if (text[0] != DIR_SEPARATOR_C)
    {
      FILE *pp = NULL;

      if (text) free (text);
      text = (char*) malloc( 1024 );

      /* Suche das ausfuehrbare Programm
         TODO symbolische Verknuepfungen */
      snprintf( text, 1024, "which %s", filename);
      pp = popen( text, "r" );
      if (pp) {
        if (fscanf (pp, "%s", text) != 1)
        {
          pclose (pp);
          printf( "no executeable path found\n" );
        }
      } else {
        printf( "could not ask for executeable path\n" );
      }

      if(text[0] != DIR_SEPARATOR_C)
      {
        char* cn = (char*) calloc(2048, sizeof(char));
        sprintf (cn, "%s%s%s", getenv("PWD"), DIR_SEPARATOR, filename);
        sprintf (text, cn);
        if(cn) free(cn);
      }
    }

    { /* remove the executable name */
      char *tmp = strrchr(text, DIR_SEPARATOR_C);
      if(tmp)
        *tmp = 0;
    }
    while (text[strlen(text)-1] == '.')
      text[strlen(text)-1] = 0;
    while (text[strlen(text)-1] == DIR_SEPARATOR_C)
      text[strlen(text)-1] = 0;

    path = text;
  }
  return path;
}

#include <dirent.h>
#include <sys/stat.h>
int
IsDir (const char* path)
{
  struct stat status;
  int r = 0;
  const char* name = path;
  status.st_mode = 0;
  r = stat (name, &status);
  r = !r &&
       ((status.st_mode & S_IFMT) & S_IFDIR);

  return r;
}



const char* gimp_personal_rc_file (const char* rcfile)
{
    static char* rc_file;
    rc_file = (char*)malloc(sizeof(char)*2048);
               if(!rc_file)
               {       return "error";
               }
    sprintf( rc_file, "%s/%s/%s",GetDirHome(), GetDirDot(), rcfile );
    return rc_file;
}

#endif
