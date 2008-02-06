/**************************************************************************************[VarOrder.h]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef VarOrder_h
#define VarOrder_h

#include "SolverTypes.h"
#include "Heap.h"


//=================================================================================================

class VarOrder {
    const vec<int>&     levels;      // var->val. Pointer to external table.
    const vec<int>&     BMS_dval;    // depth->val. Pointer to external table.
    const vec<int>&     BMS_setval;  // var->val. Pointer to external table.
    const vec<double>&  activity;    // var->act. Pointer to external activity table.
    vec<int>            sorted;      // array of Var sorted by activity
    vec<int>            Var_to_pos;  // position of index Var in <sorted>
    double              random_seed; // For the internal random number generator

public:
    VarOrder(const vec<int>& levs, const vec<int>& _BMS_dval, 
	     const vec<int>& _BMS_setval, const vec<double>& act) :
        levels(levs), BMS_dval(_BMS_dval), BMS_setval(_BMS_setval), 
      activity(act), sorted(), Var_to_pos(), random_seed(91648253)
      { }

    inline void newVar(void);
    inline void update(Var x);                  // Called when variable increased in activity.
    inline void undo(Var x);                    // Called when variable is unassigned and may be selected again.
    inline Var  select(double random_freq =.0); // Selects a new, unassigned variable (or 'var_Undef' if none exists).
};


void VarOrder::update(Var x) //insert updated variable to correct place in sorted vector
{
  int pos = Var_to_pos[x];
  while(activity[sorted[pos - 1]] > activity[sorted[pos]]) {
    sorted[pos] = sorted[pos - 1];
    pos--;
  }
  sorted[pos] = x;
  Var_to_pos[x] = pos;
}

void VarOrder::newVar(void)
{
  Var nv = BMS_setval.size() - 1;
  sorted.push(nv);
  Var_to_pos.push(nv);
  update(nv);
}

Var VarOrder::select(double random_var_freq)
{
    // Random decision:
    if (drand(random_seed) < random_var_freq){
        Var next = irand(random_seed, sorted.size());
        if (BMS_setval[next] != BMS_dval[levels[next]]) //make sure it's not assigned already
            return next;
    }

    // Activity based decision:
    for(int i = 0; i < sorted.size(); i++)
      if(BMS_setval[i] != BMS_dval[levels[i]]) //not assigned
	return i;
    return var_Undef; //all assigned
}


//=================================================================================================
#endif
