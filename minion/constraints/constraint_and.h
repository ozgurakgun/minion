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

/// var1 /\ var2 = var3
template<typename VarRef1, typename VarRef2, typename VarRef3>
struct AndConstraint : public Constraint
{
  virtual string constraint_name()
  { return "And"; }
  
  VarRef1 var1;
  VarRef2 var2;
  VarRef3 var3;
  AndConstraint(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2, VarRef3 _var3) : Constraint(_stateObj),
    var1(_var1), var2(_var2), var3(_var3)
  {
	D_ASSERT(var1.getInitialMin() == 0);
    D_ASSERT(var1.getInitialMax() == 1);
	D_ASSERT(var2.getInitialMin() == 0);
	D_ASSERT(var2.getInitialMax() == 1);
	D_ASSERT(var3.getInitialMin() == 0);
	D_ASSERT(var3.getInitialMax() == 1);
  }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, 2), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
	t.push_back(make_trigger(var1, Trigger(this, -1), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, -2), UpperBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), UpperBound));
	return t;
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(And);
    switch(i)
    {
      case 1:
        if(var2.isAssignedValue(true))
          var3.propagateAssign(true, label());
        else
        {
          if(var3.isAssignedValue(false))
            var2.propagateAssign(false, label());
        }
          break;
        
      case 2:
        if(var1.isAssignedValue(true))
          var3.propagateAssign(true, label());
        else
        {
          if(var3.isAssignedValue(false))
            var1.propagateAssign(false, label());
        }
          break;
        
      case 3:
        var1.propagateAssign(true, label());
        var2.propagateAssign(true, label());
        break;
        
        
      case -1:
      case -2:
        var3.propagateAssign(false, label());
        break;
        
      case -3:
        if(var1.isAssignedValue(true))
          var2.propagateAssign(false, label());
        else
        {
          if(var2.isAssignedValue(true))
            var1.propagateAssign(false, label());
        }
          break;
    }
    
  }
  
  virtual void full_propagate()
  {
    if(var1.isAssignedValue(false) || var2.isAssignedValue(false))
      var3.propagateAssign(false, label());
    
    if(var1.isAssignedValue(true) && var2.isAssignedValue(true))
      var3.propagateAssign(true, label());
    
    if(var3.isAssignedValue(false))
    {
      if(var1.isAssignedValue(true))
		var2.propagateAssign(false, label());
      if(var2.isAssignedValue(true))
		var1.propagateAssign(false, label());
    }
    
    if(var3.isAssignedValue(true))
    {
      var1.propagateAssign(true, label());
      var2.propagateAssign(true, label());
    }
    
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
    return ((v[0] != 0) && (v[1] != 0)) == (v[2] != 0);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
	vector<AnyVarRef> v;
	v.push_back(var1);
	v.push_back(var2);
	v.push_back(var3);
	return v;
  }
};

template<typename VarRef1, typename VarRef2, typename VarRef3>
Constraint*
AndCon(StateObj* stateObj, VarRef1 var1, VarRef2 var2, VarRef3 var3)
{ return (new AndConstraint<VarRef1,VarRef2,VarRef3>(stateObj, var1,var2,var3)); }

