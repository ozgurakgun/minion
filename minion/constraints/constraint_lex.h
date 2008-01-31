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

/** @help constraints;lexless Description
The constraint

   lexless(vec0, vec1)

takes two vectors vec0 and vec1 of the same length and ensures that
vec0 is lexicographically less than vec1 in any solution.
*/

/** @help constraints;lexless Reifiability
This constraint is reifiable.
*/

/** @help constraints;lexless References
See also

   help constraints lexleq

for a similar constraint with non-strict lexicographic inequality.
*/

/** @help constraints;lexleq Description
The constraint

   lexleq(vec0, vec1)

takes two vectors vec0 and vec1 of the same length and ensures that
vec0 is lexicographically less than or equal to vec1 in any solution.
*/

/** @help constraints;lexleq Reifiability
This constraint is reifiable.
*/

/** @help constraints;lexleq References
See also

   help constraints lexless

for a similar constraint with strict lexicographic inequality.
*/

template<typename VarArray1, typename VarArray2, BOOL Less = false>
struct LexLeqConstraint : public Constraint
{
  virtual string constraint_name()
  { if(Less) return "LexLess"; else return "LexLeq"; }
  
  typedef LexLeqConstraint<VarArray2, VarArray1,!Less> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;
  
  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;
  
  VarArray1 x;
  VarArray2 y;
  
  LexLeqConstraint(StateObj* _stateObj,const VarArray1& _x, const VarArray2& _y) :
    Constraint(_stateObj), alpha(_stateObj), beta(_stateObj), F(_stateObj), x(_x), y(_y)
  { D_ASSERT(x.size() == y.size()); }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_LEXCON,"Setting up Constraint");
    triggerCollection t;
    
    int x_size = x.size();
    for(int i=0; i < x_size; ++i)
    {
      t.push_back(make_trigger(x[i], Trigger(this, i), LowerBound));
      t.push_back(make_trigger(x[i], Trigger(this, i), UpperBound));
    }
    
    int y_size = y.size();
    for(int i=0; i < y_size; ++i)
    {
      t.push_back(make_trigger(y[i], Trigger(this, i), LowerBound));
      t.push_back(make_trigger(y[i], Trigger(this, i), UpperBound));
    }
    alpha = 0;
    if(Less)
      beta = x_size;
    else
      beta = 100000;
    F = 0;
    return t;
  }
  
  virtual Constraint* reverse_constraint()
  {
    return new LexLeqConstraint<VarArray2, VarArray1,!Less>(stateObj,y,x);
  }
  
  void updateAlpha(int i) {
    int n = x.size();
    if(Less)
    {
      if(i == n || i == beta)
      {
        getState(stateObj).setFailed(true);
        return;
      }
      if (!x[i].isAssigned() || !y[i].isAssigned() ||
          x[i].getAssignedValue() != y[i].getAssignedValue())  {
        alpha = i;
        propagate(i,0);
      }
      else updateAlpha(i+1);
    }
    else
    {
      while (i < n) {
        if (!x[i].isAssigned() || !y[i].isAssigned() ||
            x[i].getAssignedValue() != y[i].getAssignedValue())  {
          alpha = i ;
          propagate(i,0) ;
          return ;
        }
        i++ ;
      }
      F = true ;
    }
    
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // updateBeta()
  void updateBeta(int i) {
    int a = alpha ;
    while (i >= a) {
      if (x[i].getMin() < y[i].getMax()) {
        beta = i+1 ;
        if (!(x[i].getMax() < y[i].getMin())) propagate(i,0) ;
        return ;
      }
      i-- ;    
    }
    getState(stateObj).setFailed(true);
    
  }
  
  PROPAGATE_FUNCTION(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(Lex);
    D_INFO(0,DI_LEXCON,"Begin Propagation");
    if (F)
    {
      D_INFO(0,DI_LEXCON,"Already True");
      return ;
    }
    int a = alpha, b = beta;
    
    //Not sure why we need this, but we seem to.
    if(b <= a)
    {
      getState(stateObj).setFailed(true);
      return;
    }
    
    if(Less)
    { if(i < a || i >=b) return; }
    else
    { if (i >= b) return ; }
    
    if (i == a && i+1 == b) {
      x[i].setMax(y[i].getMax()-1) ;
      y[i].setMin(x[i].getMin()+1) ;
      if (checkLex(i)) {
        F = true ;
        return ;
      }
    }
    else if (i == a && i+1 < b) {
      x[i].setMax(y[i].getMax()) ;
      y[i].setMin(x[i].getMin()) ;
      if (checkLex(i)) {
        F = true ;
        return ;
      }
      if (x[i].isAssigned() && y[i].isAssigned() && x[i].getAssignedValue() == y[i].getAssignedValue())
        updateAlpha(i+1) ;
    }
    else if (a < i && i < b) {
      if ((i == b-1 && x[i].getMin() == y[i].getMax()) || x[i].getMin() > y[i].getMax())
        updateBeta(i-1) ;
    }
  }
  
  virtual BOOL check_unsat(int unsat_val, DomainDelta)
  {
    D_INFO(0,DI_LEXCON,string("Checking Unsat")+to_string(Less));
    int a = alpha;
    if(unsat_val >= a)
    {
      int x_size = x.size();
      for(int i = a; i < x_size; ++i)
      {
        DomainInt xval = x[i].getMin();
        DomainInt yval = y[i].getMax();
        if(xval < yval) 
        {
          alpha = i;
          return false;
        }
        if(xval > yval)
          return true;
      }
      if(Less)
        return true;
      else
      {
        alpha = x.size();
        return false;
      }
      
    }
    else
    {
      DomainInt xval = x[unsat_val].getMin();
      DomainInt yval = y[unsat_val].getMax();
      if (xval > yval)
        return true;
      else
        return false;
    }
    FAIL_EXIT();
  }
  
  virtual BOOL full_check_unsat()
  {
    alpha = 0;
    return check_unsat(0, 0);
  }
  
  BOOL checkLex(int i) {
    if(Less)
    {
      return x[i].getMax() < y[i].getMin();
    }
    else
    {
      int n = x.size() ;
      if (i == n-1) return (x[i].getMax() <= y[i].getMin()) ;
      else return (x[i].getMax() < y[i].getMin());
    }
  }
  
  virtual void full_propagate()
  {
    D_INFO(1, DI_LEXCON, "Begin Full Propagate");
    int i, n = x.size() ;
    for (i = 0; i < n; i++) {
      if (!x[i].isAssigned()) break ;    
      if (!y[i].isAssigned()) break ;
      if (x[i].getAssignedValue() != y[i].getAssignedValue()) break ;
    }
    if (i < n) {
      alpha = i ;
      if (checkLex(i)) {
        F = true ;
        return ;
      }
      int betaBound = -1 ;
      for (; i < n; i++) {
        if (x[i].getMin() > y[i].getMax()) break ;
        if (x[i].getMin() == y[i].getMax()) {
          if (betaBound == -1) betaBound = i ;     
        }
        else betaBound = -1 ;
      }
      if(!Less)
      {
        if (i == n) beta = 1000000 ;
        else if (betaBound == -1) beta = i ;
        else beta = betaBound ;
      }
      else
      {
        if (betaBound == -1) beta = n ;
        else beta = betaBound ;
      }
      if (alpha >= beta) getState(stateObj).setFailed(true);
      propagate(alpha,0) ;             //initial propagation, if necessary.
    }
    else 
    {
      if(Less)
        getState(stateObj).setFailed(true);
      else
        F = true;
    }
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
    D_ASSERT(v.size() == x.size() + y.size());
    size_t x_size = x.size();

    for(size_t i = 0;i < x_size; i++)
    {
      if(v[i] < v[i + x_size])
        return true;
      if(v[i] > v[i + x_size])
        return false;
    }
    if(Less)
      return false;
    else
      return true;
      
  }
   
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array_copy;
    for(unsigned int i=0;i<x.size();i++)
      array_copy.push_back(AnyVarRef(x[i]));
    
    for(unsigned int i=0;i<y.size();i++)
      array_copy.push_back(AnyVarRef(y[i]));
    return array_copy;
  }
};


template<typename VarArray1, typename VarArray2>
Constraint*
LexLeqCon(StateObj* stateObj, const VarArray1& x, const VarArray2& y)
{ return new LexLeqConstraint<VarArray1, VarArray2>(stateObj,x,y); }

template<typename VarArray1, typename VarArray2>
Constraint*
LexLessCon(StateObj* stateObj,const VarArray1& x, const VarArray2& y)
{ return new LexLeqConstraint<VarArray1, VarArray2,true>(stateObj, x,y); }

BUILD_CONSTRAINT2(CT_LEXLEQ, LexLeqCon)

BUILD_CONSTRAINT2(CT_LEXLESS, LexLessCon)

