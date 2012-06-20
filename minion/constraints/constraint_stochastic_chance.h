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

// Chance constraint for stochastic CSP.


// Variables are duplicated into the gadget for each sub-csp 

#ifndef CONSTRAINT_STOCHASTICCHANCE_H
#define CONSTRAINT_STOCHASTICCHANCE_H

#include "../preprocess.h"

template<typename VarArray>
struct StochasticChance : public AbstractConstraint, Backtrackable
{
  virtual string constraint_name()
  { return "StochasticChance"; }
  
  VarArray var_array;
  
  std::vector<std::vector<int> > var_indices;     // vars are actually two-dimensional -- these indices map from 2d to 1d.
  
  vector<StateObj*> gadget_stateObj;   //  stateObj for each sub-csp.
  
  vector<vector<AnyVarRef> > inner_vars;
  
  vector<int> scenarios;
  vector<int> scenario_probs;   // probabilities as an integer -- same range as alpha.
  int alpha;  // Threshold probability
  int numscenarios;  
  
  int sumconst;   // sum >= sumconst
  
  int numvals;
    int dom_min;
    int dom_max;
    
    StochasticChance(StateObj* _stateObj, const VarArray& _var_array, vector<int> _scenarios, vector<int> _scenario_probabilities, vector<int> sumconst_alpha) : AbstractConstraint(_stateObj), 
    var_array(_var_array), scenarios(_scenarios), scenario_probs(_scenario_probabilities), sumconst(sumconst_alpha[0]), alpha(sumconst_alpha[1])
    {
        getState(stateObj).getGenericBacktracker().add(this);
        
        make_disjunct_csps();
        
        failed_stack.push_back(-1); // marker. May not be necessary.
        
        dom_max=var_array[0].getInitialMax();
        dom_min=var_array[0].getInitialMin();
        for(int i=1; i<var_array.size(); i++) {
            if(var_array[i].getInitialMin()<dom_min) dom_min=var_array[i].getInitialMin();
            if(var_array[i].getInitialMax()>dom_max) dom_max=var_array[i].getInitialMax();
        }
        numvals=dom_max-dom_min+1;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //   Backtracking functions.
    
    // Stack of child CSPS that have failed, and need to be un-failed when backtracking.
    vector<int> failed_stack;
    
    void mark() {
        failed_stack.push_back(-1);  // marker.
        
        for(int i=0; i<gadget_stateObj.size(); i++) {
            if(!getState(gadget_stateObj[i]).isFailed()) {
                Controller::world_push(gadget_stateObj[i]);
            }
        }
    }
    
    void pop() {
        while(failed_stack.back()!=-1) {
            D_ASSERT(getState(gadget_stateObj[failed_stack.back()]).isFailed());
            getState(gadget_stateObj[failed_stack.back()]).setFailed(false);
            //cout << "Restoring failure state of: " << failed_stack.back() <<endl;
            D_ASSERT(!getState(gadget_stateObj[failed_stack.back()]).isFailed());
            failed_stack.pop_back();
        }
        // pop the marker.
        failed_stack.pop_back();
        
        // Backtrack each of the child CSPs.
        for(int i=0; i<gadget_stateObj.size(); i++) {
            if(!getState(gadget_stateObj[i]).isFailed()) {
                Controller::world_pop(gadget_stateObj[i]);
            }
        }
    }
    
  virtual ~StochasticChance()
  { for(int i=0; i<gadget_stateObj.size(); i++) delete gadget_stateObj[i]; }
  
  virtual AbstractConstraint* reverse_constraint()
  {
    cerr << "You can't reverse a stochastic chance constraint!";
    FAIL_EXIT();
  }
  
  int dynamic_trigger_count() { 
      return var_array.size()*numvals;
  }
  
  inline void attach_trigger(int var, int val) {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+(var*numvals)+(val-dom_min);
      D_ASSERT(!dt->isAttached());
      
      var_array[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  virtual BOOL check_assignment(DomainInt* v, int v_size)
  { 
    //cout << "Constructive OR Assignment:" << v[0] << "," << v[1] << endl;
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
  
  virtual void propagate(DynamicTrigger* dt)
  {
      if(getState(stateObj).isFailed()) return;
      
      int pos=dt-dynamic_trigger_start();
      int var=pos/numvals;
      int val=pos-(var*numvals)+dom_min;
      
      // Prune val in all child CSPs that have not failed.
      for(int i=0; i<gadget_stateObj.size(); i++) {
          if(!getState(gadget_stateObj[i]).isFailed()) {
              inner_vars[i][var].removeFromDomain(val);
              if(getState(gadget_stateObj[i]).isFailed()) {
                  failed_stack.push_back(i);
                  getQueue(gadget_stateObj[i]).clearQueues();   // In case some things have been queued.
              }
          }
          
      }
      
      do_prop_incremental();
  }
  
  
  virtual void full_propagate()
  {
      //for(int i=0; i<gadget_stateObj.size(); i++) D_ASSERT(!getState(gadget_stateObj[i]).isFailed());
      // Actually child constraints can already be failed since BuildCSP does an initial propagation.
      
      if(getState(stateObj).isFailed()) return;
      
      // Copy domains
      for(int i = 0; i < (var_array.size()/numscenarios) ; ++i)
      {
          DomainInt min_val = var_array[i].getMin();
          DomainInt max_val = var_array[i].getMax();
          for(int j=0; j<inner_vars.size(); j++) {
              inner_vars[j][i].setMin(min_val);
              inner_vars[j][i].setMax(max_val);
          }
          
          for(int j = min_val + 1; j < max_val; ++j)
          if(!var_array[i].inDomain(j)) {
              for(int k=0; k<inner_vars.size(); k++) {
                  inner_vars[k][i].removeFromDomain(j);
              }
          }
      }
      
      do_prop_incremental();
      
      for(int var=0; var< (var_array.size()/numscenarios) ; var++) {
          for(int val=dom_min; val<=dom_max; val++) {
              if(var_array[var].inDomain(val)) {
                  attach_trigger(var, val);
              }
          }
      }
      
  }
  
  void do_prop_incremental()
  { 
    // Assume domains of child CSPs have already been updated by propagate function.
    // Propagate the child CSPs
    
    /*cout << "Child CSP variables before propagation: " << endl;
    for(int i=0; i<gadget_stateObj.size(); i++) {
        cout <<"Child "<<i << " failed:"<<getState(gadget_stateObj[i]).isFailed()<< endl;
        for(int j=0; j<var_array.size(); j++) {
            cout << "var:"<<j<<endl;
            for(int k=inner_vars[i][j].getMin(); k<=inner_vars[i][j].getMax(); k++) {
                if(inner_vars[i][j].inDomain(k)) cout << k << ",";
            }
        }
    }*/
    
    for(int i=0; i<gadget_stateObj.size(); i++) {
        if(!getState(gadget_stateObj[i]).isFailed()) {
            PropogateCSP(gadget_stateObj[i], PropLevel_GAC, inner_vars[i]);
            
            if(getState(gadget_stateObj[i]).isFailed()) {
                //cout << "Recording failed state of " << i << " for backtracking" <<endl;
                failed_stack.push_back(i);   // Need to reset failed flag when backtracking.
                getQueue(gadget_stateObj[i]).clearQueues();   // In case propagation didn't empty the queue.
            }
        }
        
    }
    
    
    
    for(int i=0; i< (var_array.size()/numscenarios); i++) {
        for(int val=var_array[i].getMin(); val<=var_array[i].getMax(); val++) {
            if(var_array[i].inDomain(val)) {
                
                // Count the scenarios where val is supported.  Counters should be incremental but they aren't.
                
                int support_probability=0;
                
                for(int j=0; j<gadget_stateObj.size(); j++) {
                    if(!(getState(gadget_stateObj[j]).isFailed()) && inner_vars[j][i].inDomain(val)) {
                        support_probability+=scenario_probs[i];
                    }
                }
                
                if(support_probability<alpha) {
                    // prune the value. 
                    var_array[i].removeFromDomain(val);
                }
                
            }
        }
    }
    
  }
  
  //////////////////////////////////////////////////////////////////////////////
  // diseq or abs.
    /*void make_disjunct_csps()
    {
        Bounds var1bounds(var_array[0].getInitialMin(), var_array[0].getInitialMax());
        Bounds var2bounds(var_array[1].getInitialMin(), var_array[1].getInitialMax());
        vector<int> var1dom; var1dom.push_back(var_array[0].getInitialMin()); var1dom.push_back(var_array[0].getInitialMax());
        vector<int> var2dom; var2dom.push_back(var_array[1].getInitialMin()); var2dom.push_back(var_array[1].getInitialMax());
        
        
        // For each disjunct, make a CSPInstance and then call BuildCSP for it.
        StateObj* diseq_stateObj=new StateObj();
        gadget_stateObj.push_back(diseq_stateObj);
        
        StateObj* abs_stateObj=new StateObj();
        gadget_stateObj.push_back(abs_stateObj);
        
        CSPInstance cspi;
        
        Var var1 = cspi.vars.getNewVar(VAR_DISCRETE, var1dom);
        cspi.vars.addSymbol("x1", var1);
        cspi.all_vars_list.push_back(make_vec(var1));
        
        Var var2 = cspi.vars.getNewVar(VAR_DISCRETE, var2dom);
        cspi.vars.addSymbol("x2", var2);
        cspi.all_vars_list.push_back(make_vec(var2));
        
        
        
        vector<vector<Var> > cvars;
        cvars.resize(2);
        cvars[0].push_back(var1);
        cvars[1].push_back(var2);
        
        
        ConstraintBlob cblob(&(constraint_list[11]), cvars);  // diseq.
        
        cspi.add_constraint(cblob);
        
        BuildCSP(diseq_stateObj, cspi);
        
        // Stick an abs in there instead
        ConstraintBlob cblob2(&(constraint_list[17]), cvars);  // abs.
        
        cspi.add_constraint(cblob2);
        
        BuildCSP(abs_stateObj, cspi);
        
        inner_vars.resize(2);  // two constraints.
        
        // diseq
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var1));
        inner_vars[0].push_back(get_AnyVarRef_from_Var(diseq_stateObj, var2));
        
        // abs
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var1));
        inner_vars[1].push_back(get_AnyVarRef_from_Var(abs_stateObj, var2));
        
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Element
    /*
    void make_disjunct_csps()
    {
        // Copy variables 
        vector<Bounds> varbounds;
        vector<vector<int> > vardoms;
        for(int i=0; i<var_array.size(); i++) {
            Bounds b(var_array[i].getInitialMin(), var_array[i].getInitialMax());
            varbounds.push_back(b);
            
            vector<int> varidom; varidom.push_back(var_array[i].getInitialMin()); varidom.push_back(var_array[i].getInitialMax());
            vardoms.push_back(varidom);
        }
        
        // One disjunct per variable in vector
        for(int i=0; i<var_array.size()-2; i++) {
            StateObj* temp=new StateObj();
            gadget_stateObj.push_back(temp);
        }
        
        CSPInstance cspi;
        
        // Put vars into cspi
        vector<Var> vars;
        for(int i=0; i<var_array.size(); i++) {
            Var vari = cspi.vars.getNewVar(VAR_DISCRETE, vardoms[i]);
            
            std::stringstream ss; ss << i;  // convert int to string in ridiculous C++ fashion.
            
            cspi.vars.addSymbol(string("x")+ss.str(), vari);
            cspi.all_vars_list.push_back(make_vec(vari));
            vars.push_back(vari);
        }
        
        // Put constraint into cspi
        inner_vars.resize(var_array.size()-2);
        for(int i=0; i<var_array.size()-2; i++) {
            
            vector<vector<Var> > eqvars;
            eqvars.resize(2);
            eqvars[0].push_back(vars[var_array.size()-1]);
            eqvars[1].push_back(vars[i]);
            
            ConstraintBlob eqblob(&(constraint_list[72]), eqvars);    // GACeq
            
            vector<vector<Var> > litvars;
            litvars.resize(1);
            litvars[0].push_back(vars[var_array.size()-2]);
            
            ConstraintBlob litblob(&(constraint_list[56]), litvars);   // w-literal
            
            litblob.constants.resize(1);
            litblob.constants[0].push_back(i);
            
            cspi.add_constraint(eqblob);
            cspi.add_constraint(litblob);
            
            BuildCSP(gadget_stateObj[i], cspi);
            
            for(int j=0; j<var_array.size(); j++)
                inner_vars[i].push_back(get_AnyVarRef_from_Var(gadget_stateObj[i], vars[j]));
        }
    }
    */
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing.
    /*
    void make_disjunct_csps()
    {
        // Copy variables 
        vector<Bounds> varbounds;
        vector<vector<int> > vardoms;
        for(int i=0; i<var_array.size(); i++) {
            Bounds b(var_array[i].getInitialMin(), var_array[i].getInitialMax());
            varbounds.push_back(b);
            
            vector<int> varidom; varidom.push_back(var_array[i].getInitialMin()); varidom.push_back(var_array[i].getInitialMax());
            vardoms.push_back(varidom);
        }
        
        // Four disjuncts
        for(int i=0; i<4; i++) {
            StateObj* temp=new StateObj();
            gadget_stateObj.push_back(temp);
        }
        
        CSPInstance cspi;
        
        // Put vars into cspi
        vector<Var> vars;
        for(int i=0; i<var_array.size(); i++) {
            Var vari = cspi.vars.getNewVar(VAR_DISCRETE, vardoms[i]);
            
            std::stringstream ss; ss << i;  // convert int to string in ridiculous C++ fashion.
            
            cspi.vars.addSymbol(string("x")+ss.str(), vari);
            cspi.all_vars_list.push_back(make_vec(vari));
            vars.push_back(vari);
        }
        
        // Put constraint into cspi
        {vector<vector<Var> > ineqvars;
        ineqvars.resize(2);
        ineqvars[0].push_back(vars[0]);
        ineqvars[1].push_back(vars[2]);
        
        ConstraintBlob ineqblob(&(constraint_list[18]), ineqvars);   // ineq
        
        ineqblob.constants.resize(1);
        ineqblob.constants[0].push_back(-var_array[4].getInitialMin());
        
        cspi.add_constraint(ineqblob);
        
        BuildCSP(gadget_stateObj[0], cspi);}
        
        {vector<vector<Var> > ineqvars;
        ineqvars.resize(2);
        ineqvars[0].push_back(vars[2]);
        ineqvars[1].push_back(vars[0]);
        
        ConstraintBlob ineqblob(&(constraint_list[18]), ineqvars);   // ineq
        
        ineqblob.constants.resize(1);
        ineqblob.constants[0].push_back(-var_array[5].getInitialMin());
        
        cspi.add_constraint(ineqblob);
        
        BuildCSP(gadget_stateObj[1], cspi);}
        
        {vector<vector<Var> > ineqvars;
        ineqvars.resize(2);
        ineqvars[0].push_back(vars[1]);
        ineqvars[1].push_back(vars[3]);
        
        ConstraintBlob ineqblob(&(constraint_list[18]), ineqvars);   // ineq
        
        ineqblob.constants.resize(1);
        ineqblob.constants[0].push_back(-var_array[4].getInitialMin());
        
        cspi.add_constraint(ineqblob);
        
        BuildCSP(gadget_stateObj[2], cspi);}
        
        {vector<vector<Var> > ineqvars;
        ineqvars.resize(2);
        ineqvars[0].push_back(vars[3]);
        ineqvars[1].push_back(vars[1]);
        
        ConstraintBlob ineqblob(&(constraint_list[18]), ineqvars);   // ineq
        
        ineqblob.constants.resize(1);
        ineqblob.constants[0].push_back(-var_array[5].getInitialMin());
        
        cspi.add_constraint(ineqblob);
        
        BuildCSP(gadget_stateObj[3], cspi);}
        
        
        
        
        inner_vars.resize(4);
        for(int i=0; i<4; i++) {
            for(int j=0; j<var_array.size(); j++)
                inner_vars[i].push_back(get_AnyVarRef_from_Var(gadget_stateObj[i], vars[j]));
        }
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Hack in a stochastic csp example. 
    
    void make_disjunct_csps()
    {
        // Four scenarios. 
        
        int numcopyvars=2;
        numscenarios=4;
        
        // Copy variables 
        vector<Bounds> varbounds;
        vector<vector<int> > vardoms;
        
        for(int i=0; i<numcopyvars; i++) {
            Bounds b(var_array[i].getInitialMin(), var_array[i].getInitialMax());
            varbounds.push_back(b);
            
            vector<int> varidom; varidom.push_back(var_array[i].getInitialMin()); varidom.push_back(var_array[i].getInitialMax());
            vardoms.push_back(varidom);
        }
        
        // One disjunct per variable in each vector
        for(int i=0; i<numscenarios; i++) {
            StateObj* temp=new StateObj();
            gadget_stateObj.push_back(temp);
        }
        
        CSPInstance cspi;
        
        // Put vars into cspi
        vector<Var> vars;
        
        for(int i=0; i<numcopyvars; i++) {
            Var vari = cspi.vars.getNewVar(VAR_DISCRETE, vardoms[i]);
            
            std::stringstream ss; ss << i;  // convert int to string in ridiculous C++ fashion.
            
            cspi.vars.addSymbol(string("x")+ss.str(), vari);
            cspi.all_vars_list.push_back(make_vec(vari));
            vars.push_back(vari);
            
        }
        
        // put in constant 30. 
        vector<int> bnds; bnds.push_back(sumconst); bnds.push_back(sumconst); 
        Var vari = cspi.vars.getNewVar(VAR_DISCRETE, bnds);
        cspi.vars.addSymbol(string("sumconst"), vari);
        cspi.all_vars_list.push_back(make_vec(vari));
        vars.push_back(vari);
        
        
        // Put constraint into cspi
        inner_vars.resize(numscenarios);
        
        for(int i=0; i<numscenarios; i++) {
            vector<vector<int> > constblob;
            vector<vector<Var> > eqvars;
            
            eqvars.resize(2);
            
            for(int k=0; k<numcopyvars; k++) {
                eqvars[0].push_back(vars[k]);
            }
            
            eqvars[1].push_back(vars[numcopyvars]);
            
            vector<int> scenario;
            
            for(int j=numcopyvars*i; j<numcopyvars*(i+1); j++) scenario.push_back(scenarios[j]);
            
            ConstraintBlob eqblob(&(constraint_list[32]), eqvars);    // weighted sum geq
            eqblob.constants.push_back(scenario);
            cspi.add_constraint(eqblob);
            
            BuildCSP(gadget_stateObj[i], cspi);
            
            for(int j=0; j<numcopyvars; j++)
                inner_vars[i].push_back(get_AnyVarRef_from_Var(gadget_stateObj[i], vars[j]));
        }
    }
    
    
};

#endif
