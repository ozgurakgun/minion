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

#ifndef TRIGGERLIST_H
#define TRIGGERLIST_H

class TriggerList;

struct triggerBacktrackRecord
{
    // The trigger pointer, and the queue to put it back into.
    DynamicTrigger* trigger;
    DynamicTrigger* queue;
};

class TriggerMem
{
  vector<TriggerList*> trigger_lists;  // indexed by what? variable?
  char* triggerlist_data;
  StateObj* stateObj;
  
#ifdef MIXEDTRIGGERS
  // declare the backtrack stack and stack height.
  int _local_depth;             // could be unsigned 
  ReversibleInt _backtrack_depth;

  vector<triggerBacktrackRecord> _backtracking_stack;
#endif
  
public:
  void addTriggerList(TriggerList* t) 
  { trigger_lists.push_back(t); }
  
  void finaliseTriggerLists();
  
  
  #ifdef MIXEDTRIGGERS
  TriggerMem(StateObj* _stateObj) : triggerlist_data(NULL), stateObj(_stateObj),
    _local_depth(0), _backtrack_depth(0)
  {
  _backtracking_stack.resize(10000);
  }
  #else
  TriggerMem(StateObj* _stateObj) : triggerlist_data(NULL), stateObj(_stateObj)
  { }
  #endif
  
  void allocateTriggerListData(unsigned mem)
  {
    D_ASSERT(triggerlist_data == NULL);
    triggerlist_data = new char[mem];
  }
  
  char* getTriggerListDataPtr() { return triggerlist_data; }
  ~TriggerMem()
  { delete[] triggerlist_data; }
  
#ifdef MIXEDTRIGGERS
  void undo()
  {
    int bt_depth = _backtrack_depth;
    
    //D_ASSERT( _local_depth < (_max_depth+1) && _local_depth >= bt_depth && bt_depth >=0);
    //cout << "In the undo function. local depth:"<< _local_depth<< " _backtrack_depth:" << bt_depth <<endl;
    while(_local_depth > bt_depth)
    {
      -- _local_depth;
      // Perform undo here.
      triggerBacktrackRecord& record= _backtracking_stack[_local_depth];
      // Might have to check if the queue is null, and if so, do a sleep().
      if(record.queue==NULL)
      {
          (record.trigger)->sleepDynamicTrigger(stateObj);  // does unneccessary check of the global next ptr.
      }
      else
      {
          (record.trigger)->add_after(record.queue, stateObj);
      }
      record.trigger->queue=record.queue;
      D_DATA(cout << "Backtracked a dynamic trigger: "<< (int)(record.trigger) << "info:" << record.trigger->trigger_info() <<endl);
    }
    
    D_ASSERT(_local_depth == bt_depth);
  }
  
  inline void add_bt_record(DynamicTrigger* t, DynamicTrigger* queue)
  {
      //cout << "In add_bt_record"<<endl;
      if(_local_depth==_backtracking_stack.size())
      {
          // This faffing is to avoid the need to create an object and push_back
          // it, because I'm not sure that the object creation will be optimized away.
          _backtracking_stack.resize(_backtracking_stack.size()*2);
      }
      triggerBacktrackRecord& record=_backtracking_stack[_local_depth];
      record.trigger=t;
      record.queue=queue;
      _local_depth++;
  }
  
  void before_branch_left()
  { _backtrack_depth = _local_depth; }
  
#endif
  
};

class TriggerList
{
  StateObj* stateObj;
  
  TriggerList(const TriggerList&);
  void operator=(const TriggerList&);
  bool only_bounds;
  

public:
	TriggerList(StateObj* _stateObj, bool _only_bounds) : stateObj(_stateObj), 
	only_bounds(_only_bounds)
  {
	  var_count_m = 0;
	  lock_first = lock_second = 0; 
  }
  
  vector<vector<vector<Trigger> > > triggers;
  
#ifdef DYNAMICTRIGGERS
  MemOffset dynamic_triggers;
#endif
  
  Trigger** trigger_data_m;
  
  int var_count_m;
  int lock_first;
  int lock_second;
  
  DomainInt vars_min_domain_val;
  DomainInt vars_max_domain_val;
  unsigned vars_domain_size;
  
  void lock(int size, DomainInt min_domain_val, DomainInt max_domain_val)
  {
	D_ASSERT(!lock_first && !lock_second);
	lock_first = true;
	var_count_m = size;
	vars_min_domain_val = min_domain_val;
	vars_max_domain_val = max_domain_val;
	vars_domain_size = checked_cast<unsigned>(max_domain_val - min_domain_val + 1);
	
	triggers.resize(4);
	for(unsigned i = 0; i < 4; ++i)
	  triggers[i].resize(var_count_m);
	
#ifdef DYNAMICTRIGGERS
    if(only_bounds)
      dynamic_triggers = getMemory(stateObj).nonBackTrack().request_bytes(size * sizeof(DynamicTrigger) * 4);
    else
      dynamic_triggers = getMemory(stateObj).nonBackTrack().request_bytes(size * sizeof(DynamicTrigger) * (4 + vars_domain_size));
#else
	if(only_bounds)
  	  dynamic_triggers = getMemory(stateObj).backTrack().request_bytes(size * sizeof(DynamicTrigger) * 4);  
	else
	  dynamic_triggers = getMemory(stateObj).backTrack().request_bytes(size * sizeof(DynamicTrigger) * (4 + vars_domain_size));
#endif
	getTriggerMem(stateObj).addTriggerList(this);
  }
  
  size_t memRequirement()
  {
	D_ASSERT(lock_first && !lock_second);
	size_t storage = 0;
	for(unsigned i = 0; i < 4; ++i)
	{
	  for(unsigned j = 0; j < triggers[i].size(); ++j)
	    storage += triggers[i][j].size();
	}
	return storage * sizeof(Trigger) + 4 * (var_count_m + 1) * sizeof(Trigger*);
  }
  
  void allocateMem(char* mem_start)
  {
	D_ASSERT(lock_first && !lock_second);
	lock_second = true;

	Trigger** trigger_ranges = (Trigger**)(mem_start);
	trigger_data_m = trigger_ranges;
	Trigger* trigger_data = (Trigger*)(mem_start + 4 * (triggers[UpperBound].size() + 1) * sizeof(Trigger*));
	
	for(unsigned int type = 0; type < 4; ++type)
	{
	  for(unsigned int i = 0; i < triggers[type].size(); ++i)
	  {
		*trigger_ranges = trigger_data;
		++trigger_ranges;
		for(unsigned int j = 0; j < triggers[type][i].size(); ++j)
		{
		  *trigger_data = triggers[type][i][j];
		  trigger_data++;
		}
	  }
	  *trigger_ranges = trigger_data;
	  ++trigger_ranges;
	}
	
	D_ASSERT(static_cast<void*>(mem_start + 4 * (var_count_m + 1) * sizeof(Trigger*)) ==
			 static_cast<void*>(trigger_ranges));
	
	// This is a common C++ trick to completely free the memory of an object.
	{ 
	  vector<vector<vector<Trigger> > > t; 
	  triggers.swap(t);
	}
	
#ifdef DYNAMICTRIGGERS
	DynamicTrigger* trigger_ptr = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr());
	
	int trigger_types = ( only_bounds ? 4 : (4 + vars_domain_size));
	for(unsigned i = 0; i < var_count_m * trigger_types; ++i)
	{
	  new (trigger_ptr + i) DynamicTrigger;
	  D_ASSERT((trigger_ptr + i)->sanity_check_list());
	}
#endif
  }
  
  
  pair<Trigger*, Trigger*> get_trigger_range(int var_num, TrigType type)
  {
    Trigger** first_trig = trigger_data_m + var_num + (var_count_m + 1) * type;
	Trigger* trig_range_start = *first_trig;
	first_trig++;
	Trigger* trig_range_end = *first_trig;
	return pair<Trigger*,Trigger*>(trig_range_start, trig_range_end);
  }
  
#ifdef DYNAMICTRIGGERS
  void dynamic_propagate(int var_num, TrigType type, DomainInt val_removed = -999)
  {
    D_ASSERT(val_removed == -999 || ( type == DomainRemoval && val_removed != -999) );
	D_ASSERT(!only_bounds || type != DomainRemoval);
	DynamicTrigger* trig;
	if(type != DomainRemoval)
	{
	  trig = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			 + var_num + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val_removed);
	  D_ASSERT(vars_max_domain_val >= val_removed);
	  trig = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())
			 + checked_cast<int>(var_num + (DomainRemoval + (val_removed - vars_min_domain_val)) * var_count_m);
	}
    D_ASSERT(trig->next != NULL);
    // This is an optimisation, no need to push empty lists.
    if(trig->next != trig)
	  getQueue(stateObj).pushDynamicTriggers(trig);
  }
#endif
  
  void push_upper(int var_num, DomainInt upper_delta)
  {
#ifdef DYNAMICTRIGGERS
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, UpperBound);
#endif
	D_ASSERT(lock_second);
    D_ASSERT(upper_delta > 0 || getState(stateObj).isFailed());
	
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, UpperBound);
	if (range.first != range.second)
	  getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, 
											 checked_cast<int>(upper_delta)));
  }
  
  void push_lower(int var_num, DomainInt lower_delta)
  { 
#ifdef DYNAMICTRIGGERS
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, LowerBound);
#endif
	D_ASSERT(lock_second);
	D_ASSERT(lower_delta > 0 || getState(stateObj).isFailed());
    
	pair<Trigger*, Trigger*> range = get_trigger_range(var_num, LowerBound);
	if (range.first != range.second)
	  getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, 
											 checked_cast<int>(lower_delta)));
  }
  
  
  void push_assign(int var_num, DomainInt)
  { 
#ifdef DYNAMICTRIGGERS
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, Assigned);
#endif
    D_ASSERT(lock_second);
    
    pair<Trigger*, Trigger*> range = get_trigger_range(var_num, Assigned);
	if (range.first != range.second)
	  getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, -1));
  }
  
  void push_domain(int var_num)
  { 
#ifdef DYNAMICTRIGGERS
    if (getState(stateObj).isDynamicTriggersUsed()) dynamic_propagate(var_num, DomainChanged);
#endif
	
	D_ASSERT(lock_second);
	pair<Trigger*, Trigger*> range = get_trigger_range(var_num, DomainChanged);
	if (range.first != range.second)	  
	  getQueue(stateObj).pushTriggers(TriggerRange(range.first, range.second, -1));
  }
  
  void push_domain_removal(int var_num, DomainInt val_removed)
  { 
	D_ASSERT(!only_bounds);
#ifdef DYNAMICTRIGGERS
    dynamic_propagate(var_num, DomainRemoval, val_removed);
#endif
	D_ASSERT(lock_second);
  }
  
  void add_domain_trigger(int b, Trigger t)
  { 
	D_ASSERT(!only_bounds);
	D_ASSERT(lock_first && !lock_second); 
	triggers[DomainChanged][b].push_back(t); 
  }
  
  void add_trigger(int b, Trigger t, TrigType type)
  {
	D_ASSERT(type != DomainRemoval)
	D_ASSERT(lock_first && !lock_second);
	//if(type != DomainRemoval)
  	  triggers[type][b].push_back(t);
	//else
	//  triggers[type + val - vars_min_domain_val][b].push_back(t);
  }
  
#ifdef DYNAMICTRIGGERS
  void addWatchTrigger(int b, DynamicTrigger* t, TrigType type, DomainInt val)
  {
    D_INFO(1, DI_QUEUE, "Adding Dynamic Trigger");
	D_ASSERT(lock_second);
	D_ASSERT(!only_bounds || type != DomainRemoval);
	D_ASSERT(t->constraint != NULL);
	D_ASSERT(t->sanity_check == 1234);
	// This variable is only use in debug mode, and will be optimised away at any optimisation level.
	DynamicTrigger* old_list;
	old_list = t->next;
	DynamicTrigger* queue;
    int offset;
	if(type != DomainRemoval)
	{
	  offset= b + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val);
	  D_ASSERT(vars_max_domain_val >= val);
	  offset= checked_cast<int>(b + (DomainRemoval + (val - vars_min_domain_val)) * var_count_m);
	}
    queue = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())+offset;
    
	D_ASSERT(queue->sanity_check_list());
    
	t->add_after(queue, stateObj);
	D_ASSERT(old_list == NULL || old_list->sanity_check_list(false));
  }
  
  #ifdef MIXEDTRIGGERS
  void addDynamicTrigger(int b, DynamicTrigger* t, TrigType type, DomainInt val)
  {
    D_INFO(1, DI_QUEUE, "Adding Dynamic Trigger");
	D_ASSERT(lock_second);
	D_ASSERT(!only_bounds || type != DomainRemoval);
	D_ASSERT(t->constraint != NULL);
	D_ASSERT(t->sanity_check == 1234);
	// This variable is only use in debug mode, and will be optimised away at any optimisation level.
	DynamicTrigger* old_list;
	old_list = t->next;
	DynamicTrigger* queue;
    int offset;
	if(type != DomainRemoval)
	{
	  offset= b + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val);
	  D_ASSERT(vars_max_domain_val >= val);
	  offset= checked_cast<int>(b + (DomainRemoval + (val - vars_min_domain_val)) * var_count_m);
	}
    queue = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())+offset;
    // Store the queue.
    t->queue=queue;
    
	D_ASSERT(queue->sanity_check_list());
    
	t->add_after(queue, stateObj);
	D_ASSERT(old_list == NULL || old_list->sanity_check_list(false));
  }
  
  void addDynamicTriggerBT(int b, DynamicTrigger* t, TrigType type, DomainInt val)
  {
      // b is variable number
    D_INFO(1, DI_QUEUE, "Adding Dynamic Trigger");
	D_ASSERT(lock_second);
	D_ASSERT(!only_bounds || type != DomainRemoval);
	D_ASSERT(t->constraint != NULL);
	D_ASSERT(t->sanity_check == 1234);
	// This variable is only use in debug mode, and will be optimised away at any optimisation level.
	DynamicTrigger* old_list;
	old_list = t->next;
    // Make an offset and store that in the dynamic trigger for backtracking, each time the trigger is moved.
	DynamicTrigger* queue;
    
    int offset;
	if(type != DomainRemoval)
	{
	  offset= b + type*var_count_m;
	}
	else
	{
	  D_ASSERT(!only_bounds);
	  D_ASSERT(vars_min_domain_val <= val);
	  D_ASSERT(vars_max_domain_val >= val);
	  offset= checked_cast<int>(b + (DomainRemoval + (val - vars_min_domain_val)) * var_count_m);
	}
    queue = static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr())+offset;
    
    getTriggerMem(stateObj).add_bt_record(t, t->queue);   // Add backtrack record before changing queue.
    
    t->queue=queue;
    
	D_ASSERT(queue->sanity_check_list());
    
    t->add_after(queue, stateObj);
	D_ASSERT(old_list == NULL || old_list->sanity_check_list(false));
  }
  
  void findTriggerPosition(TrigType & type, DynamicTrigger* t, int & varnum, DomainInt & val)
  {
      // This function is mainly untested, be careful.
      DynamicTrigger* queue=t->queue;
      // Should really checke that the trigger is actually in the queue that 
      // queue points to.
      int offset=(int)(queue-static_cast<DynamicTrigger*>(dynamic_triggers.get_ptr()));
      if(offset < 4*var_count_m)
      {
          type=(TrigType) (offset/var_count_m);
          varnum=offset%var_count_m;
      }
      else
      {
          type=DomainRemoval;
          offset=offset-DomainRemoval*var_count_m;
          
          // now offset is the start fo the block for DomainRemoval type.
          
          // offset == varnum + (val-vars_min_domain_val)*var_count_m
          
          varnum=offset % var_count_m;
          
          val=(DomainInt)(((offset-varnum)/var_count_m)+vars_min_domain_val);
      }
      
  }
  #endif
#endif
  
};

void inline TriggerMem::finaliseTriggerLists()
  {
	size_t trigger_size = 0;
	for(unsigned int i = 0;i < trigger_lists.size(); i++)
	  trigger_size += trigger_lists[i]->memRequirement();
	getTriggerMem(stateObj).allocateTriggerListData(trigger_size);
	
	char* triggerlist_offset = getTriggerMem(stateObj).getTriggerListDataPtr();
    
	for(unsigned int i=0;i<trigger_lists.size();i++)
	{
	  size_t offset = trigger_lists[i]->memRequirement();
	  trigger_lists[i]->allocateMem(triggerlist_offset);
	  triggerlist_offset += offset;
	}
	D_ASSERT(triggerlist_offset - getTriggerMem(stateObj).getTriggerListDataPtr() == (int)trigger_size);
  }


#endif //TRIGGERLIST_H

