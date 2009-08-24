

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

/** @help constraints;eq Description
Constrain two variables to take equal values.
*/

/** @help constraints;eq Example
eq(x0,x1)
*/

/** @help constraints;eq Notes
Achieves bounds consistency.
*/

/** @help constraints;eq Reference
help constraints minuseq
*/

/** @help constraints;minuseq Description
Constraint

   minuseq(x,y)

nsures that x=-y.
*/

/** @help constraints;minuseq Reference
help constraints eq
*/


/** @help constraints;diseq Description
Constrain two variables to take different values.
*/

/** @help constraints;diseq Notes
Achieves arc consistency.
*/

/** @help constraints;diseq Example
diseq(v0,v1)
*/

// This will become always true sooner or later.


/// (var1 = var2) = var3

#ifndef CONSTRAINT_EQUAL_H
#define CONSTRAINT_EQUAL_H

// New version written by PN with bound triggers.
// Also stronger in eq case: copies bounds across rather than just propagating on assignment. 
template<typename EqualVarRef1, typename EqualVarRef2, typename ReifyVarRef>
struct ReifiedEqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "ReifiedEqual"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  ReifyVarRef var3;
  ReifiedEqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2, ReifyVarRef _var3) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2), var3(_var3)
  {
      CHECK(var3.getInitialMin() >= 0, "Reification variables must have domain within {0,1}");
      CHECK(var3.getInitialMin() <= 1, "Reification variables must have domain within {0,1}");
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 10), LowerBound));
    t.push_back(make_trigger(var1, Trigger(this, 11), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 20), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, 21), UpperBound));
    t.push_back(make_trigger(var3, Trigger(this, 3), Assigned));
    PUSH_EQUALITY_TRIGGER(t, var1.getBaseVar(), var2.getBaseVar(), this, 1);
    PUSH_DISEQUALITY_TRIGGER(t, var1.getBaseVar(), var2.getBaseVar(), this, -1);
    return t;
  }
  
  // rewrite the following two functions.
  virtual void full_propagate()
  {
    cout << "reify eq FP" << endl;
    if(var3.isAssigned())
    {
      if(var3.getAssignedValue() == 1) {
	cout << "setting equal in " << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
	SET_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
        eqprop();
      } else {
	cout << "setting disequal inside full prop for " << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
	SET_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
          if(var1.isAssigned())
          {
              diseqvar1assigned();
          }
          if(var2.isAssigned())
          {
              diseqvar2assigned();
          }
      }
    }
    else
    {   // r not assigned.
        check();
    }
  }
  
  virtual void propagate(int i, DomainDelta)
  {
    cout << "reify eq DP DD" << endl;
    PROP_INFO_ADDONE(ReifyEqual);
    switch(i)
    {
      case 10:
          // var1 lower bound has moved
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var2.setMin(var1.getMin());
              }
              else
              { // not equal.     
                  diseq();
              }
          }
          else
          {
              check();
          }
        break;
        
    case 11:
        // var1 upper bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var2.setMax(var1.getMax());
              }
              else
              { // not equal.     
                  diseq();
              }
          }
          else
          {
              check();
          }
        break;        
        
      case 20:
          // var2 lower bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var1.setMin(var2.getMin());
              }
              else
              {
                  diseq();
              }
          }
          else
          {
              check();
          }
          break;
          
      case 21:
          // var2 upper bound has moved.
          if(var3.isAssigned())
          {
              if(var3.getAssignedValue()==1)
              {
                  var1.setMax(var2.getMax());
              }
              else
              {
                  diseq();
              }
          }
          else
          {
              check();
          }
          break;

    case 1:
      cout << "in reify received equal," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      var3.propagateAssign(1);
      eqprop();
      break;

    case -1:
      cout << "in reify received disequal," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      var3.propagateAssign(0);
      diseq();
      break;
          
      case 3:
	  D_ASSERT(var3.isAssigned());
          if(var3.getAssignedValue()==1)
	  {
              SET_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
	      cout << "in reify produced equal," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
              eqprop();
          }
          else
          {
	      SET_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
	      cout << "in reify produced disequal," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
              diseq();
          }
          break;
        
    }
  }
  
  inline void eqprop()
  {
      var1.setMin(var2.getMin());
      var1.setMax(var2.getMax());
      var2.setMin(var1.getMin());
      var2.setMax(var1.getMax());
  }
  
  inline void check()
  {   // var1 or var2 has changed, so check
    if(var1.getMax()<var2.getMin() || var1.getMin()>var2.getMax() 
       || ARE_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar()))
      {   // not equal
	if(ARE_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar()))
	  cout << "discovered disequal in check()," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
          var3.propagateAssign(0);
      }
    if((var1.isAssigned() && var2.isAssigned()
	&& var1.getAssignedValue()==var2.getAssignedValue())
       || ARE_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar()))
      {   // equal
	if(ARE_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar()))
	  cout << "discovered equal in check()," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
          var3.propagateAssign(1);
      }
  }
  
  inline void diseqvar1assigned()
  {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
  }
  
  inline void diseqvar2assigned()
  {
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
  }
  
  inline void diseq()
  {
      if(var1.isAssigned())
      {
          diseqvar1assigned();
      }
      else if(var2.isAssigned())
      {
          diseqvar2assigned();
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

// This is required for the following to have bound triggers
// when the variables are bound,
// and propagate confluently.
// Remove the define to go back to assignment triggers in all cases.
#define MAKECONFLUENT

template<typename VarRef1, typename VarRef2>
struct NeqConstraintBinary : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Neq(Binary)"; }
  
  VarRef1 var1;
  VarRef2 var2;
  
  
  NeqConstraintBinary(StateObj* _stateObj, const VarRef1& _var1, const VarRef2& _var2 ) :
    AbstractConstraint(_stateObj), var1(_var1), var2(_var2)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    #ifndef MAKECONFLUENT
    t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
    #else
    if(var1.isBound())
    {
        t.push_back(make_trigger(var1, Trigger(this, 3), UpperBound));
        t.push_back(make_trigger(var1, Trigger(this, 4), LowerBound));
    }
    else
    {
        t.push_back(make_trigger(var1, Trigger(this, 1), Assigned));
    }
    
    if(var2.isBound())
    {
        t.push_back(make_trigger(var2, Trigger(this, 5), UpperBound));
        t.push_back(make_trigger(var2, Trigger(this, 6), LowerBound));
    }
    else
    {
        t.push_back(make_trigger(var2, Trigger(this, 2), Assigned));
    }
    #endif
    PUSH_EQUALITY_TRIGGER(t, var1.getBaseVar(), var2.getBaseVar(), this, 1001);
    return t;
  }
  
  virtual void propagate(int prop_val, DomainDelta)
  {
    PROP_INFO_ADDONE(BinaryNeq);
    if(prop_val == 1001) {
      cout << "failing in diseq," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      getState(stateObj).setFailed(true);
      return;
    }
    if (prop_val == 1) {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
    }
    #ifdef MAKECONFLUENT
    else if(prop_val == 3)
    {   // ub moved var1
        if(var2.isAssigned() && var2.getAssignedValue()==var1.getMax())
            var1.setMax(var1.getMax()-1);
        if(var1.isAssigned())
        {
            var1assigned();
        }
    }
    else if(prop_val == 4)
    {   // lb moved var1
        if(var2.isAssigned() && var2.getAssignedValue()==var1.getMin())
            var1.setMin(var1.getMin()+1);
        if(var1.isAssigned())
        {
            var1assigned();
        }
    }
    else if(prop_val == 5)
    {   // ub moved var2
        if(var1.isAssigned() && var1.getAssignedValue()==var2.getMax())
            var2.setMax(var2.getMax()-1);
        if(var2.isAssigned())
        {
            var2assigned();
        }
    }
    else if(prop_val == 6)
    {   // lb moved var2
        if(var1.isAssigned() && var1.getAssignedValue()==var2.getMin())
            var2.setMin(var2.getMin()+1);
        if(var2.isAssigned())
        {
            var2assigned();
        }
    }
    #endif
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
      else {
        var1.removeFromDomain(remove_val);
      }
    }
  }
  
  inline void var1assigned()
  {
      DomainInt remove_val = var1.getAssignedValue();
      if(var2.isBound())
      {
        if(var2.getMin() == remove_val)
          var2.setMin(remove_val + 1);
        if(var2.getMax() == remove_val)
          var2.setMax(remove_val - 1);
      }
      else {
        var2.removeFromDomain(remove_val);
      }
  }
  
  inline void var2assigned()
  {
      DomainInt remove_val = var2.getAssignedValue();
      if(var1.isBound())
      {
        if(var1.getMin() == remove_val)
          var1.setMin(remove_val + 1);
        if(var1.getMax() == remove_val)
          var1.setMax(remove_val - 1);
      }
      else {
        var1.removeFromDomain(remove_val);
      }
  }
  
  virtual void full_propagate()
  {
    cout << "setting disequal," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
    SET_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());

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
      else {
        var2.removeFromDomain(remove_val);
      }
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
      else {
        var1.removeFromDomain(remove_val);
      }
    }
  }
    
    virtual BOOL check_assignment(DomainInt* v, int v_size)
    {
      D_ASSERT(v_size == 2); 
      if(v[0]==v[1]) return false;
      return true;
    }
    
    virtual void append_sat_assg_vars_firsttime() 
    { singleton_sat_assg_vars.push_back(GET_DISEQ_BOOL(stateObj, var1.getBaseVar(), var2.getBaseVar())); } //idx 2
    
    virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    if(ARE_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar())) {
      cout << "refused to make sat assg in diseq con," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      return false;
    }

    if(var1.getMin() != var2.getMax())
    {
      cout << "produced sat assg for " << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      assignment.push_back(make_pair(0, var1.getMin()));
      assignment.push_back(make_pair(1, var2.getMax()));
      assignment.push_back(make_pair(2, 0));
      return true; 
    }
    
    if(var1.getMax() != var2.getMin())
    {
      cout << "produced sat assg for " << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      assignment.push_back(make_pair(0, var1.getMax()));
      assignment.push_back(make_pair(1, var2.getMin()));
      assignment.push_back(make_pair(2, 0));
      return true;
    }
    
    D_ASSERT(var1.isAssigned() && var2.isAssigned());
    D_ASSERT(var1.getAssignedValue() == var2.getAssignedValue());
    return false;
  }
  
  
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> vars(2);
          vars[0] = var1;
          vars[1] = var2;
      return vars;
    }
};


template<typename EqualVarRef1, typename EqualVarRef2, bool isMinuseq>
struct EqualConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Equal"; }
  
  EqualVarRef1 var1;
  EqualVarRef2 var2;
  EqualConstraint(StateObj* _stateObj, EqualVarRef1 _var1, EqualVarRef2 _var2) : AbstractConstraint(_stateObj),
    var1(_var1), var2(_var2)
  {}

  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(var1, Trigger(this, 1), UpperBound));
    t.push_back(make_trigger(var1, Trigger(this, 2), LowerBound));
    t.push_back(make_trigger(var2, Trigger(this, 3), UpperBound));
    t.push_back(make_trigger(var2, Trigger(this, 4), LowerBound));
    if(isMinuseq)
      PUSH_EQUALITY_TRIGGER(t, var1.getBaseVar(), var2.getBaseVar(), this, 5);
    else {
      PUSH_DISEQUALITY_TRIGGER(t, var1.getBaseVar(), var2.getBaseVar(), this, 5);
    }
    return t;
  }

  int dynamic_trigger_count() 
  { 
    if(isMinuseq) return 2; //check if 0 is removed from either variable
    else return 0;
  }
  
  virtual void full_propagate()
  {
    cout << "FP equal" << endl;
    if(!isMinuseq) {
      if(ARE_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar())) {
	getState(stateObj).setFailed(true);
	return;
      }
      cout << "setting equal in equal con," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
      SET_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
    } else {
      if(ARE_EQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar())) {
	cout << "in minuseq FP: vars equal" << endl;
	var1.propagateAssign(0);
	var2.propagateAssign(0);
      }
      if(!var1.inDomain(0) || !var2.inDomain(0)) {
	cout << "in minuseq FP: 0 missing from var1 or var2, setting disequal" << endl;
	SET_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
      }
      if(var1.inDomain(0))
	var1.addDynamicTrigger(dynamic_trigger_start(), DomainRemoval, 0);
      if(var2.inDomain(0))
	var2.addDynamicTrigger(dynamic_trigger_start() + 1, DomainRemoval, 0);
    }
    propagate(1,0);
    propagate(2,0);
    propagate(3,0);
    propagate(4,0);
  }
  
  virtual void propagate(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(Equal);
    switch(i)
    {
      case 1:
        var2.setMax(var1.getMax());
        return;
      case 2:
        var2.setMin(var1.getMin());
        return;
      case 3:
        var1.setMax(var2.getMax());
        return;
      case 4:
        var1.setMin(var2.getMin());
        return;
      case 5:
	if(isMinuseq) {
	  cout << "failing in equal constraint," << var1.getBaseVar() << "," << var2.getBaseVar() << endl;
	  getState(stateObj).setFailed(true); //must fail on any disequality between these vars
	  return;
	} else {
	  cout << "in minuseq prop: equal" << endl;
	  var1.propagateAssign(0);
	  var2.propagateAssign(0);
	  return;
	}
    }
  }

  virtual void propagate(DynamicTrigger* dt)
  {
    cout << "in minuseq prop: either var1 or var2 missing 0, setting disequal" << endl;
    SET_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar());
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

  virtual void append_sat_assg_vars_firsttime()
  { singleton_sat_assg_vars.push_back(GET_DISEQ_BOOL(stateObj, var1.getBaseVar(), var2.getBaseVar())); } //idx 2

   virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
   {

     if(ARE_DISEQUAL(stateObj, var1.getBaseVar(), var2.getBaseVar()))
       return false;

     DomainInt min_val = max(var1.getMin(), var2.getMin());
     DomainInt max_val = min(var1.getMax(), var2.getMax());
     
     for(DomainInt i = min_val ; i <= max_val; ++i)
     {
       if(var1.inDomain(i) && var2.inDomain(i))
       {
         assignment.push_back(make_pair(0, i));
         assignment.push_back(make_pair(1, i));
	 assignment.push_back(make_pair(2, 1)); //also make sure that equality hasn't become impossible
         return true;
       } 
     }
     return false;
   }
   
   virtual AbstractConstraint* reverse_constraint()
   {
       return new NeqConstraintBinary<EqualVarRef1, EqualVarRef2>(stateObj, var1, var2);
   }
};

template<typename EqualVarRef1, typename EqualVarRef2>
AbstractConstraint*
EqualCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, EqualVarRef2, false>(stateObj, var1,var2); }

template<typename EqualVarRef1, typename EqualVarRef2>
AbstractConstraint*
EqualMinusCon(StateObj* stateObj, EqualVarRef1 var1, EqualVarRef2 var2)
{ return new EqualConstraint<EqualVarRef1, VarNeg<EqualVarRef2>, true>(stateObj, var1,VarNegRef(var2)); }

template<typename Var1, typename Var2>
AbstractConstraint*
NeqConBinary(StateObj* stateObj, const Var1& var1, const Var2& var2)
{
  return new NeqConstraintBinary<Var1, Var2>(stateObj, var1, var2); 
}

#endif
