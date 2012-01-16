/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
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



#ifndef CONSTRAINT_TEST_H
#define CONSTRAINT_TEST_H

template<typename VarArray>
struct TestConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Test"; }
  
   typedef typename VarArray::value_type ArrayVarRef;
  
   array<ArrayVarRef,10> var_array;
  
  TestConstraint(StateObj* _stateObj, const VarArray& _var_array) :
    AbstractConstraint(_stateObj)
  {
      for(int i=0; i<_var_array.size(); i++)
      {
          var_array[i] = _var_array[i];
      }
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < var_array.size(); ++i)
    { // Have to add 1 else the 0th element will be lost.
      //t.push_back(make_trigger(var_array[i], Trigger(this, 2*i), LowerBound));
      //t.push_back(make_trigger(var_array[i], Trigger(this, 2*i+1), UpperBound));
      t.push_back(make_trigger(var_array[i], Trigger(this, 2*i+1), DomainChanged));
    }
    
    return t;
  }
  
  
  virtual void propagate(int lit, DomainDelta)
  {

   full_propagate();
  }
  
    
  virtual void full_propagate()
  {
#include "generated_constraint_code.h"
      // life 
    //#include "generated_life_ct.h"

      
      // still life
//#include "generated_stilllife_ct.h"

//#include "generated_labs_ct.h"
//#include "generated_alldiff3_ct.h"
          
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
   return true;// return (std::min(v[0], v[1]) == v[2]);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;
  }
};
#endif
