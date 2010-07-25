/*
 * WidgetWrapper.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
 *
 * Copyright (c) 2005-2006  Hartmut Sbosny  <hartmut.sbosny@gmx.de>
 *
 * LICENSE:
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/**
  WidgetWrapper.cpp
*/

#include <cstring>              // memcpy()
#include "WidgetWrapper.hpp"


#ifdef BR_DEBUG_WIDGET_WRAPPER
#  include <cstdio>
#  define DBG_PRINTF(args)   printf args ;
#else
#  define DBG_PRINTF(args)
#endif



/**
  Registers the WidgetWrapper object \a wr for deleting the next time 
   do_wrappers_deletion() is called.
*/
void WidgetWrapper::delete_wrapper (WidgetWrapper* wr)
{
  if (!wr) return;

  if (num_dwrappers_ >= num_alloc_dwrappers_) {
    WidgetWrapper  **temp;

    temp = new WidgetWrapper* [num_alloc_dwrappers_ + 10];
    if (num_alloc_dwrappers_) {
      memcpy (temp, dwrappers_, num_alloc_dwrappers_ * sizeof(WidgetWrapper *));
      delete[] dwrappers_;
    }

    dwrappers_ = temp;
    num_alloc_dwrappers_ += 10;
  }

  dwrappers_[num_dwrappers_] = wr;
  num_dwrappers_ ++;
}

/**
  Deletes all WidgetWrapper objects registered since last do_wrappers_deletion().
*/
void WidgetWrapper::do_wrappers_deletion()
{
  if (!num_dwrappers_) return;

  bool removed_backup = false;
  bool removed_cleanup = false;
  
  for (int i=0; i < num_dwrappers_; i++)
  {
    /*  Call the associated backup function (if any) BEFORE deletion */
    for (int k=0; k < num_backups_; k++)
      if (backups_[k].wr == dwrappers_[i]  &&  backups_[k].func)
      {
        DBG_PRINTF(("%s(): call backup( u=%p )\n",__func__, backups_[k].userdata))
        
        backups_[k].func (backups_[k].userdata);
        backups_[k].wr = 0;    // mark as "deleted"
        removed_backup = true;
      }
    
    delete dwrappers_[i];
    
    /*  Call the associated cleanup function (if any) AFTER deletion */
    for (int k=0; k < num_cleanups_; k++)
      if (cleanups_[k].wr == dwrappers_[i]  &&  cleanups_[k].func)
      {
        DBG_PRINTF(("%s(): call cleanup( u=%p )\n",__func__, cleanups_[k].userdata))
       
        cleanups_[k].func (cleanups_[k].userdata);
        cleanups_[k].wr = 0;    // mark as "deleted"
        removed_cleanup = true;
      }
  }
  num_dwrappers_ = 0;
  
  if (removed_backup) condense_backups_();
  if (removed_cleanup) condense_cleanups_();
}

/**
  Registers for the WidgetWrapper object \a wr the backup function \a f, which
   will be called BEFORE deletion if \a wr should be deleted by do_wrappers_deletion().
*/
void WidgetWrapper::register_backup_func (WidgetWrapper *wr, BackupFunc *f, void* udata)
{
  if (!wr || !f) return;

  if (num_backups_ >= num_alloc_backups_) {
    BackupEntry  *temp;

    temp = new BackupEntry [num_alloc_backups_ + 10];
    if (num_alloc_backups_) {
      memcpy (temp, backups_, num_alloc_backups_ * sizeof(BackupEntry));
      delete[] backups_;
    }

    backups_ = temp;
    num_alloc_backups_ += 10;
  }

  backups_[num_backups_] = BackupEntry (wr, f, udata);
  num_backups_ ++;
}

/**
  Registers for the WidgetWrapper object \a wr the cleanup function \a f, which
   will be called AFTER deletion if \a wr should be deleted by do_wrappers_deletion().
*/
void WidgetWrapper::register_cleanup_func (WidgetWrapper *wr, CleanupFunc *f, void* udata)
{
  if (!wr || !f) return;

  if (num_cleanups_ >= num_alloc_cleanups_) {
    CleanupEntry  *temp;

    temp = new CleanupEntry [num_alloc_cleanups_ + 10];
    if (num_alloc_cleanups_) {
      memcpy (temp, cleanups_, num_alloc_cleanups_ * sizeof(CleanupEntry));
      delete[] cleanups_;
    }

    cleanups_ = temp;
    num_alloc_cleanups_ += 10;
  }

  cleanups_[num_cleanups_] = CleanupEntry (wr, f, udata);
  num_cleanups_ ++;
}

/** 
  Condenses the internal \a backups_ array to an effective size. Function is
   called by do_wrappers_deletion() after the deletion has been done. Private.
*/
void WidgetWrapper::condense_backups_() 
{
  /*  Determine effective number of backup entries */
  int n_entries = 0;
  for (int i=0; i < num_backups_; i++)
    if (backups_[i].wr)  n_entries ++;
  
  if (n_entries < num_alloc_backups_ - 10) {
    int n_new = 10 + 10 * (n_entries / 10);  // 9 -> 10, 10 -> 20, 11 -> 20
    BackupEntry* temp = new BackupEntry [n_new];
    
    int k=0;
    for (int i=0; i < num_alloc_backups_; i++)
      if (backups_[i].wr)  
        temp [k ++] = backups_[i];
        
    delete[] backups_;
    backups_ = temp;    
    num_alloc_backups_ = n_new;
    num_backups_ = n_entries;
  }
}

/** 
  Condenses the internal \a cleanups_ array to an effective size. Function is
   called by do_wrappers_deletion() after the deletion has been done. Private.
*/
void WidgetWrapper::condense_cleanups_() 
{
  /*  Determine effective number of cleanup entries */
  int n_entries = 0;
  for (int i=0; i < num_cleanups_; i++)
    if (cleanups_[i].wr)  n_entries ++;
  
  if (n_entries < num_alloc_cleanups_ - 10) {
    int n_new = 10 + 10 * (n_entries / 10);  // 9 -> 10, 10 -> 20, 11 -> 20
    CleanupEntry* temp = new CleanupEntry [n_new];
    
    int k=0;
    for (int i=0; i < num_alloc_cleanups_; i++)
      if (cleanups_[i].wr)  
        temp [k ++] = cleanups_[i];
        
    delete[] cleanups_;
    cleanups_ = temp;    
    num_alloc_cleanups_ = n_new;
    num_cleanups_ = n_entries;
  }
}


/** static WidgetWrapper elements */

int             WidgetWrapper::num_dwrappers_       = 0;
int             WidgetWrapper::num_alloc_dwrappers_ = 0;
WidgetWrapper** WidgetWrapper::dwrappers_           = 0;

int             WidgetWrapper::num_backups_         = 0;
int             WidgetWrapper::num_alloc_backups_   = 0;
WidgetWrapper::BackupEntry*
                WidgetWrapper::backups_             = 0;

int             WidgetWrapper::num_cleanups_        = 0;
int             WidgetWrapper::num_alloc_cleanups_  = 0;
WidgetWrapper::CleanupEntry*
                WidgetWrapper::cleanups_            = 0;

// END OF FILE
