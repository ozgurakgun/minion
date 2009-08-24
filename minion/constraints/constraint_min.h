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

/** @help constraints;max Description
The constraint

   max(vec, x)

ensures that x is equal to the maximum value of any variable in vec.
*/

/** @help constraints;max References
See

   help constraints min

for the opposite constraint.
*/

/** @help constraints;min Description
The constraint

   min(vec, x)

ensures that x is equal to the minimum value of any variable in vec.
*/

/** @help constraints;min References
See

   help constraints max

for the opposite constraint.
*/

#ifndef CONSTRAINT_MIN_H
#define CONSTRAINT_MIN_H

template<typename VarArray, typename MinVarRef>
struct MinConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Min"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type ArrayVarRef;
  
  VarArray var_array;
  MinVarRef min_var;
  
  MinConstraint(StateObj* _stateObj, const VarArray& _var_array, const MinVarRef& _min_var) :
    AbstractConstraint(_stateObj), var_array(_var_array), min_var(_min_var)
  { }

  int dynamic_trigger_count() //watching 2 diseq vars between v_a[i] and min_var s.t. !(v_a[i]=min_var)
  { return 1; }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    
    for(int i = 0; i < var_array.size(); ++i)
    { // Have to add 1 else the 0th element will be lost.
      t.push_back(make_trigger(var_array[i], Trigger(this, i + 1), LowerBound));
      t.push_back(make_trigger(var_array[i], Trigger(this, -(i + 1)), UpperBound));
    }
    t.push_back(make_trigger(min_var, Trigger(this, var_array.size() + 1 ),LowerBound));
    t.push_back(make_trigger(min_var, Trigger(this, -((int)var_array.size() + 1) ),UpperBound));
    Var min_var_v = min_var.getBaseVar();
    for(int i = 0; i < var_array.size(); i++)
      PUSH_DISEQUALITY_TRIGGER(t, var_array[i].getBaseVar(), min_var_v, this, var_array.size() + 2 + i);
    
    return t;
  }
  
  //  virtual AbstractConstraint* reverse_constraint()
  
  virtual void propagate(int prop_val, DomainDelta)
  {
    PROP_INFO_ADDONE(Min);
    if(prop_val > 0 && prop_val < var_array.size() + 2)
    {// Lower Bound Changed

    //Had to add 1 to fix "0th array" problem.
      --prop_val;

      if(prop_val == (int)(var_array.size()))  
      {
        DomainInt new_min = min_var.getMin();
        typename VarArray::iterator end = var_array.end();
        for(typename VarArray::iterator it = var_array.begin(); it < end; ++it)
          (*it).setMin(new_min);
      }
      else
      {
        typename VarArray::iterator it = var_array.begin();
        typename VarArray::iterator end = var_array.end();
        DomainInt min = it->getMin();
        ++it;
        for(; it < end; ++it)
        {
          DomainInt it_min = it->getMin();
          if(it_min < min)
            min = it_min;
        }
        min_var.setMin(min);
      }
    }
    else if(prop_val < 0)
    {// Upper Bound Changed
      // See above for reason behind "-1".
      prop_val = -prop_val - 1;
      if(prop_val == (int)(var_array.size()))
      {
        typename VarArray::iterator it = var_array.begin();
        DomainInt minvar_max = min_var.getMax();
        while(it != var_array.end() && (*it).getMin() > minvar_max)
          ++it;
        if(it == var_array.end())
        {
          getState(stateObj).setFailed(true);
          return;
        }
        // Possibly this variable is the only one that can be the minimum
        typename VarArray::iterator it_copy(it);
        ++it;
        while(it != var_array.end() && (*it).getMin() > minvar_max)
          ++it;
        if(it != var_array.end())
        { // No, another variable can be the minimum
          return;
        }
        it_copy->setMax(minvar_max);
      }
      else
      {
        min_var.setMax(var_array[prop_val].getMax());
      }
    } else {
      //trigger on disequality has fired
      int pos = 0;
      Var min_var_v = min_var.getBaseVar();
      while(pos < var_array.size() && ARE_DISEQUAL(stateObj, var_array[pos].getBaseVar(), min_var_v))
	pos++;
      int first_notdisequal;
      if(pos == var_array.size()) {
	cout << "min DP: everything is disequal so fail" << endl;
	getState(stateObj).setFailed(true);
	return;
      } else
	first_notdisequal = pos;
      pos++;
      while(pos < var_array.size() && ARE_DISEQUAL(stateObj, var_array[pos].getBaseVar(), min_var_v))
	pos++;
      if(pos == var_array.size()) {
	//found exactly one vars_array[i] not known to be disequal to min_var
	cout << "min DP: only one possible v_a=min_var - var_array[" << first_notdisequal << "] = min_var)" << endl;
	SET_EQUAL(stateObj, var_array[first_notdisequal].getBaseVar(), min_var_v);
      }
    }

  }

  //function that returns the next index i after start_pos st. var_array[i] and var_array[i+1]
  //are not known to be equal
  //it will wrap around from index n-2 to 0 and will return -1 if none is found
  int find_not_equal_pos(StateObj* stateObj, int start_pos)
  {
    int v_a_s = var_array.size();
    int i = start_pos;
    while(i + 1 < v_a_s && ARE_EQUAL(stateObj, var_array[i].getBaseVar(), var_array[i+1].getBaseVar()))
      i++;
    if(i + 1 != v_a_s) return i; //found pos, return it
    i = 0;
    while(i < start_pos && ARE_EQUAL(stateObj, var_array[i].getBaseVar(), var_array[i+1].getBaseVar()))
      i++;
    if(i != start_pos) return i;
    else return -1;
  }
    
  virtual void full_propagate()
  {
    int array_size = var_array.size();
    if(array_size == 0)
    {
      getState(stateObj).setFailed(true);
    }
    else
    {
      cout << "in min FP" << endl;
      propagate(array_size + 2, 0); //check if all but one v[i] is disequal to min_val
      cout << "min FP: finished diseq check" << endl;
      if(getState(stateObj).isFailed()) return;
      int n_e_watch_pos = find_not_equal_pos(stateObj, 0);
      if(n_e_watch_pos == -1) {
	cout << "min FP: all var_array equal - setting all equal to min_var" << endl;
	Var min_var_v = min_var.getBaseVar();
	for(int i = 0; i < array_size; i++)
	  SET_EQUAL(stateObj, var_array[i].getBaseVar(), min_var_v);
      } else {
	DynamicTrigger* dt = dynamic_trigger_start();
	dt->trigger_info() = n_e_watch_pos;
	TRIGGER_ON_EQUALITY(var_array[n_e_watch_pos].getBaseVar(), var_array[n_e_watch_pos+1].getBaseVar(), dt);
      }
      for(int i = 1;i <= array_size + 1; ++i)
      {
        propagate(i,0);
        propagate(-i,0);
      }
    }
  }

  virtual void propagate(DynamicTrigger* dt)
  {
    D_ASSERT(dt == dynamic_trigger_start());
    int array_size = var_array.size();
    int n_e_watch_pos = find_not_equal_pos(stateObj, dt->trigger_info());
    if(n_e_watch_pos == -1) {
      cout << "min DP: all var_array equal - setting all equal to min_var" << endl;
      Var min_var_v = min_var.getBaseVar();
      for(int i = 0; i < array_size; i++)
	SET_EQUAL(stateObj, var_array[i].getBaseVar(), min_var_v);
    } else {
      dt->trigger_info() = n_e_watch_pos;
      TRIGGER_ON_EQUALITY(var_array[n_e_watch_pos].getBaseVar(), var_array[n_e_watch_pos+1].getBaseVar(), dt);
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == var_array.size() + 1);
    if(v_size == 1)
      return false;
      
    DomainInt min_val = big_constant;
    for(int i = 0;i < v_size - 1;i++)
      min_val = min(min_val, v[i]);
    return min_val == *(v + v_size - 1);
  }

  // Bah: This could be much better!
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    for(int i = min_var.getMin(); i <= min_var.getMax(); ++i)
    {
      if(min_var.inDomain(i))
      {
        bool flag_domain = false;
        for(int j = 0; j < var_array.size(); ++j)
        {
          if(var_array[j].inDomain(i))
          {
            flag_domain = true;
            assignment.push_back(make_pair(j, i));
          }
          else
          {
            if(var_array[j].getMax() < i)
            {
              return false;
            }
            if(var_array[j].getInitialMin() < i)
              assignment.push_back(make_pair(j, var_array[j].getMax()));
          }
        }
      
        if(flag_domain)
        {
          assignment.push_back(make_pair(var_array.size(), i));
          return true;
        }
        else
          assignment.clear();
      }
    }
    return false;
  }
  
  // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      vector<AnyVarRef> t;
      for(int i=0; i<var_array.size(); i++)
          t.push_back(var_array[i]);
      t.push_back(min_var);
      
      return new CheckAssignConstraint<vector<AnyVarRef>, MinConstraint>(stateObj, t, *this);
  }

  virtual vector<AnyVarRef> get_vars()
  {
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size() + 1);
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    vars.push_back(AnyVarRef(min_var));
    return vars;
  }
};
#endif
