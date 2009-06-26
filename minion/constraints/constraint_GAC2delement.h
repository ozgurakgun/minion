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

#ifndef CONSTRAINT_GAC2DELEMENT_H_OSAFDJ
#define CONSTRAINT_GAC2DELEMENT_H_OSAFDJ

template<typename VarArray, typename IndexArray, typename VarRef>
struct GAC2DElementConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "GAC2dElement"; }
  
  VarArray var_array;
  typedef typename IndexArray::value_type Indexvartype;
  
  Indexvartype indexvar1;
  Indexvartype indexvar2;
  VarRef resultvar;
  int rowlength;
  
  DomainInt var_array_min_val;
  DomainInt var_array_max_val;
  GAC2DElementConstraint(StateObj* _stateObj, const VarArray& _var_array, const IndexArray& _indexvar, const VarRef& _resultvar, int _rowlength) :
    AbstractConstraint(_stateObj), var_array(_var_array), indexvar1(_indexvar[0]), indexvar2(_indexvar[1]), resultvar(_resultvar),
    rowlength(_rowlength),
    var_array_min_val(0), var_array_max_val(0) 
  { 
      cout << "row length:" << rowlength <<endl;
  
  
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    if(var_array.empty())
      return t;
      
    int array_size = var_array.size();
    DomainInt min_val = var_array[0].getInitialMin();
    DomainInt max_val = var_array[0].getInitialMax();
    for(int i = 1; i < array_size; ++i)
    {
      min_val = min(min_val, var_array[i].getInitialMin());
      max_val = max(max_val, var_array[i].getInitialMax());
    }
    
    var_array_min_val = min_val;
    var_array_max_val = max_val;
    
    // DomainInt domain_size = var_array_max_val - var_array_min_val + 1;
    for(int i = 0; i < array_size; ++i)
    {
      t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    }
    
    t.push_back(make_trigger(indexvar1,
                             Trigger(this, array_size), DomainChanged));
    
    t.push_back(make_trigger(indexvar2,
                             Trigger(this, array_size + 1), DomainChanged));
    
    t.push_back(make_trigger(resultvar,
                             Trigger(this, array_size + 2), DomainChanged));
    return t;
  }
  
  virtual void propagate(int prop_val, DomainDelta)
  {
    //PROP_INFO_ADDONE(GACElement);
    
    
    
  }
  
  virtual void full_propagate()
  {
    
  }
  
  // This needs to be adapted.
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    int length = v_size;
    if(v[length-2] < 0 ||
       v[length-2] > length - 3)
      return false;
    return v[checked_cast<int>(v[length-2])] == v[length-1];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> array;
    array.reserve(var_array.size() + 2);
    for(unsigned int i=0;i<var_array.size(); ++i)
      array.push_back(var_array[i]);
    array.push_back(indexvar1);
    array.push_back(indexvar2);
    array.push_back(resultvar);
    return array;
  }
  /*
  virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    DomainInt array_start = max(DomainInt(0), indexvar.getMin());
    DomainInt array_end   = min((DomainInt)var_array.size() - 1, indexvar.getMax());

    for(DomainInt i = array_start; i <= array_end; ++i)
    {
      if(indexvar.inDomain(i))
      {
        DomainInt dom_start = max(resultvar.getMin(), var_array[i].getMin());
        DomainInt dom_end   = min(resultvar.getMax(), var_array[i].getMax());
        for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
        {
          if(var_array[i].inDomain(domval) && resultvar.inDomain(domval))
          {
            // indexvar = i
            assignment.push_back(make_pair(var_array.size(), i));
            // resultvar = domval
            assignment.push_back(make_pair(var_array.size() + 1, domval));
            // vararray[i] = domval
            assignment.push_back(make_pair(i, domval));
            return true;
          }
        }
      }
    }
    return false;
  }
  
  virtual AbstractConstraint* reverse_constraint()
  {
      // This is a slow-ish temporary solution.
      // (i=1 and X[1]!=r) or (i=2 ...
      vector<AbstractConstraint*> con;
      // or the index is out of range:
      vector<int> r; r.push_back(0); r.push_back(var_array.size()-1);
      AbstractConstraint* t4=(AbstractConstraint*) new WatchNotInRangeConstraint<IndexRef>(stateObj, indexvar, r);
      con.push_back(t4);
      
      for(int i=0; i<var_array.size(); i++)
      {
          vector<AbstractConstraint*> con2;
          WatchLiteralConstraint<IndexRef>* t=new WatchLiteralConstraint<IndexRef>(stateObj, indexvar, i);
          con2.push_back((AbstractConstraint*) t);
          NeqConstraintBinary<AnyVarRef, VarRef>* t2=new NeqConstraintBinary<AnyVarRef, VarRef>(stateObj, var_array[i], resultvar);
          con2.push_back((AbstractConstraint*) t2);
          
          Dynamic_AND* t3= new Dynamic_AND(stateObj, con2);
          con.push_back((AbstractConstraint*) t3);
      }
      
      return new Dynamic_OR(stateObj, con);
  }*/
};
#endif
