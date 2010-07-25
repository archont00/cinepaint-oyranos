/*
 * ProgressInfo.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ProgressInfo.hpp  -  Abstract base class ProgressInfo.
*/
#ifndef ProgressInfo_hpp
#define ProgressInfo_hpp


#include <cmath>        // fabsf()
#include <cstdio>       // printf (debug)

/**===========================================================================

  @class ProgressInfo  
  
  ProgressInfo is the abstract base class of our concrete progress-info classes.
   The abstract ProgressInfo base class shall be available in (e.g. numerical)
   algorithms without referencing to any concrete GUI. A specific GUI must to 
   be implementable subsequently -- by deriving from this base class and
   overwriting the virtual functions. The pre-sequent numerical algorithms ply
   either then with a global abstract ProgressInfo pointer (no object) or such
   pointer is handed over as argument. In the superior GUI layer this abstract
   pointer is to point "later" to a suitably derived concrete ProgressInfo object.
    
  The following virtual (mostly abstract) functions are to overwrite by heirs:
   - start(float)   // non-abstract
   - start(float, const char*)
   - text(const char*)
   - finish().
   - flush_value(float)  // protected
   
  @note Each start() function in a derived class overwriting start(float vmax)
   or start(float vmax, const char*) must call ProgressInfo::start(float vmax)
   to update the private base variables \a vmax_, \a v_ and \a v_last_flush_!
   
  ProgressInfo comes with three \a value functions: the two non-virtual, public
   functions value() and direct_value(), and the virtual, protected flush_value(). 
   
  flush_value(float v) is to define by heirs. It shall output a progress info
   for value \a v, and it is assumed that it flushes the output more or less
   immediately on the screen. flush_value() is not intended for calling by users
   directly and should therefore declared by heirs \a protected as well. 
  
  @warning <b>A direct call would bypass variable updating in the base class 
   ProgressInfo!</b>
   
  For such direct calls we provide direct_value(), which calls simply flush_value()
   and updates before private base variables.  
  
  On the other hand, value(float v) is a "smart" value function, it calls
   flush_value() only if the argument \a v differs at least by \a dv_min_flush_
   from the last flushed value. \a dv_min_flush_ can be set appropriately via
   delta_min_flush(float dv). This allows to avoid too frequently flushes in
   opaque looping situations and disburdens the user from this worry. 
   Example: Be \a W the width of a progress bar widget; the setting 
   <i>dv_min_flush_ = 1 * maximum() / W</i> causes, that progress values
   get flushed not until they change the bar by at least one pixel. For 
   <i>dv_min_flush_ = 0</i> each value() enforces a flush_value() call. 
   The Ctor default setting is 0.
  
  @note As minimum progress value we assume general 0. The maximum value \a vmax_
   can be set arbitrarily via start(), the Ctor default setting is 1.0.
   
  @note Wenn zwischen start() und finish() eine Ausnahme auftritt, ist im
   catch()-Teil ein finish() nicht schlecht. Sonst steht der Balken womoeglich
   bis zum naechsten start().
  
  @note Beim Gebrauch eines ueberantworteten ProgressInfo Zeigers \a pr ist durch
   <tt>`if (pr)...'</tt> der Nichtinitialisierungsfall stets abzufangen! Fuer einen
   globalen Zeiger uebernimmt diesen Dienst die Klasse TheProgressInfo.
     
  @todo Idee: Zwischen zwei flush_value()s koennte die Zeit genommen und je
   nachdem \a dv_min_flush_ angehoben oder verringert werden. 
   Fortschritts-Infos wuerden feiner, wenn etwas laenger dauert und grober,
   wenn's schnell geht. Wenn ein reiner Balken die Info, ist die Aenderung
   um einen Pixel eine geeignete untere Schranke und Zeitnahme weniger wichtig,
   wenn aber (auch) z.B. Prozentzahlen ausgegeben werden, ist die Zeit das Mass.
   
  @todo Zu ueberlegen, ob die virtuelle <i>start(float vmax, const char* txt)</i>
   nicht als <tt>{text(txt); start(vmax);}</tt> vorkodiert wird. Insbesondere
   wenn text() bei den Erben nicht "flushed", ist es genau das, was auch sonst
   passiert.
  
=============================================================================*/

class ProgressInfo 
{
  float vmax_;          // max value (==100%); for min value we assume 0.
  float v_;             // current value (needed for forward())
  float v_last_flush_;  // last flushed value via flush_value()
  float dv_min_flush_;  // minimum value difference between flushes

protected:
  //  Ctor protected to avoid attempted instances of abstract class
  ProgressInfo() 
     : vmax_(1.0f), v_(0.0f), v_last_flush_(0.0f), dv_min_flush_(0.0f)
    {}
  virtual void flush_value (float v) = 0;
  
public:
  virtual ~ProgressInfo() {}  // avoids warnings concerning virtual fncts
  
  virtual void start (float vmax)       {vmax_= vmax; v_= v_last_flush_= 0.0f;}
  virtual void start (float vmax, const char* txt) = 0;//{text(txt); start(vmax);}
  virtual void finish() = 0;
  virtual void text (const char*) = 0;
  
  float value() const                   {return v_;}
  float maximum() const                 {return vmax_;}
  void  forward (float dv)              {value (v_ + dv);}
  void  direct_value (float v)          {v_=v; v_last_flush_=v; flush_value(v);}
  void  delta_min_flush (float dv)      {dv_min_flush_ = dv;}
  float delta_min_flush() const         {return dv_min_flush_;}
  
  /**
    Smart value() function: calls flush_value() only if \a v differs from the
     last flushed value by at least \a dv_min_flush_. "1e-7" concerns finite
     float precision.
  */
  void  value (float v) {
      v_ = v;  // for forward()
      if (fabsf(v - v_last_flush_) >= dv_min_flush_ + 1e-7f) {
        v_last_flush_ = v;
        flush_value (v);
      }
      //else printf("\t%f - zu klein (diff = %f)\n", v, v - v_last_flush_);
    }  
};


#endif  // ProgressInfo_hpp

// END OF FILE
