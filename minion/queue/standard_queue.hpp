/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: standard_queue.hpp 745 2007-11-02 13:37:26Z azumanga $
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

// This file contains all the methods of the standard queue class.

  inline DynamicTrigger*& Queues::getNextQueuePtrRef() { return next_queue_ptr; }
  
  inline void Queues::pushSpecialTrigger(SpecialTriggerable* trigger)
  {
      CON_INFO_ADDONE(AddSpecialToQueue);
      special_triggers.push_back(trigger);
  }
  
  
  inline void Queues::pushTriggers(TriggerRange new_triggers)
  { 
	CON_INFO_ADDONE(AddConToQueue);
    D_INFO(1, DI_QUEUE, string("Adding new triggers. Trigger list size is ") + 
		   to_string(propagate_trigger_list.size()) + ".");
	propagate_trigger_list.push_back(new_triggers); 
  }
  
#ifdef DYNAMICTRIGGERS
  inline void Queues::pushDynamicTriggers(DynamicTrigger* new_dynamic_trig_range)
  { 
	CON_INFO_ADDONE(AddDynToQueue);
    D_ASSERT(new_dynamic_trig_range->sanity_check_list());
    dynamic_trigger_list.push_back(new_dynamic_trig_range);   
  }
#endif
  
  
  inline void Queues::clearQueues()
  {
	propagate_trigger_list.clear();
	dynamic_trigger_list.clear();
	
	if(!special_triggers.empty())
	{
	  int size = special_triggers.size();
	  for(int i = 0; i < size; ++i)
		special_triggers[i]->special_unlock();
	  special_triggers.clear();
	}
  }
  
  inline bool Queues::isQueuesEmpty()
  { 
	return propagate_trigger_list.empty() && dynamic_trigger_list.empty() &&
    special_triggers.empty();
  }
  
  
  inline bool Queues::propagateDynamicTriggerLists()
  {
	bool* fail_ptr = getState(stateObj).getFailedPtr();
	while(!dynamic_trigger_list.empty())
	{
	  DynamicTrigger* t = dynamic_trigger_list.back();
	  D_INFO(1, DI_QUEUE, string("Checking queue ") + to_string(t));
	  dynamic_trigger_list.pop_back();
	  DynamicTrigger* it = t->next;
	  
	  while(it != t)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clearQueues();
		  return true; 
		}
#endif
		D_INFO(1, DI_QUEUE, string("Checking ") + to_string(it));
        
#ifdef NO_DYN_CHECK
		DynamicTrigger* next_queue_ptr;
#endif
		next_queue_ptr = it->next;
		D_INFO(1, DI_QUEUE, string("Will do ") + to_string(next_queue_ptr) + " next");
		CON_INFO_ADDONE(DynamicTrigger);
		it->propagate();  

		it = next_queue_ptr;
	  }
	}
	return false;
  }
  
  inline bool Queues::propagateStaticTriggerLists()
  {
	bool* fail_ptr = getState(stateObj).getFailedPtr();
	while(!propagate_trigger_list.empty())
	{
	  TriggerRange t = propagate_trigger_list.back();
	  int data_val = t.data;
	  propagate_trigger_list.pop_back();
	  
	  for(Trigger* it = t.begin(); it != t.end(); it++)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clearQueues();
		  return true; 
		}
#endif
		
#ifndef NO_DEBUG
		if(getOptions(stateObj).fullpropagate)
		  it->full_propagate();
		else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#endif
	  }
	}
	
	return false;
  }
  
  inline void Queues::propagateQueue()
  {
    D_INFO(2, DI_QUEUE, "Starting Propagation");
#ifdef USE_SETJMP
    int setjmp_return = SYSTEM_SETJMP(*(getState(stateObj).getJmpBufPtr()));
	if(setjmp_return != 0)
	{ // Failure has occured
	  D_ASSERT(!getState(stateObj).isFailed());
	  getState(stateObj).setFailed(true);
	  getQueue(stateObj).clearQueues();
	  printf("!\n");
	  return;
	}
#endif
	
	while(true)
	{
#ifdef DYNAMICTRIGGERS
	  if (getState(stateObj).isDynamicTriggersUsed()) 
	  {
		while(!propagate_trigger_list.empty() || !dynamic_trigger_list.empty())
		{
		  if(propagateDynamicTriggerLists())
			return;
		  
		  /* Don't like code duplication here but a slight efficiency gain */
		  if(propagateStaticTriggerLists())
			return;
		}
	  }
	  else
	  {
		if(propagateStaticTriggerLists())
		  return;
	  }
#else
	  if(propagateStaticTriggerLists())
		return;
#endif
	  
	  if(special_triggers.empty())
		return;
	  
	  D_INFO(1, DI_QUEUE, string("Doing a special trigger!"));
	  SpecialTriggerable* trig = special_triggers.back();
	  special_triggers.pop_back();
	  CON_INFO_ADDONE(SpecialTrigger);
	  trig->special_check();

	} // while(true)
	
  } // end Function
  
  
  // ******************************************************************************************
  // Second copy of the propagate queue methods, adapted for the root node only.
  
  inline bool Queues::propagateDynamicTriggerListsRoot()
  {
	bool* fail_ptr = getState(stateObj).getFailedPtr();
	while(!dynamic_trigger_list.empty())
	{
	  DynamicTrigger* t = dynamic_trigger_list.back();
	  D_INFO(1, DI_QUEUE, string("Checking queue ") + to_string(t));
	  dynamic_trigger_list.pop_back();
	  DynamicTrigger* it = t->next;
	  
	  while(it != t)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clearQueues();
		  return true; 
		}
#endif
		D_INFO(1, DI_QUEUE, string("Checking ") + to_string(it));
        
#ifdef NO_DYN_CHECK
		DynamicTrigger* next_queue_ptr;
#endif
		next_queue_ptr = it->next;
        
        if(it->constraint->full_propagate_done)
        {
            D_INFO(1, DI_QUEUE, string("Will do ") + to_string(next_queue_ptr) + " next");
            CON_INFO_ADDONE(DynamicTrigger);
            it->propagate();
        }
        
		it = next_queue_ptr;
	  }
	}
	return false;
  }
  
  inline bool Queues::propagateStaticTriggerListsRoot()
  {
	bool* fail_ptr = getState(stateObj).getFailedPtr();
	while(!propagate_trigger_list.empty())
	{
	  TriggerRange t = propagate_trigger_list.back();
	  int data_val = t.data;
	  propagate_trigger_list.pop_back();
	  
	  for(Trigger* it = t.begin(); it != t.end(); it++)
	  {
#ifndef USE_SETJMP
		if(*fail_ptr) 
		{
		  clearQueues();
		  return true; 
		}
#endif
		if(it->constraint->full_propagate_done)
        {
#ifndef NO_DEBUG
		if(getOptions(stateObj).fullpropagate)
		  it->full_propagate();
		else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#else
		{
		  CON_INFO_ADDONE(StaticTrigger);
		  it->propagate(data_val);
		}
#endif
        }
	  }
	}
	
	return false;
  }
  
  inline void Queues::propagateQueueRoot()
  {
    D_INFO(2, DI_QUEUE, "Starting Propagation at root");
#ifdef USE_SETJMP
    int setjmp_return = SYSTEM_SETJMP(*(getState(stateObj).getJmpBufPtr()));
	if(setjmp_return != 0)
	{ // Failure has occured
	  D_ASSERT(!getState(stateObj).isFailed());
	  getState(stateObj).setFailed(true);
	  getQueue(stateObj).clearQueues();
	  printf("!\n");
	  return;
	}
#endif
	
	while(true)
	{
#ifdef DYNAMICTRIGGERS
	  if (getState(stateObj).isDynamicTriggersUsed()) 
	  {
		while(!propagate_trigger_list.empty() || !dynamic_trigger_list.empty())
		{
		  if(propagateDynamicTriggerListsRoot())
			return;
		  
		  /* Don't like code duplication here but a slight efficiency gain */
		  if(propagateStaticTriggerListsRoot())
			return;
		}
	  }
	  else
	  {
		if(propagateStaticTriggerListsRoot())
		  return;
	  }
#else
	  if(propagateStaticTriggerListsRoot())
		return;
#endif
	  
	  if(special_triggers.empty())
		return;
	  
	  D_INFO(1, DI_QUEUE, string("Doing a special trigger!"));
	  SpecialTriggerable* trig = special_triggers.back();
	  special_triggers.pop_back();
	  CON_INFO_ADDONE(SpecialTrigger);
	  trig->special_check();

	} // while(true)
	
  } // end Function
