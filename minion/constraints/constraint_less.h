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

/** @help constraints;ineq Description
The constraint

   ineq(x, y, k)

ensures that 

   x <= y + k 

in any solution.
*/

/** @help constraints;ineq Notes
Minion has no strict inequality (<) constraints. However x < y can be
achieved by

   ineq(x, y, -1)
*/

#ifndef CONSTRAINT_LESS_H
#define CONSTRAINT_LESS_H

// x <= y + offset
template<typename VarRef1, typename VarRef2, typename Offset>
struct LeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "Leq"; }
  
  //typedef BoolLessSumConstraint<VarArray, VarSum,1-VarToCount> NegConstraintType;
  const Offset offset;
  VarRef1 x;
  VarRef2 y;
  
  LeqConstraint(StateObj* _stateObj,VarRef1 _x, VarRef2 _y, Offset _o) :
    AbstractConstraint(_stateObj), offset(_o), x(_x), y(_y)
  { }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    t.push_back(make_trigger(x, Trigger(this, 0), LowerBound));
    t.push_back(make_trigger(y, Trigger(this, 1), UpperBound));
    PUSH_EQUALITY_TRIGGER(t, x.getBaseVar(), y.getBaseVar(), this, 2);
    return t;
    
  }

  // Needs to be at end of file
  virtual AbstractConstraint* reverse_constraint();
  
  virtual void propagate(int prop_val,DomainDelta)
  {
    cout << "ineq prop DD" << endl;

    PROP_INFO_ADDONE(BinaryLeq);
    if(prop_val == 1)
    {// y changed
      x.setMax(y.getMax() + offset);
    }
    else if(prop_val == 0)
    {// x changed
      y.setMin(x.getMin() - offset);
    }
    else if(prop_val == 2) {
      D_ASSERT(ARE_EQUAL(stateObj, x.getBaseVar(), y.getBaseVar()));
      if(offset < 0) {
	getState(stateObj).setFailed(true);
	cout << "DP: in ineq failing because of equality and negative k" << endl;
      }
    }
  }
  
  virtual BOOL check_unsat(int,DomainDelta)
  { return (x.getMin() > y.getMax() + offset); }
  
  virtual BOOL full_check_unsat()
  { return (x.getMin() > y.getMax() + offset); }
  
  virtual void full_propagate()
  {
    cout << "FP ineq" << endl;
    if(offset >= 0 && ARE_EQUAL(stateObj, x.getBaseVar(), y.getBaseVar())) {
      propagate(2, 0);
    }
    if(offset < 0) {
      SET_DISEQUAL(stateObj, x.getBaseVar(), y.getBaseVar());
      cout << "in ineq setting disequal because of negative k" << endl;
    }
    propagate(0,0);
    propagate(1,0);
    cout << "end FP ineq" << endl;
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == 2);
    return v[0] <= (v[1] + offset);
  }
  
  virtual void append_sat_assg_vars_firsttime() 
  { singleton_sat_assg_vars.push_back(GET_DISEQ_BOOL(stateObj, x.getBaseVar(), y.getBaseVar())); } //idx 2
    
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    int x_min = x.getMin();
    int y_max = y.getMax();
    
    if(offset < 0 && ARE_EQUAL(stateObj, x.getBaseVar(), y.getBaseVar())) {
      cout << "refusing to return sat assg in ineq failing because of equality and negative k" << endl;
      return false;
    }

    if(x_min <= y_max + offset)
    {
      assignment.push_back(make_pair(0, x_min));
      assignment.push_back(make_pair(1, y_max));
      assignment.push_back(make_pair(2, 1));
      return true;
    } 
    return false;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array;
    array.reserve(2);
    array.push_back(x);
    array.push_back(y);
    return array;
  }
};

template<typename VarRef1, typename VarRef2, typename Offset>
AbstractConstraint*
LeqCon(StateObj* stateObj, VarRef1 v1, VarRef2 v2, Offset o)
{ return new LeqConstraint<VarRef1,VarRef2,Offset>(stateObj,v1,v2,o); }

template<typename VarRef1, typename VarRef2>
AbstractConstraint*
LeqCon(StateObj* stateObj,VarRef1 v1, VarRef2 v2)
{ return new LeqConstraint<VarRef1,VarRef2,compiletime_val<0> >(stateObj,v1,v2,compiletime_val<0>()); }

template<typename VarRef>
AbstractConstraint*
ImpliesCon(StateObj* stateObj, VarRef v1, VarRef v2)
{ return new LeqConstraint<VarRef,VarRef,compiletime_val<0> >(stateObj,v1,v2,compiletime_val<0>()); }

// This is mainly inline to avoid multiple definitions.
template<typename VarRef1, typename VarRef2, typename Offset>
inline AbstractConstraint* LeqConstraint<VarRef1, VarRef2, Offset>::reverse_constraint()
{ return LeqCon(stateObj,y,x, offset.negminusone()); }

#endif
