/* wire/c_typedefs.h
// Typedefs needed by wire protocol
// Copyright May 3, 2003, Robin.Rowe@MovieEditor.com
// License MIT (http://opensource.org/licenses/mit-license.php)

WORKAROUND FOR THIS:

typedef struct WireMessage WireMessage;
typedef struct WireMessage WireMessage;

$ gcc  -g -O2 -Wall -c wire.c
In file included from wire.c:32:
wire.h:32: redefinition of `WireMessage'
wire.h:31: `WireMessage' previously declared here

[ Can't redefine C typedef even when identical. ]
***********************************************************/
#ifndef C_TYPEDEFS_H
#define C_TYPEDEFS_H

typedef struct WireBuffer WireBuffer;
typedef int (*PluginMain)(void* data);

typedef struct PlugIn PlugIn;
typedef struct WireMessage WireMessage;

typedef struct Argument Argument;
typedef struct ProcRecord ProcRecord;

typedef struct GPlugInInfo GPlugInInfo;
typedef struct GDrawableClass GDrawableClass;

typedef struct GTile GTile;
#define GimpTile GTile 

typedef struct GPixelRgn GPixelRgn;
#define GimpPixelRgn GPixelRgn 

typedef struct TileDrawable TileDrawable;
typedef struct CanvasDrawable CanvasDrawable;

#define GimpPDBArgType GParamType 
#define GimpFillType GFillType 
#define GimpDrawableType GImageType 
#define GimpRunModeType GRunModeType
 
#define GimpPDBProcType GimpProcedureType

typedef struct GParam GParam;
#define GimpParam GParam       

typedef union GParamData GParamData;
#define GimpParamData GParamData

typedef struct GParamDef    GParamDef;
#define GimpParamDef GParamDef 

typedef struct GParamColor GParamColor;
#define GimpParamColor GParamColor  

typedef struct GParamRegion GParamRegion;
#define GimpParamRegion GParamRegion 


typedef struct Canvas Canvas;
typedef struct DrawCore DrawCore;
typedef struct Tool Tool;
typedef struct GimpDrawableClass GimpDrawableClass;
typedef struct PixelRow PixelRow;


typedef struct PixelArea PixelArea;
typedef struct Guide  Guide;
typedef struct GImage GImage;
typedef struct TileManager TileManager;
/*#define TileManager Canvas*/

typedef struct GimpLayer GimpLayer;
typedef struct GimpLayerClass GimpLayerClass;
typedef struct GimpLayerMask GimpLayerMask;
typedef struct GimpLayerMaskClass GimpLayerMaskClass;

typedef GimpLayer Layer;		/* convenience */
typedef GimpLayerMask LayerMask;	/* convenience */

typedef struct LayerUndo LayerUndo;
typedef struct LayerMaskUndo LayerMaskUndo;

typedef struct FStoLayerUndo FStoLayerUndo;

typedef struct GPattern  GPattern, * GPatternP;

typedef struct GimpChannel      GimpChannel;
typedef struct GimpChannelClass GimpChannelClass;

typedef GimpChannel Channel;		/* convenience */

typedef TileDrawable GDrawable;
/*
typedef struct PaintCore16 PaintCore16;
typedef PaintCore16 PaintCore;
*/

#define CAST(x) (x)

#endif
