#ifndef LOOKPROFILE_H
#define LOOKPROFILE_H

#include "image_map.h"

typedef struct _LookProfile LookProfile;

/* adds the sample strip */
LookProfile *look_profile_create(GImage *gimage, gint sample_depth);
/* includes a manipulation in the profile */
void look_profile_include (LookProfile *, ImageMapApplyFunc, void *);
/* creates an icc profile from the sample strip */
void look_profile_save(LookProfile *profile, gchar *file_name,
                       int intent, DWORD flags);
/* deletes the sample strip */
void look_profile_close(LookProfile *profile);


/* INTERFACE */
typedef struct _LookProfileGui LookProfileManager;
void look_profile_gui(void *display);
void look_profile_gui_refresh(void *display);

#endif /* LOOKPROFILE_H */
