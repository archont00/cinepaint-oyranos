/*
 * This is the base class for the frame managers
 */

#ifndef	BASE_FRAME_MANAGER
#define BASE_FRAME_MANAGER

#include "gimage.h"
#include "gdisplay.h"
#include "procedural_db.h"

typedef struct store_frame_manager store_frame_manager;
typedef struct clip_frame_manager clip_frame_manager;


struct base_frame_manager
{
  GImage *fg;
  GImage *bg;
  char onionskin_b;
  clip_frame_manager *cfm;
  store_frame_manager *sfm;
  char *src_dir;
  char *dest_dir;  
  char *src_name;
  char *dest_name;  

};

void bfm_create_sfm (GDisplay *);
void bfm_create_cfm (GDisplay *);
void bfm_delete_sfm (GDisplay *);
void bfm_delete_cfm (GDisplay *);

char bfm_check (GDisplay *);
int bfm (GDisplay *);

GImage* bfm_get_fg (GDisplay *);
GImage* bfm_get_bg (GDisplay *);
void bfm_dirty (GDisplay *);

char bfm_onionskin (GDisplay *);
void bfm_onionskin_set_offset (GDisplay *, int, int);
void bfm_onionskin_rm (GDisplay *);
void bfm_set_fg (GDisplay *, GImage *);
void bfm_set_bg (GDisplay *, GImage *);
CanvasDrawable* bfm_get_fg_drawable (GDisplay *);
CanvasDrawable* bfm_get_bg_drawable (GDisplay *);
int bfm_get_fg_ID (GDisplay *);     /* renamed bfm_get_fg_display() -hsbo */
int bfm_get_bg_ID (GDisplay *);     /* new -hsbo */
int bfm_getset_bg_ID (GDisplay *);  /* renamed bfm_get_bg_display() -hsbo */
void bfm_onionskin_display (GDisplay *, double, int, int, int, int); 

void bfm_next_filename (GImage *, char *, char *, char, GDisplay *);
char* bfm_get_name (char *);
char* bfm_get_frame (char *);
char* bfm_get_ext (char *);
void bfm_this_filename (char *, char *, char *, char *, int);

GDisplay* bfm_load_image_into_fm (GDisplay *, GImage *);
extern ProcRecord bfm_set_dir_src_proc; 
extern ProcRecord bfm_set_dir_dest_proc; 
void bfm_set_dir_src (GDisplay *, char*); 
void bfm_set_dir_dest (GDisplay *, char*); 

/**
 * This is a single store
 */
typedef struct
{
  GImage *gimage;
  GImage *new_gimage;
  char readonly;
  char advance;
  char flip;
  char fg;
  char bg;
  char selected;
  char active;
  char special;
  char remove; 
  /* GUI */
  GtkWidget *iadvance;
  GtkWidget *iflip;
  GtkWidget *ibg;
  GtkWidget *idirty;
  GtkWidget *ireadonly;
  GtkWidget *label;
  GtkWidget *list_item;

}store;

/**
 * This is the store frame manager
 */
struct store_frame_manager
{
  GSList *stores;
  gint fg;
  gint bg;
  gint readonly;
  gint s_x, s_y, e_x, e_y;    /**<@brief actual Area of Interesst */
  gint sx, sy, ex, ey;        /**<@brief stored Area of Interesst */
  gint onionskin_pause;       /**<@brief let onionskin sleep until revoce */
  int  old_offset_x;          /**<@brief save old variables */
  int  old_offset_y;
  int  old_ex;
  int  old_ey;
  double old_opacity;
  gint play;
  double fps;                 /**<@brief Frames per second */
  gint load_smart;
  /* GUI */
  GtkWidget *aofi_cbtn;       /**<@brief Area of interesst */
  GtkWidget *autosave_cbtn;   /**<@brief check button */
  GtkWidget *onionskin_cbtn;  /**<@brief check button */
  GtkWidget *shell;           /**<@brief the sfm dialog window */
  GtkWidget *store_list;      /**<@brief The central list widget */
  GtkWidget *onionskin_vslider;/**<@brief value slider */
  GtkWidget *add_dialog;      /**<@brief New Store Option dialog*/
  GtkWidget *chg_frame_dialog;/**<@brief dialog window */
  GtkWidget *change_to_entry; /**<@brief entry */
  GtkWidget *num_to_add_spin; /**<@brief spin button : import */
  GtkWidget *num_to_adv_spin; /**<@brief spin button : advance */
  GtkWidget *src_dir_label;   /**<@brief Label */
  GtkWidget *dest_dir_label;  /**<@brief Label */
  GtkWidget *status_label;    /**<@brief Label */
  GtkAccelGroup *accel_group; /**<@brief */
};

/*
 * This is the clip frame manager
 */
typedef struct
{
  GImage *gimage;
}frame;

typedef struct
{
  GSList *frames;
}clip;

struct clip_frame_manager
{
  GSList *fg_clip;
  GSList *bg_clip;
  /* GUI */
};

#endif

