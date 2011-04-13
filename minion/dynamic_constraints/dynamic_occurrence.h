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

/** @help constraints;watchvecneq Description
The constraint

   watchvecneq(A, B)

ensures that A and B are not the same vector, i.e., there exists some index i
such that A[i] != B[i].
*/

#ifndef _DYNAMIC_OCCUR_H
#define _DYNAMIC_OCCUR_H

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

template<typename VarArray, bool leq>      // Can be leq or neq
  struct WatchOccurrence : public AbstractConstraint
{
  virtual string constraint_name()
    { return string("WatchOccurrence"); }
    
  typedef typename VarArray::value_type VarRef;
  
  VarArray var_array;
  
  // do leq first.
  // watch |X|-c+1 things that are not equal to a.. when that's not possible, prune a from |X|-c vars. or fail. 
  
  int value;
  int val_count;
  
  int numwatches;
  
  vector<int> watch_index;
  vector<bool> watching_var; // for each var, is it being watched. 
  
  Reversible<bool> propagate_mode;
  
  
  WatchOccurrence(StateObj* _stateObj, const VarArray& _var_array, int _value, int _val_count) :
  AbstractConstraint(_stateObj), var_array(_var_array), value(_value), val_count(_val_count), propagate_mode(_stateObj, false)
    {
        D_ASSERT(leq);
        numwatches=var_array.size()-val_count+1;
        if(numwatches>0) {
            watch_index.resize(numwatches);
            watching_var.resize(var_array.size(), 0);
        }
    }
    
    int dynamic_trigger_count()
    { 
        if(numwatches>0) return numwatches;
        else return 0;
    }

    
  virtual void full_propagate()
  {
    P("watchoccurrence full propagate");
    
    if(numwatches<2) {
        propagate_mode=true;
        return;
    }
    
    DynamicTrigger* dt = dynamic_trigger_start();
    
    propagate_mode=false;
    
    // Find numwatches things to watch.
    int count=0; 
    for(int i=0; i<var_array.size() && count<numwatches; i++) {
        if(var_array[i].getMax()!=value || var_array[i].getMin()!=value) {
            watch_index[count]=i;
            watching_var[i]=1;
            if(var_array[i].getMax()!=value) {
                var_array[i].addDynamicTrigger(dt, DomainRemoval, var_array[i].getMax());
            }
            else {
                var_array[i].addDynamicTrigger(dt, DomainRemoval, var_array[i].getMin());
            }
            count++;
            dt++;
        }
    }
    
    if(count<numwatches) {
        if(count==(var_array.size()-val_count)) {
            for(int i=0; i<count; i++) {
                var_array[watch_index[i]].removeFromDomain(value);
            }
            propagate_mode=true;   // does nothing from now on.
        }
        else{
            // fail.
            getState(stateObj).setFailed(true);
            return;
        }
    }
    
  }
  
  virtual void propagate(DynamicTrigger* dt_trigger)
  {
      if(propagate_mode) return;
      //cout << watch_index <<endl;
      //cout <<watching_var << endl;
      DynamicTrigger* dt = dynamic_trigger_start();
      
      int idx=dt_trigger-dt;
      
      // Lost the watch watch_index[idx]
      int var_lost=watch_index[idx];
      watching_var[var_lost]=0;
      
      int vars=var_array.size();
      for(int i=var_lost; i<vars; i++) {   // include var_lost because it might have another watchable value.
          if(!watching_var[i] && ( var_array[i].getMax()!=value || var_array[i].getMin()!=value )) {
              watch_index[idx]=i;
              watching_var[i]=1;
                if(var_array[i].getMax()!=value) {
                    var_array[i].addDynamicTrigger(dt+idx, DomainRemoval, var_array[i].getMax());
                }
                else {
                    var_array[i].addDynamicTrigger(dt+idx, DomainRemoval, var_array[i].getMin());
                }
              D_DATA(
              for(int j=0; j<watch_index.size(); j++) {
                  D_ASSERT(watching_var[watch_index[j]]);
              }
              int count=0;
              for(int j=0; j<watching_var.size(); j++) {
                  if(watching_var[j]) count++;
              }
              D_ASSERT(count==numwatches);
              )
                
              return;
          }
      }
      
      // Start again at beginning of vaR_array
      for(int i=0; i<var_lost; i++) {
          if(!watching_var[i] && ( var_array[i].getMax()!=value || var_array[i].getMin()!=value )) {
              watch_index[idx]=i;
              watching_var[i]=1;
                if(var_array[i].getMax()!=value) {
                    var_array[i].addDynamicTrigger(dt+idx, DomainRemoval, var_array[i].getMax());
                }
                else {
                    var_array[i].addDynamicTrigger(dt+idx, DomainRemoval, var_array[i].getMin());
                }
              
              D_DATA(
              for(int j=0; j<watch_index.size(); j++) {
                  D_ASSERT(watching_var[watch_index[j]]);
              }
              int count=0;
              for(int j=0; j<watching_var.size(); j++) {
                  if(watching_var[j]) count++;
              }
              D_ASSERT(count==numwatches);
              )
                
              return;
          }
      }
      
      // Didn't find a replacement watch. Prune value from  the other watches.
      watching_var[var_lost]=1;  // put this back.
      for(int i=0; i<numwatches; i++) {
          if(i!=idx) {
              var_array[watch_index[i]].removeFromDomain(value);
          }
      }
      propagate_mode=true;
      return;
      
  }

  virtual BOOL check_assignment(DomainInt* v, int v_size)
  {
    
    int count=0; 
    for(int i = 0; i < v_size; ++i) {
        if(v[i]==value) count++;
    }
    return count<=val_count;
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> vars;
    vars.reserve(var_array.size());
    for(unsigned i = 0; i < var_array.size(); ++i)
      vars.push_back(AnyVarRef(var_array[i]));
    return vars;  
  }
  
  /*virtual bool get_satisfying_assignment(box<pair<int,DomainInt> >& assignment)
  {
    pair<int, int> assign;
    for(int i = 0; i < var_array1.size(); ++i)
    {
      if(Operator::get_satisfying_assignment(var_array1[i], var_array2[i], assign))
      {
        D_ASSERT(var_array1[i].inDomain(assign.first));
        D_ASSERT(var_array2[i].inDomain(assign.second));
        D_ASSERT(Operator::check_assignment(assign.first, assign.second));
        assignment.push_back(make_pair(i, assign.first));
        assignment.push_back(make_pair(i + var_array1.size(), assign.second));
        return true;
      }
    }
    return false;
  }*/
  
  /*
  virtual AbstractConstraint* reverse_constraint()
  {
      vector<AbstractConstraint*> con;
      for(int i=0; i<var_array1.size(); i++)
      {
          con.push_back(Operator::reverse_constraint(stateObj, var_array1[i], var_array2[i]));
      }
      return new Dynamic_AND(stateObj, con);
  }*/
};
#endif
