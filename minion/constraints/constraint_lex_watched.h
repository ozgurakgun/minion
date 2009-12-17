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

/** @help constraints;lexless Description
The constraint

   lexless(vec0, vec1)

takes two vectors vec0 and vec1 of the same length and ensures that
vec0 is lexicographically less than vec1 in any solution.
*/

/** @help constraints;lexless Notes
This constraint maintains GAC.
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

/** @help constraints;lexleq Notes
This constraints achieves GAC.
*/

/** @help constraints;lexleq References
See also

   help constraints lexless

for a similar constraint with strict lexicographic inequality.
*/

#ifndef CONSTRAINT_WATCHED_LEX_H
#define CONSTRAINT_WATCHED_LEX_H

template<typename VarArray1, typename VarArray2, BOOL Less, bool DoShrink, bool DoEntailed, bool DoBeta = true, bool BeLazy = false>
struct LexLeqWatchedConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { if(Less) return "LexLess"; else return "LexLeq"; }
  
  typedef LexLeqWatchedConstraint<VarArray2, VarArray1,!Less, DoShrink, DoEntailed, DoBeta, BeLazy> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;
  
  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;
  
  VarArray1 x;
  VarArray2 y;
  
  LexLeqWatchedConstraint(StateObj* _stateObj,const VarArray1& _x, const VarArray2& _y) :
    AbstractConstraint(_stateObj), alpha(_stateObj), beta(_stateObj), F(_stateObj), x(_x), y(_y)
  { 
    D_ASSERT(DoBeta || (!DoShrink && !DoEntailed));
    D_ASSERT(!(BeLazy && !DoBeta));
    D_ASSERT(x.size() == y.size());
    alpha = 0;
    if(Less)
      beta = x.size();
    else
      beta = 1000000;
    F = false;
  }
  
  int dynamic_trigger_count()
  { return x.size() * 2; }

  virtual AbstractConstraint* reverse_constraint()
  {
    return new LexLeqWatchedConstraint<VarArray2, VarArray1,!Less, DoShrink, DoEntailed, DoBeta, BeLazy>(stateObj,y,x);
  }
  
  void attach_all_triggers()
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int size = x.size();
    for(int i = 0; i < size; ++i)
    {
      x[i].addDynamicTrigger(dt + i,       LowerBound, NoDomainValue BT_CALL_BACKTRACK);
      y[i].addDynamicTrigger(dt + i + size,UpperBound, NoDomainValue BT_CALL_BACKTRACK);
    }
  }

  // Remove triggers in the range [i, j)
  void release_trigger_range(int i, int j)
  {
    DynamicTrigger* dt = dynamic_trigger_start();
    int size = x.size();
    for(int l = i; l < j; ++l)
    {
      releaseTrigger(stateObj, dt + l        BT_CALL_BACKTRACK);
      releaseTrigger(stateObj, dt + l + size BT_CALL_BACKTRACK);
    }
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
        index_propagate(i);
      }
      else updateAlpha(i+1);
    }
    else
    {
      while (i < n) {
        if (!x[i].isAssigned() || !y[i].isAssigned() ||
            x[i].getAssignedValue() != y[i].getAssignedValue())  {
          alpha = i ;
          index_propagate(i) ;
          return ;
        }
        i++ ;
      }
      set_implied();
    }
    
  }
  
  ///////////////////////////////////////////////////////////////////////////////
  // updateBeta()
  void updateBeta(int i) {
    D_ASSERT(DoBeta);
    int a = alpha ;
    while (i >= a) {
      if (x[i].getMin() < y[i].getMax()) {
        beta = i+1 ;
        if (!(x[i].getMax() < y[i].getMin())) index_propagate(i) ;
        return ;
      }
      i-- ;    
    }
    getState(stateObj).setFailed(true);
    
  }
  
  virtual void propagate(DynamicTrigger* dt)
  {
    PROP_INFO_ADDONE(WatchLex);
    int i = (dt - dynamic_trigger_start()) % (x.size());
    if(BeLazy)
    {
      if(DoEntailed && F)
      {
        releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
        PROP_INFO_ADDONE(EntailedShrinkLexTs);
        return;
      }
      if(DoShrink && i > beta)
      {
        releaseTrigger(stateObj, dt BT_CALL_BACKTRACK);
        PROP_INFO_ADDONE(LazyShrinkLexTs);
        return;
      }
    }

    int old_beta = beta;
    index_propagate(i);
    if(DoShrink && !BeLazy)
    {
      int max_val = min(old_beta, (int)x.size());
#ifdef MORE_SEARCH_INFO
      int deleted_triggers = max(0, max_val - (beta + 1));
      if(deleted_triggers > 0)
      {
        PROP_INFO_ADDONE(ShrinkLexTriggers);
        PROP_INFO_ADD(ShrinkLexTCount, deleted_triggers);
      }
#endif
      release_trigger_range(beta + 1, max_val);
    }
  }

  void set_implied()
  {
    F = true;
    if(DoEntailed && !BeLazy)
    {
      PROP_INFO_ADDONE(EntailedLex);
      int max_val = x.size();
      if(DoShrink)
        max_val = min((int)beta, max_val);
      release_trigger_range(alpha, max_val);
    }
  }

  void index_propagate(int i)
  {
    if (F)
    {
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
        set_implied();
        return ;
      }
    }
    else if (i == a && i+1 < b) {
      x[i].setMax(y[i].getMax()) ;
      y[i].setMin(x[i].getMin()) ;
      if (checkLex(i)) {
        set_implied();
        return ;
      }
      if (x[i].isAssigned() && y[i].isAssigned() && x[i].getAssignedValue() == y[i].getAssignedValue())
        updateAlpha(i+1) ;
    }
    else if (a < i && i < b && DoBeta) {
      if ((i == b-1 && x[i].getMin() == y[i].getMax()) || x[i].getMin() > y[i].getMax())
        updateBeta(i-1) ;
    }
  }
  
  virtual BOOL check_unsat(int unsat_val, DomainDelta)
  {
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
    attach_all_triggers();
    int i, n = x.size() ;
    for (i = 0; i < n; i++) {
      if (!x[i].isAssigned()) break ;    
      if (!y[i].isAssigned()) break ;
      if (x[i].getAssignedValue() != y[i].getAssignedValue()) break ;
    }
    if (i < n) {
      alpha = i ;
      if (checkLex(i)) {
        set_implied();
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
        if(i == n) beta = n;
        if (betaBound == -1) beta = i ;
        else beta = betaBound ;
      }
      if (alpha >= beta) getState(stateObj).setFailed(true);
      index_propagate(alpha) ;             //initial propagation, if necessary.
    }
    else 
    {
      if(Less)
        getState(stateObj).setFailed(true);
      else
        set_implied();
    }
    if(DoShrink && !BeLazy)
        release_trigger_range(beta + 1, x.size());
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    D_ASSERT(v_size == x.size() + y.size());
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
  
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    size_t x_size = x.size();
    for(size_t i = 0; i < x_size; ++i)
    {
      DomainInt x_i_min = x[i].getMin();
      DomainInt y_i_max = y[i].getMax();
      
      if(x_i_min > y_i_max)
      {
        return false;
      }
      
      assignment.push_back(make_pair(i         , x_i_min));
      assignment.push_back(make_pair(i + x_size, y_i_max));
      if(x_i_min < y_i_max)
        return true;
    }
    
    if(Less)
      return false;
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
#endif