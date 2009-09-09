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

/** @help constraints;gaclexless Description
  The constraint

  gaclexless(vec0, vec1)

  takes two vectors vec0 and vec1 of the same length and ensures that
  vec0 is lexicographically less than vec1 in any solution.
*/

/** @help constraints;gaclexless Notes
  This constraint maintains GAC.
*/

/** @help constraints;gaclexless References
  See also

  help constraints gaclexleq

  for a similar constraint with non-strict lexicographic inequality.
*/

/** @help constraints;gaclexleq Description
  The constraint

  gaclexleq(vec0, vec1)

  takes two vectors vec0 and vec1 of the same length and ensures that
  vec0 is lexicographically less than or equal to vec1 in any solution.
*/

/** @help constraints;gaclexleq Notes
  This constraints achieves GAC.
*/

/** @help constraints;gaclexleq References
  See also

  help constraints gaclexless

  for a similar constraint with strict lexicographic inequality.
*/

#ifndef CONSTRAINT_GACLEX_H
#define CONSTRAINT_GACLEX_H

template<typename VarArray1, typename VarArray2, BOOL Less = false>
  struct GacLexLeqConstraint : public AbstractConstraint
{
  virtual string constraint_name()
    { if(Less) return "LexLess"; else return "GacLexLeq"; }

  typedef GacLexLeqConstraint<VarArray2, VarArray1,!Less> NegConstraintType;
  typedef typename VarArray1::value_type ArrayVarRef1;
  typedef typename VarArray2::value_type ArrayVarRef2;

  ReversibleInt alpha;
  ReversibleInt beta;
  ReversibleInt F;

  VarArray1 x;
  VarArray2 y;

  vector<pair<int, int> > earliest_occurrence_x;
  vector<pair<int, int> > earliest_occurrence_y;

  GacLexLeqConstraint(StateObj* _stateObj,const VarArray1& _x, const VarArray2& _y) :
  AbstractConstraint(_stateObj), alpha(_stateObj), beta(_stateObj), F(_stateObj), x(_x), y(_y)
  { 
    D_ASSERT(x.size() == y.size()); 
    for(int i = 0; i < x.size(); ++i)
    {
      if(x[i].getBaseVar() == y[i].getBaseVar())
        D_FATAL_ERROR("GacLex constraints cannot have a variable repeated at an index");
    }

    for(int i = 0; i < x.size(); ++i)
    {
      Var base = x[i].getBaseVar();
      pair<int, int> pos = make_pair(0, i);
      for(int j = 0; j < i; ++j)
      {
        if(x[j].getBaseVar() == base)
        {
          pos = make_pair(0, j);
          j = i;
        }
        if(y[j].getBaseVar() == base)
        {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_x.push_back(pos);
    }

    for(int i = 0; i < y.size(); ++i)
    {
      Var base = y[i].getBaseVar();
      pair<int, int> pos = make_pair(1, i);
      for(int j = 0; j < i; ++j)
      {
        if(x[j].getBaseVar() == base)
        {
          pos = make_pair(0, j);
          j = i;
        }
        if(y[j].getBaseVar() == base)
        {
          pos = make_pair(1, j);
          j = i;
        }
      }
      earliest_occurrence_y.push_back(pos);
    }

  }

  virtual triggerCollection setup_internal()
  {
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

  virtual AbstractConstraint* reverse_constraint()
  {
    return new GacLexLeqConstraint<VarArray2, VarArray1,!Less>(stateObj,y,x);
  }

  BOOL updateAlpha(int i) {
    int n = x.size();
    if(Less)
    {
      if(i == n || i == beta)
      {
        return false;
      }
      if (!x[i].isAssigned() || !y[i].isAssigned() ||
      x[i].getAssignedValue() != y[i].getAssignedValue())  {
        alpha = i;
        if(!propagate(i,0))
            return false;
      }
      else updateAlpha(i+1);
    }
    else
    {
      while (i < n) {
        if (!x[i].isAssigned() || !y[i].isAssigned() ||
        x[i].getAssignedValue() != y[i].getAssignedValue())  {
          alpha = i ;
          return propagate(i,0) ;
        }
        i++ ;
      }
      F = true ;
    }

    return true;
  }

  ///////////////////////////////////////////////////////////////////////////////
  // updateBeta()
  BOOL updateBeta(int i) {
    int a = alpha ;
    while (i >= a) {
      if (x[i].getMin() < y[i].getMax()) {
        beta = i+1 ;
        if (!(x[i].getMax() < y[i].getMin())) return propagate(i,0) ;
      }
      i-- ;    
    }
    return false;

  }

  virtual BOOL propagate(int i, DomainDelta)
  {
    PROP_INFO_ADDONE(Lex);
    if (F)
    {
      return true;
    }
    int a = alpha, b = beta;

    //Not sure why we need this, but we seem to.
    if(b <= a)
    {
      return false;
    }

    if(Less)
      { if(i < a || i >=b) return true; }
    else
      { if (i >= b) return true; }

    if (i == a && i+1 == b) {
      if(!x[i].setMax(y[i].getMax()-1))
        return false;
      if(!y[i].setMin(x[i].getMin()+1))
        return false;
      if (checkLex(i)) {
        F = true ;
        return true;
      }
    }
    else if (i == a && i+1 < b) {
      if(!x[i].setMax(y[i].getMax()))
        return false;
      if(!y[i].setMin(x[i].getMin()))
        return false;
      if (checkLex(i)) {
        F = true ;
        return true;
      }
      if (x[i].isAssigned() && y[i].isAssigned() && x[i].getAssignedValue() == y[i].getAssignedValue())
        if(!updateAlpha(i+1))
            return false;
    }
    else if (a < i && i < b) {
      if ((i == b-1 && x[i].getMin() == y[i].getMax()) || x[i].getMin() > y[i].getMax())
        if(!updateBeta(i-1))
            return false;
    }

    return gacpass();
  }

  BOOL gacpass()
  {
    int a = alpha;
    int n = x.size();
    //int b = beta;
    
    if(x[a].getMax() == y[a].getMax())
    { 
      // We need to find support for x[a] = max.
      for(int i = a+1; i < n; ++i)
      {
        int x_val;
        int y_val;
        if(earliest_occurrence_x[i].second != a)
        {
          if(earliest_occurrence_x[i].first == 0)
            x_val = x[i].getMin();
          else
            x_val = x[i].getMax();
        }
        else
          x_val = x[i].getMax();

        if(earliest_occurrence_y[i].second != a)
        {
          if(earliest_occurrence_y[i].first == 0)
            y_val = y[i].getMin();
          else
            y_val = y[i].getMax();
        }
        else
          y_val = y[i].getMax();

        if(x_val < y_val)
          goto y_case;

        if(x_val > y_val)
        {
          if(!x[a].setMax(y[a].getMax() - 1))
            return false;
          goto y_case;
        }
      }
    }
    
    y_case:

    //cout << "!!" << endl;
    if(x[a].getMin() == y[a].getMin())
    { 
        // We need to find support for y[a] = min.
      for(int i = a+1; i < n; ++i)
      {
        int x_val;
        int y_val;
        if(earliest_occurrence_x[i].second != a)
        {
          if(earliest_occurrence_x[i].first == 0)
            x_val = x[i].getMin();
          else
            x_val = x[i].getMax();
        }
        else
          x_val = x[i].getMin();

        if(earliest_occurrence_y[i].second != a)
        {
          if(earliest_occurrence_y[i].first == 0)
            y_val = y[i].getMin();
          else
            y_val = y[i].getMax();
        }
        else
          y_val = y[i].getMin();
          
        //cout << x_val << "." << y_val << endl;
        //cout << earliest_occurrence_x[i] << "." << earliest_occurrence_y[i] << endl;

        if(x_val < y_val)
          return true;

        if(x_val > y_val)
        {
          //cout << "Prop trigger!" << endl;
          //cout << a << ":" << i << ":" << (int)(beta) << endl;
          if(!y[a].setMin(x[a].getMin() + 1))
            return false;
          return true;
        }
      }
    }

    return true;
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

  virtual BOOL full_propagate()
  {
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
        return true;
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
      if (alpha >= beta) return false;
      if(!propagate(alpha,0))
        return false;
    }
    else 
    {
      if(Less)
        return false;
      else
        F = true;
    }
    return true;
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
