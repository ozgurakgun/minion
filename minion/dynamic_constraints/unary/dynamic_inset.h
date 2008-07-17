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


// Checks if a variable is in a fixed set.
template<typename Var>
  struct WatchInSetConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { return "WatchedInSet"; }

  Var var;

  vector<DomainInt> vals;

  template<typename T>
  WatchInSetConstraint(StateObj* _stateObj, const Var& _var, const T& _vals) :
  AbstractConstraint(_stateObj), var(_var), vals(_vals.begin(), _vals.end())
    { stable_sort(vals.begin(), vals.end()); }

  int dynamic_trigger_count()
    {	return 2; }

  virtual void full_propagate()
  {  
    DynamicTrigger* dt = dynamic_trigger_start();
    var.setMin(vals.front());
    var.setMax(vals.back());

    if(var.isBound())
    {
      // May as well pass DomainRemoval
      var.addDynamicTrigger(dt, DomainChanged);
      propagate(NULL);
    }
    else
    {
      for(int i = 0; i < vals.size() - 1; ++i)
        for(DomainInt pos = vals[i] + 1; pos < vals[i+1]; ++pos)
        var.removeFromDomain(pos);
    }
  }


  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchInSet);
    // If we are in here, we have a bounds variable.
    D_ASSERT(var.isBound());
    // This is basically lifted from "sparse int bound vars"
    vector<DomainInt>::iterator it_low = std::lower_bound(vals.begin(), vals.end(), var.getMin());
    if(it_low == vals.end())
    {
      getState(stateObj).setFailed(true);
    }
    else
    {
      var.setMin(*it_low);
    }

    vector<DomainInt>::iterator it_high = std::lower_bound(vals.begin(), vals.end(), var.getMax());
    if(it_high == vals.end())
    {
      var.setMax(*(it_high - 1));
      return;
    }

    if(*it_high == var.getMax())
      return;

    if(it_high == vals.begin())
    {
      getState(stateObj).setFailed(true);
      return;
    }

    var.setMax(*(it_high - 1));
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 1);
    return binary_search(vals.begin(), vals.end(), v[0]);
  }

  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(1);
    vars.push_back(var);
    return vars;
  }

  virtual void get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {  
    /// TODO: Make faster
    for(int i = 0; i < vals.size(); ++i)
    { 
      if(var.inDomain(vals[i]))
      {
        assignment.push_back(make_pair(0, vals[i]));
        return;
      }
    } 
  }
};

template<typename VarArray1>
AbstractConstraint*
WatchInSetConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const ConstraintBlob& b)
{ 
  return new WatchInSetConstraint<typename VarArray1::value_type>
    (stateObj, _var_array_1[0], b.constants[0]); 
}

BUILD_CONSTRAINT1_WITH_BLOB(CT_WATCHED_INSET, WatchInSetConDynamic)
