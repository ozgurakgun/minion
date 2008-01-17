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



template<typename VarArray>
struct NeqConstraint : public Constraint
{
  virtual string constraint_name()
  { return "Neq"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;
  
  NeqConstraint(StateObj* _stateObj, const VarArray& _var_array) : Constraint(_stateObj),
    var_array(_var_array)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), Assigned));
    return t;
  }
  
  virtual Constraint* reverse_constraint()
  { return new CheckAssignConstraint<VarArray, NeqConstraint>(stateObj, var_array, *this); }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(ArrayNeq);
    DomainInt remove_val = var_array[prop_val].getAssignedValue();
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
    {
      if(i != prop_val)
	  {
		if(var_array[i].isBound())
	    {
		  if(var_array[i].getMin() == remove_val)
		    var_array[i].setMin(remove_val + 1);
		  if(var_array[i].getMax() == remove_val)
		    var_array[i].setMax(remove_val - 1);
	    }
		else
		  var_array[i].removeFromDomain(remove_val);
	  }
    }
	
  }
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
	{
	  if(var_array[i].isAssigned())
	  {
	  
	    for(int j = i + 1; j < v_size; ++j)
		{
		  if(var_array[j].isAssigned())
		  {
		    if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
		      return true;
		  }
		}
		
	  }
	}
	
	return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
	D_ASSERT(var_array[i].isAssigned());
	DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
	{
	  if(loop != i)
	  {
	    if(var_array[loop].isAssigned() && 
		   var_array[loop].getAssignedValue() == assign_val)
		return true;
	  }
	}
	return false;
  }
  
  
  virtual void full_propagate()
  {
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
      if(var_array[i].isAssigned())
      {
		DomainInt remove_val = var_array[i].getAssignedValue();
		for(int j = 0; j < array_size;++j)
		{
		  if(i != j)
		  {
			if(var_array[j].isBound())
			{
			  if(var_array[j].getMin() == remove_val)
				var_array[j].setMin(remove_val + 1);
			  if(var_array[j].getMax() == remove_val)
				var_array[j].setMax(remove_val - 1);
			}
			else
			  var_array[j].removeFromDomain(remove_val);
		  }
		}
      }
  }
	
	virtual BOOL check_assignment(vector<DomainInt> v)
	{
	  D_ASSERT(v.size() == var_array.size());
	  int array_size = v.size();
	  for(int i=0;i<array_size;i++)
		for( int j=i+1;j<array_size;j++)
		  if(v[i]==v[j]) return false;
	  return true;
	}
	
	virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
	  return vars;
	}
  };

template<typename VarRef1, typename VarRef2>
struct NeqConstraintBinary : public Constraint
{
  virtual string constraint_name()
  { return "Neq(Binary)"; }
  
  VarRef1 var1;
  VarRef2 var2;
  
  
  NeqConstraintBinary(StateObj* _stateObj, VarRef1 _var1, VarRef2 _var2 ) :
    Constraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
    return t;
  }
  
  //  virtual Constraint* reverse_constraint()
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(BinaryNeq);
    if (prop_val == 1) {
      DomainInt remove_val = var1.getAssignedValue();
	  if(var2.isBound())
	  {
		if(var2.getMin() == remove_val)
		  var2.setMin(remove_val + 1);
		if(var2.getMax() == remove_val)
		  var2.setMax(remove_val - 1);
	  }
	  else
        var2.removeFromDomain(remove_val);
	}
    else
    {
      D_ASSERT(prop_val == 2);
      DomainInt remove_val = var2.getAssignedValue();
	  if(var1.isBound())
	  {
		if(var1.getMin() == remove_val)
		  var1.setMin(remove_val + 1);
		if(var1.getMax() == remove_val)
		  var1.setMax(remove_val - 1);
	  }
	  else
        var1.removeFromDomain(remove_val);
    }
  }
  
  //  virtual BOOL check_unsat(int i, DomainDelta)
  
  
  virtual void full_propagate()
  {
    if(var1.isAssigned())
    { 
      DomainInt remove_val = var1.getAssignedValue();
	  if(var2.isBound())
	  {
		if(var2.getMin() == remove_val)
		  var2.setMin(remove_val + 1);
		if(var2.getMax() == remove_val)
		  var2.setMax(remove_val - 1);
	  }
	  else
        var2.removeFromDomain(remove_val);
    }
    if(var2.isAssigned())
    { 
      DomainInt remove_val = var2.getAssignedValue();
	  if(var1.isBound())
	  {
		if(var1.getMin() == remove_val)
		  var1.setMin(remove_val + 1);
		if(var1.getMax() == remove_val)
		  var1.setMax(remove_val - 1);
	  }
	  else
        var1.removeFromDomain(remove_val);
    }
  }
	
	virtual BOOL check_assignment(vector<DomainInt> v)
	{
	  D_ASSERT(v.size() == 2); 
	  if(v[0]==v[1]) return false;
	  return true;
	}
	
	virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars(2);
          vars[0] = var1;
          vars[1] = var2;
	  return vars;
	}
  };


template<typename VarArray>
Constraint*
NeqCon(StateObj* stateObj, const VarArray& var_array)
{ return new NeqConstraint<VarArray>(stateObj, var_array); }


template<typename Var1, typename Var2>
Constraint*
NeqConBinary(StateObj* stateObj, const Var1& var1, const Var2& var2)
{
  typedef typename Var1::value_type VarRef1;
  typedef typename Var2::value_type VarRef2;
  return new NeqConstraintBinary<VarRef1, VarRef2>(stateObj, var1[0], var2[0]); 
}

BUILD_CONSTRAINT2(CT_DISEQ, NeqConBinary)

BUILD_CONSTRAINT1(CT_ALLDIFF, NeqCon)
