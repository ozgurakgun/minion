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

/** @help constraints;eq Description
Constrain two variables to take equal values.
*/

/** @help constraints;eq Example
eq(x0,x1)
*/

/** @help constraints;eq Notes
Achieves bounds consistency.
*/

/** @help constraints;eq Reifiability
This constraint is reifiable.
*/

/** @help constraints;eq Reference
help constraints minuseq
*/

/** @help constraints;minuseq Description
Constraint

   minuseq(x,y)

ensures that x=-y.
*/

/** @help constraints;minuseq Reifiability
This constraint is reifiable.
*/

/** @help constraints;minuseq Reference
help constraints eq
*/


/// (var1 = var2) = var3
template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
struct ReifiedEqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "ReifiedEqual"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  BoolVarRef var3;
  ReifiedEqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2, BoolVarRef _var3) :
    Constraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
	t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
	t.push_back(make_trigger(var3, Trigger(this, 3), LowerBound));
	t.push_back(make_trigger(var3, Trigger(this, -3), UpperBound));
	return t;
  }
  
  virtual void full_propagate()
  {
    if(var3.isAssigned())
    {
      if(var3.getAssignedValue() == 1)
		propagate(3,0);
      else
		propagate(-3,0);
    }
    
    if(var1.isAssigned())
      propagate(1,0);
    
    if(var2.isAssigned())
      propagate(2,0);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(ReifyEqual);
    switch(i)
    {
      case 1:
        D_ASSERT(var1.isAssigned());
		if(var2.isAssigned())
		{ var3.propagateAssign(var1.getAssignedValue() == var2.getAssignedValue(), label()); }
		else
		{
		  if(var3.isAssigned())
		  {
			if(var3.getAssignedValue() == 1)
			{ var2.propagateAssign(var1.getAssignedValue(), label()); }
		  }
		}
		break;
        
      case 2:
        D_ASSERT(var2.isAssigned());
        if(var1.isAssigned())
		{ var3.propagateAssign(var1.getAssignedValue() == var2.getAssignedValue(), label()); }
		else
		{
		  if(var3.isAssigned())
		  {
			if(var3.getAssignedValue() == 1)
			{ var1.propagateAssign(var2.getAssignedValue(), label()); }
		  }
		}
		break;        
		
      case 3:
        D_ASSERT(var3.isAssigned() && var3.getAssignedValue()==1);
        // reifyvar==1
		if(var1.isAssigned())
		{ var2.propagateAssign(var1.getAssignedValue(), label()); }
		else
		{
		  if(var2.isAssigned())
		  { var1.propagateAssign(var2.getAssignedValue(), label()); }
		}
		break;
		
      case -3:
        D_ASSERT(var3.isAssigned() && var3.getAssignedValue()==0);
        // reifyvar==0
        
        if(var1.isAssigned() && !var2.isBound())
        {
            var2.removeFromDomain(var1.getAssignedValue(), label());
        }
        
        if(var2.isAssigned() && !var1.isBound())
        {
            var1.removeFromDomain(var2.getAssignedValue(), label());
        }
        
        if(var1.isAssigned() && var2.isAssigned())
		{ 
		  if(var1.getAssignedValue() == var2.getAssignedValue())
			getState(stateObj).setFailed(true);
		}
		break;
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 3);
    D_ASSERT(v[2] == 0 || v[2] == 1);
    return (v[0] == v[1]) == v[2];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(3);
	vars.push_back(var1);
	vars.push_back(var2);
	vars.push_back(var3);
	return vars;
  }
};

template<typename EqualVarRef1, typename EqualVarRef2>
struct EqualConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Equal"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  EqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2) : Constraint(_stateObj),
    var1(_var1), var2(_var2)
  {}
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_ANDCON,"Setting up Constraint");
    triggerCollection t;
	t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
	t.push_back(make_trigger(var1, Trigger(this, 2), LowerBound));
	t.push_back(make_trigger(var2, Trigger(this, 3), UpperBound));
	t.push_back(make_trigger(var2, Trigger(this, 4), LowerBound));
	return t;
  }
  
  virtual void full_propagate()
  {
	propagate(1,0);
	propagate(2,0);
	propagate(3,0);
	propagate(4,0);
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
	PROP_INFO_ADDONE(Equal);
    switch(i)
	{
	  case 1:
		var2.setMax(var1.getMax(), label());
		return;
	  case 2:
		var2.setMin(var1.getMin(), label());
		return;
	  case 3:
		var1.setMax(var2.getMax(), label());
		return;
	  case 4:
		var1.setMin(var2.getMin(), label());
		return;
	}
  }
  
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return (v[0] == v[1]);
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
	vars.reserve(2);
	vars.push_back(var1);
	vars.push_back(var2);
    return vars;
  }
  
};


template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
Constraint*
ReifiedEqualCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2, BoolVarRef var3)
{ return new ReifiedEqualConstraint<EqualVarRef1, EqualVarRef2, BoolVarRef>(stateObj,var1,var2,var3); }

template<typename EqualVarRef1, typename EqualVarRef2>
Constraint*
EqualCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, EqualVarRef2>(stateObj, var1,var2); }


template<typename EqualVarRef1, typename EqualVarRef2, typename BoolVarRef>
Constraint*
ReifiedEqualMinusCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2, BoolVarRef var3)
{ return new ReifiedEqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2>, BoolVarRef>(stateObj, var1,VarNegRef(var2),var3); }

template<typename EqualVarRef1, typename EqualVarRef2>
Constraint*
EqualMinusCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2> >(stateObj, var1,VarNegRef(var2)); }


template<typename T1, typename T2>
Constraint*
BuildCT_EQ(StateObj* stateObj, const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob&) 
{
  if(reify)
  { return ReifiedEqualCon(stateObj, t1[0],t2[0], reifyVar); }
  else
  { return EqualCon(stateObj, t1[0],t2[0]); }
}

template<typename T1, typename T2>
Constraint*
BuildCT_MINUSEQ(StateObj* stateObj, const T1& t1, const T2& t2, BOOL reify, const BoolVarRef& reifyVar, ConstraintBlob&) 
{
  if(reify)
  { return ReifiedEqualMinusCon(stateObj, t1[0],t2[0], reifyVar); }
  else
  { return EqualMinusCon(stateObj, t1[0],t2[0]); }
}
