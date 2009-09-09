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

/** @help constraints;w-inrange Description
  The constraint w-inrange(x, [a,b]) ensures that a <= x <= b.
*/

/** @help constraints;w-inrange References
  See also

  help constraints w-notinrange
*/

#ifndef CONSTRAINT_DYNAMIC_UNARY_INRANGE_H
#define CONSTRAINT_DYNAMIC_UNARY_INRANGE_H

// Checks if a variable is in a fixed Range.
template<typename Var>
  struct WatchInRangeConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "WatchedInRange"; }

  Var var;

  DomainInt range_min;
  DomainInt range_max;

  template<typename T>
  WatchInRangeConstraint(StateObj* _stateObj, const Var& _var, const T& _vals) :
  AbstractConstraint(_stateObj), var(_var)
  { 
    if(_vals.size() != 2)
    {
      cerr << "The range of an 'inrange' constraint must contain 2 values!" << endl;
      abort();
    }
    
    range_min = _vals[0];
    range_max = _vals[1];
  }

  int dynamic_trigger_count()
    { return 2; }

  virtual BOOL full_propagate()
  {  
    if(!var.setMin(range_min))
        return false;
    return var.setMax(range_max);
  }


  virtual BOOL propagate(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInRange);
    D_FATAL_ERROR("Propagation is never called for 'in range'");
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 1);
    return (v[0] >= range_min && v[0] <= range_max);
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
    /// TODO: Make faster
    int min_val = max(range_min, var.getMin());
    int max_val = min(range_max, var.getMax());
    for(int i = min_val; i <= max_val; ++i)
    { 
      if(var.inDomain(i))
      {
        assignment.push_back(make_pair(0, i));
        return true;
      }
    }
    return false;
  }
};
#endif
