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
#ifndef __PROCEDURAL_DB_H__
#define __PROCEDURAL_DB_H__

#include "glib.h"
#include "../lib/wire/c_typedefs.h"

/*  Procedural database types  */
typedef enum
{
  PDB_INT32,
  PDB_INT16,
  PDB_INT8,
  PDB_FLOAT,
  PDB_STRING,
  PDB_INT32ARRAY,
  PDB_INT16ARRAY,
  PDB_INT8ARRAY,
  PDB_FLOATARRAY,
  PDB_STRINGARRAY,
  PDB_COLOR,
  PDB_REGION,
  PDB_DISPLAY,
  PDB_IMAGE,
  PDB_LAYER,
  PDB_CHANNEL,
  PDB_DRAWABLE,
  PDB_SELECTION,
  PDB_BOUNDARY,
  PDB_PATH,
  PDB_STATUS,
  PDB_END
} PDBArgType;


/*  Error types  */
typedef enum
{
  PDB_EXECUTION_ERROR,
  PDB_CALLING_ERROR,
  PDB_PASS_THROUGH,
  PDB_SUCCESS
} PDBStatusType;


/*  Procedure types  */
typedef enum
{
  PDB_INTERNAL,
  PDB_PLUGIN,
  PDB_EXTENSION,
  PDB_TEMPORARY
} PDBProcType;


/*  Argument type  */

struct Argument
{
  PDBArgType    arg_type;       /*  argument type  */

  union _ArgValue
  {
    gint32      pdb_int;        /*  Integer type  */
    gdouble     pdb_float;      /*  Floating point type  */
    gpointer    pdb_pointer;    /*  Pointer type  */
  } value;
};


/*  Argument marshalling procedures  */
typedef Argument * (* ArgMarshal) (Argument *);


/*  Execution types  */
typedef struct IntExec IntExec;
typedef struct PlugInExec PlugInExec;
typedef struct ExtExec ExtExec;
typedef struct TempExec TempExec;
typedef struct NetExec NetExec;

struct IntExec
{
  ArgMarshal  marshal_func;   /*  Function called to marshal arguments  */
};

struct PlugInExec
{
#ifdef WIN32
  PluginMain plugin_main; /* dll entry point */
#endif
  gchar *     filename;       /*  Where is the executable on disk?  */
};

struct ExtExec
{
  gchar *     filename;       /*  Where is the executable on disk?  */
};

struct TempExec
{
  void *      plug_in;        /*  Plug-in that registered this temp proc  */
};

struct NetExec
{
  gchar *     host;           /*  Host responsible for procedure execution  */
  gint32      port;           /*  Port on host to send data to  */
};


/*  Structure for a procedure argument  */
typedef struct ProcArg ProcArg;

struct ProcArg
{
  PDBArgType  arg_type;       /*  Argument type (int, char, char *, etc)  */
  char *      name;           /*  Argument name  */
  char *      description;    /*  Argument description  */
};


/*  Structure for a procedure  */

struct ProcRecord
{
  /*  Procedure information  */
  gchar *     name;           /*  Procedure name  */
  gchar *     blurb;          /*  Short procedure description  */
  gchar *     help;           /*  Detailed help instructions  */
  gchar *     author;         /*  Author field  */
  gchar *     copyright;      /*  Copyright field  */
  gchar *     date;           /*  Date field  */

  /*  Procedure type  */
  PDBProcType proc_type;      /*  Type of procedure--Internal, Plug-In, Extension  */

  /*  Input arguments  */
  gint32      num_args;       /*  Number of procedure arguments  */
  ProcArg *   args;           /*  Array of procedure arguments  */

  /*  Output values  */
  gint32      num_values;     /*  Number of return values  */
  ProcArg *   values;         /*  Array of return values  */

  /*  Method of procedure execution  */
  union _ExecMethod
  {
    IntExec     internal;     /*  Execution information for internal procs  */
    PlugInExec  plug_in;      /*  ..................... for plug-ins  */
    ExtExec     extension;    /*  ..................... for extensions  */
    TempExec    temporary;    /*  ..................... for temp procs  */
  } exec_method;
};

/*  External data  */
extern ProcRecord procedural_db_dump_proc;
extern ProcRecord procedural_db_query_proc;
extern ProcRecord procedural_db_proc_info_proc;
extern ProcRecord procedural_db_proc_arg_proc;
extern ProcRecord procedural_db_proc_val_proc;
extern ProcRecord procedural_db_get_data_proc;
extern ProcRecord procedural_db_set_data_proc;

/*  Functions  */
void          procedural_db_init         (void);
void          procedural_db_free         (void);
void          procedural_db_register     (ProcRecord *);
void          procedural_db_unregister   (gchar      *);
ProcRecord *  procedural_db_lookup       (gchar      *);
Argument *    procedural_db_execute      (gchar      *,
					  Argument   *);
Argument *    procedural_db_run_proc     (gchar      *,
					  gint       *,
					  ...);
Argument *    procedural_db_return_args  (ProcRecord *,
					  int);
void          procedural_db_destroy_args (Argument   *,
					  int);

#endif  /*  __PROCEDURAL_DB_H__  */
