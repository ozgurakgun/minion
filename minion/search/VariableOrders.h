/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: VariableOrders.h 751 2007-11-03 14:34:29Z azumanga $
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
void inline maybe_print_search_assignment(StateObj* stateObj, T& var, DomainInt val, BOOL equal, bool force = false)
{
  if(getOptions(stateObj).dumptree)
    {
      if(force)
        cout << "ForceAssign: " << var << (equal?" = ":" != ") << val << endl;
      else
        cout << "SearchAssign:" << var << (equal?" = ":" != ") << val << endl;
    }
}

template<typename VarType = AnyVarRef, typename BranchType = SlowStaticBranch>
struct VariableOrder
{
  StateObj* stateObj;
  vector<VarType> var_order;
  vector<int> val_order;
  unsigned pos;
  vector<int> branches;
  
  BranchType branch_method;
    
  VariableOrder(StateObj* _stateObj, vector<VarType>& _varorder, vector<int>& _valorder): 
    stateObj(_stateObj), var_order(_varorder), val_order(_valorder)
  {
    // if this isn't enough room, the vector will autoresize. While that can be slow,
    // it only has to happen at most the log of the maximum search depth.
    pos = 0; 
  }

  bool find_next_unassigned() { return true; }
  int get_current_pos() { return 0; }
  bool finished_search() { return true; }
  void force_branch_left(int) {;}

  bool assigned_all()
  {
    pos = branch_method(var_order, pos);
    return pos == var_order.size(); //no branches left
  }
  
  void branch_left()
  {
    D_ASSERT(!var_order[pos].isAssigned()) ;
    DomainInt assign_val;
    if(val_order[pos]) {
      assign_val = 0;
    } else {
      assign_val = 1;
    }
    var_order[pos].uncheckedAssign(assign_val);
    maybe_print_search_assignment(stateObj, var_order[pos], assign_val, true);
    branches.push_back(pos);
  }

  void branch_right()
  {  
    if(val_order[pos])
    {
      maybe_print_search_assignment(stateObj, var_order[pos], 0, false);
      var_order[pos].setMin(1);
    }
    else
    {
      maybe_print_search_assignment(stateObj, var_order[pos], 1, false);
      var_order[pos].setMax(0);
    }
    branches.push_back(-1);
  }

  void pop_branches(StateObj* stateObj, int depth)
  {
    int times = branches.size() - depth;
    while(times--) {
      branches.pop_back();
    }
  }

  //return true iff no left branches left to reverse, i.e. finished
  bool pop_to_left_branch_pt(StateObj* stateObj)
  {
    int pops_left = branches.size();
    do {
      pos = branches.back();
      branches.pop_back();
      world_pop(stateObj);
      pops_left--;
    } while(pos == -1 && pops_left > 0);
    return pos == -1;
  }

};

#endif
