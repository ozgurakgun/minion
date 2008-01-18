/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/* Minion
* Copyright (C) 2006
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


class DynamicTrigger;

class Queues
{
  StateObj* stateObj;
  
  vector<TriggerRange> propagate_trigger_list;
  vector<DynamicTrigger*> dynamic_trigger_list;
  
  // Special triggers are those which can only be run while the
  // normal queue is empty. This list is at the moment only used
  // by reified constraints when they want to start propagation.
  // I don't like it, but it is necesasary.
  vector<SpecialTriggerable *> special_triggers;
  
#ifndef NO_DYN_CHECK
  DynamicTrigger* next_queue_ptr;
#endif
  
public:
    
  DynamicTrigger*& getNextQueuePtrRef();
  
  Queues(StateObj* _stateObj) : next_queue_ptr(NULL), stateObj(_stateObj)
  {}
  
  void pushSpecialTrigger(SpecialTriggerable* trigger);
  
  inline void pushTriggers(TriggerRange new_triggers);
  
#ifdef DYNAMICTRIGGERS
  void pushDynamicTriggers(DynamicTrigger* new_dynamic_trig_range);
#endif
  
  void clearQueues();
  
  bool isQueuesEmpty();
  
  bool propagateDynamicTriggerLists();
  
  bool propagateStaticTriggerLists();
  
  inline void propagateQueue();
};  

// This just allows SAC (which wants a list of vars)
// and normal propagate to have the same input method.
// Just checking the bounds doesn't make sense here, so we ignore it.
//template<typename Vars>
//inline void propagate_queue_vars(StateObj* stateObj, Vars& vars, bool /*CheckBounds*/)
//{	getQueue(stateObj).propagateQueue(); }
