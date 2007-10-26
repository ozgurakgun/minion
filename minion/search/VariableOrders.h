/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: VariableOrders.h 701 2007-10-09 14:12:05Z azumanga $
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

#ifndef VARIABLEORDERS_H
#define VARIABLEORDERS_H

#include "search_methods.h"

template<typename T>
void inline maybe_print_search_assignment(StateObj* stateObj, T& var, DomainInt val, BOOL equal)
{
    if(getOptions(stateObj).dumptree)
        cout << "SearchAssign:" << var << (equal?" = ":" != ") << val << endl;
}

template<typename VarType = AnyVarRef, typename BranchType = StaticBranch>
struct VariableOrder
{
  StateObj* stateObj;
  vector<VarType> var_order;
  vector<int> val_order;
  vector<int> branches;
  unsigned pos;
  
  BranchType branch_method;
  
  
  VariableOrder(StateObj* _stateObj, vector<VarType>& _varorder, vector<int>& _valorder)
  : stateObj(_stateObj), var_order(_varorder), val_order(_valorder)
  {
    // if this isn't enough room, the vector will autoresize. While that can be slow,
    // it only has to happen at most the log of the maximum search depth.
	  branches.reserve(var_order.size());
	  pos = 0; 
  }
  
  
  // Returns true if all variables assigned
  bool find_next_unassigned()
  {
	pos = branch_method(var_order, pos);
	return pos == var_order.size();
  }
	  
  bool finished_search()
  { return branches.size() == 0; }
  
  void branch_left()
  {
	D_ASSERT(!var_order[pos].isAssigned()) 
	DomainInt assign_val;
	if(val_order[pos])
	  assign_val = var_order[pos].getMin();
	else
	  assign_val = var_order[pos].getMax();
	var_order[pos].uncheckedAssign(assign_val);
	maybe_print_search_assignment(stateObj, var_order[pos], assign_val, true);
	branches.push_back(pos);
  }
  
  void branch_right()
  {  
	 pos = branches.back();
	 branches.pop_back();
	 
	 if(val_order[pos])
	 {
	   D_ASSERT(var_order[pos].getMax() >= var_order[pos].getMin() + 1);
       maybe_print_search_assignment(stateObj, var_order[pos], var_order[pos].getMin(), false);
	   var_order[pos].setMin(var_order[pos].getMin() + 1);
	 }
	 else
	 {
	   D_ASSERT(var_order[pos].getMax() >= var_order[pos].getMin() + 1);
       maybe_print_search_assignment(stateObj, var_order[pos], var_order[pos].getMax(), false);
	   var_order[pos].setMax(var_order[pos].getMax() - 1);
	 }
  }
};

#endif
