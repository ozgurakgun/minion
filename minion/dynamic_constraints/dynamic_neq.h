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


class WNCCompData : public ConCompData
{
public:
  Var var1;
  Var var2;
  
  WNCCompData(Var _var1, Var _var2) : var1(_var1), var2(_var2) {}
};

template<typename Var1, typename Var2>
struct WatchNeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "WatchedNEQ"; }
  
  Var1 var1;
  Var2 var2;

  WatchNeqConstraint(StateObj* _stateObj, const Var1& _var1, const Var2& _var2) :
	AbstractConstraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  int dynamic_trigger_count()
  {	return 2; }
  
  virtual void full_propagate()
  {  
  	DynamicTrigger* dt = dynamic_trigger_start();
	
	  if(var1.isAssigned() && var2.isAssigned() && var1.getAssignedValue() == var2.getAssignedValue())
	  {
	    //the following will cause a DWO
	    storeExpl(false, var2, var1.getAssignedValue(), VirtConPtr(new WatchNeqPrunRight<Var1,Var2>(this, var1.getAssignedValue())));
	    var1.removeFromDomain(var1.getAssignedValue());
	    return;
	  }
	  
	  if(var1.isAssigned())
	  {
	    if(var2.inDomain(var1.getAssignedValue()))
	      storeExpl(false, var2, var1.getAssignedValue(), VirtConPtr(new WatchNeqPrunRight<Var1,Var2>(this, var1.getAssignedValue())));
	    var2.removeFromDomain(var1.getAssignedValue());
	    return;
	  }
	  
	  if(var2.isAssigned())
	  {
	    if(var1.inDomain(var2.getAssignedValue()))
	      storeExpl(false, var1, var2.getAssignedValue(), VirtConPtr(new WatchNeqPrunLeft<Var1,Var2>(this, var2.getAssignedValue())));
	    var1.removeFromDomain(var2.getAssignedValue());
	    return;
	  }
	  
    var1.addDynamicTrigger(dt    , Assigned);
    var2.addDynamicTrigger(dt + 1, Assigned);
  }
  
    
  DYNAMIC_PROPAGATE_FUNCTION(DynamicTrigger* dt)
  {
	  PROP_INFO_ADDONE(WatchNEQ);
	  DynamicTrigger* dt_start = dynamic_trigger_start();
	  
    D_ASSERT(dt == dt_start || dt == dt_start + 1);
    
	  if(dt == dt_start)
	  {
	    D_ASSERT(var1.isAssigned());
	    if(var2.inDomain(var1.getAssignedValue()))
	      storeExpl(false, var2, var1.getAssignedValue(), VirtConPtr(new WatchNeqPrunRight<Var1,Var2>(this, var1.getAssignedValue())));
	    var2.removeFromDomain(var1.getAssignedValue());
	  }
	  else
	  {
	    D_ASSERT(var2.isAssigned());
	    if(var1.inDomain(var2.getAssignedValue()))
	      storeExpl(false, var1, var2.getAssignedValue(), VirtConPtr(new WatchNeqPrunLeft<Var1,Var2>(this, var2.getAssignedValue())));
	    var1.removeFromDomain(var2.getAssignedValue());
	  }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return v[0] != v[1];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	  vars.reserve(2);
    vars.push_back(var1);
    vars.push_back(var2);
    return vars;
  }
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(var1.isAssigned() && var2.isAssigned() && var1.getAssignedValue() == var2.getAssignedValue())
      return false;
    
    if(var1.isAssigned())
    {
      assignment.push_back(make_pair(0, var1.getAssignedValue()));
      if(var2.getMin() != var1.getAssignedValue())
        assignment.push_back(make_pair(1, var2.getMin()));
      else
        assignment.push_back(make_pair(1, var2.getMax()));
    }
    else
    {
      assignment.push_back(make_pair(1, var2.getMin()));
      if(var1.getMin() != var2.getMin())
        assignment.push_back(make_pair(0, var1.getMin()));
      else
        assignment.push_back(make_pair(0, var1.getMax()));
    }
    return true;
  }

  virtual vector<VirtConPtr> whyF() const
  {
    D_ASSERT(var1.getAssignedValue() == var2.getAssignedValue());
    const DomainInt var1_av = var1.getAssignedValue();
    vector<VirtConPtr> retval;
    retval.reserve(2);
    retval.push_back(var1.getExpl(true, var1_av));
    retval.push_back(var2.getExpl(true, var1_av));
    return retval;
  }

  virtual pair<unsigned,unsigned> whenF() const
  { 
    const DomainInt var1_av = var1.getAssignedValue();
    return max(var1.getDepth(true, var1_av), var2.getDepth(true, var1_av)); 
  }

  virtual void print(std::ostream& o) const
  { 
    o << "watchneq("; inputPrint(o, stateObj, var1.getBaseVar());
    o << ","; inputPrint(o, stateObj, var2.getBaseVar()); o << ")"; 
  }

  virtual void printNeg(std::ostream& o) const
  {
    o << "eq("; inputPrint(o, stateObj, var2.getBaseVar());
    o << ","; inputPrint(o, stateObj, var1.getBaseVar()); o << ",0)";
  }

  virtual AbstractConstraint* copy() const
  { return new WatchNeqConstraint<Var1,Var2>(stateObj, var1, var2); }

  virtual size_t hash() const
  { return 57 * var1.getBaseVar().pos() + var2.getBaseVar().pos(); }

  virtual bool equal(AbstractConstraint* other) const
  { 
    if(guid != other->guid) return false;
    WNCCompData* other_data = static_cast<WNCCompData*>(other->getConCompData());
    D_ASSERT(dynamic_cast<WNCCompData*>(other_data));
    bool retVal = var1.getBaseVar() == other_data->var1 && var2.getBaseVar() == other_data->var2;
    delete other_data;
    return retVal;
  }

  virtual bool less(AbstractConstraint* other) const
  { 
    if(guid < other->guid) return true;
    if(other->guid < guid) return false;
    WNCCompData* other_data = static_cast<WNCCompData*>(other->getConCompData());
    D_ASSERT(dynamic_cast<WNCCompData*>(other_data));
    bool retVal = var1.getBaseVar() < other_data->var1 
      || (var1.getBaseVar() == other_data->var1 && var2.getBaseVar() < other_data->var2);
    delete other_data;
    return retVal;
  }

  virtual WNCCompData* getConCompData() const
  { return new WNCCompData(var1.getBaseVar(), var2.getBaseVar()); }
};

template<typename VarArray1, typename VarArray2>
AbstractConstraint*
WatchNeqConDynamic(StateObj* stateObj, const VarArray1& _var_array_1, const VarArray2& _var_array_2)
{ 
  return new WatchNeqConstraint<typename VarArray1::value_type, typename VarArray2::value_type>
    (stateObj, _var_array_1[0], _var_array_2[0]); 
}

BUILD_CONSTRAINT2(CT_WATCHED_NEQ, WatchNeqConDynamic)
