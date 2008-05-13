


#include <stdlib.h>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include "alldiff_common.h"

template<typename VarArray1, typename VarArray2>
struct GCC : public Constraint
{
    GCC(StateObj* _stateObj, const VarArray1& _var_array, const VarArray2& _capacity_array) : Constraint(_stateObj),
    stateObj(_stateObj), var_array(_var_array), capacity_array(_capacity_array)
    {
        dom_min=var_array[0].getInitialMin();
        dom_max=var_array[0].getInitialMax();
        
        for(int i=0; i<var_array.size(); ++i)
        {
          if(var_array[i].getInitialMin()<dom_min)
              dom_min=var_array[i].getInitialMin();
          if(var_array[i].getInitialMax()>dom_max)
              dom_max=var_array[i].getInitialMax();
          
          
        }
        numvars=var_array.size();  // number of main variables in the constraint
        numvals=dom_max-dom_min+1;
        if(capacity_array.size()>numvals)
        {
            numvals=capacity_array.size();
        }
        
        varvalmatching.resize(numvars, dom_min-1);
        usage.resize(numvals, 0);
        
        lower.resize(numvals);
        upper.resize(numvals);
        
        prev.resize(numvars+numvals);
        visited.reserve(numvars+numvals+1);
    }
    
    VarArray1 var_array;
    VarArray2 capacity_array;
    
    virtual void full_propagate()
    {
        D_ASSERT(capacity_array.size()==numvals);
        for(int i=0; i<numvals; i++)
        {
            capacity_array[i].setMin(0);
            capacity_array[i].setMax(numvars);
        }
        do_gcc_prop();
    }
    
    PROPAGATE_FUNCTION(int prop_var, DomainDelta)
    {
        do_gcc_prop();
    }
    
    void do_gcc_prop()
    {
        // find/ repair the matching.
        
        // this should not be necessary -- remove eventually
        //varvalmatching.resize(0);
        //varvalmatching.resize(numvars, dom_min-1);
        
        // populate lower and upper
        for(int i=0; i<numvals; i++)
        {
            lower[i]=capacity_array[i].getMin();
            upper[i]=capacity_array[i].getMax();
            //usage[i]=0;
        }
        
        bool flag=bfsmatching_gcc();
        cout << "matching:"<<flag<<endl;
        
        if(!flag)
        {
            getState(stateObj).setFailed(true);
            return;
        }
        
        // rest of algorihtm.
    }
    
    StateObj * stateObj;
    
    int dom_min, dom_max, numvars, numvals;
    deque<int> fifo;
    vector<int> prev;
    
    vector<int> matchbac;
    
    vector<int> lower;
    vector<int> upper;
    vector<int> usage;
    vector<int> usagebac;
    vector<int> varvalmatching;
    
    smallset_nolist visited;
    
    
    inline bool bfsmatching_gcc()
    {
        // lower and upper are indexed by value-dom_min and provide the capacities.
        // usage is the number of times a value is used in the matching.
        
        // back up the matching to cover failure
        matchbac=varvalmatching;
        usagebac=usage;
        
        // clear out unmatched variables
        for(int i=0; i<numvars; i++)
        {
            if(varvalmatching[i]!=dom_min-1
                && !var_array[i].inDomain(varvalmatching[i]))
            {
                usage[varvalmatching[i]-dom_min]--;
                varvalmatching[i]=dom_min-1;   // marker for unmatched.
            }
        }
        
        // If the upper bounds have been changed since last call, it is possible that
        // the usage[val] of some value is greater than upper[val]. This is impossible
        // in the flow graph, so it must be corrected before we run the algorithm.
        // Some values in the matching are changed to blank (dom_min-1).
        cout << varvalmatching << endl;
        cout << usage << endl;
        for(int valindex=0; valindex<=dom_max-dom_min; valindex++)
        {
            if(usage[valindex]>upper[valindex] && upper[valindex]>=0)
            {
                for(int i=0; i<numvars && usage[valindex]>upper[valindex]; i++)
                {
                    if(varvalmatching[i]==valindex+dom_min)
                    {
                        varvalmatching[i]=dom_min-1;
                        usage[valindex]--;
                    }
                }
                D_ASSERT(usage[valindex]<=upper[valindex]);
            }
        }
        
        int lowertotal=0;   // add up lower bounds
        for(int i=0; i<dom_max-dom_min; i++)
        {
            lowertotal+=lower[i];
        }
        if(lowertotal>numvars)
        {
            varvalmatching=matchbac;
            usage=usagebac;
            return false;
        }
        cout << "Lower:" <<lower<<endl;
        // iterate through the values looking for ones which are below their lower capacity bound. 
        for(int startvalindex=0; startvalindex<numvals; startvalindex++)
        {
            while(usage[startvalindex]<lower[startvalindex])
            {
                // usage of val needs to increase. Construct an augmenting path starting at val.
                int startval=startvalindex+dom_min;
                
                D_DATA(cout << "Searching for augmenting path for val: " << startval <<endl );
                // Matching edge lost; BFS search for augmenting path to fix it.
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvalindex+numvars);
                visited.clear();
                visited.insert(startvalindex+numvars);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    D_DATA(cout << "Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ) <<endl);
                    if(curnode<numvars)
                    { // it's a variable
                        // follow the matching edge, if there is one.
                        int valtoqueue=varvalmatching[curnode];
                        if(valtoqueue!=dom_min-1 
                            && !visited.in(valtoqueue-dom_min+numvars))
                        {
                            D_ASSERT(var_array[curnode].inDomain(valtoqueue));
                            int validx=valtoqueue-dom_min+numvars;
                            if(usage[valtoqueue-dom_min]>lower[valtoqueue-dom_min])
                            {
                                // can reduce the flow of valtoqueue to increase startval.
                                prev[validx]=curnode;
                                apply_augmenting_path(validx, startvalindex+numvars);
                                finished=true;
                            }
                            else
                            {
                                visited.insert(validx);
                                prev[validx]=curnode;
                                fifo.push_back(validx);
                            }
                        }
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        for(int vartoqueue=0; vartoqueue<numvars; vartoqueue++)
                        {
                            // For each variable, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            if(!visited.in(vartoqueue)
                                && var_array[vartoqueue].inDomain(stackval) 
                                && varvalmatching[vartoqueue]!=stackval)   // Need to exclude the matching edges????
                            {
                                // there is an edge from stackval to vartoqueue.
                                if(varvalmatching[vartoqueue]==dom_min-1)
                                {
                                    // vartoqueue terminates an odd alternating path.
                                    // Unwind and apply the path here
                                    prev[vartoqueue]=curnode;
                                    apply_augmenting_path(vartoqueue, startvalindex+numvars);
                                    finished=true;
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    // queue vartoqueue
                                    visited.insert(vartoqueue);
                                    prev[vartoqueue]=curnode;
                                    fifo.push_back(vartoqueue);
                                }
                            }
                        }  // end for.
                    }  // end value
                }  // end while
                if(!finished)
                {   // no augmenting path found
                    D_DATA(cout << "No augmenting path found."<<endl);
                    // restore the matching to its state before the algo was called.
                    varvalmatching=matchbac;
                    usage=usagebac;
                    return false;
                }
                
            }  // end while below lower bound.
        } // end for each value
        
        // now search for augmenting paths for unmatched vars.
        
        cout << "feasible matching (respects lower & upper bounds)"<<endl;
        cout << varvalmatching <<endl;
        
        // Flip the graph around, so it's like the alldiff case now. 
        // follow an edge in the matching from a value to a variable,
        // follow edges not in the matching from variables to values. 
        
        for(int startvar=0; startvar<numvars; startvar++)
        {
            if(varvalmatching[startvar]==dom_min-1)
            {
            // attempt to find an augmenting path starting with startvar.
                D_DATA(cout << "Searching for augmenting path for var: " << startvar <<endl );
                
                // done up to here.
                
                // Matching edge lost; BFS search for augmenting path to fix it.
                fifo.clear();  // this should be constant time but probably is not.
                fifo.push_back(startvar);
                visited.clear();
                visited.insert(startvar);
                bool finished=false;
                while(!fifo.empty() && !finished)
                {
                    // pop a vertex and expand it.
                    int curnode=fifo.front();
                    fifo.pop_front();
                    D_DATA(cout << "Popped vertex " << (curnode<numvars? "(var)":"(val)") << (curnode<numvars? curnode : curnode+dom_min-numvars ) <<endl);
                    if(curnode<numvars)
                    { // it's a variable
                        // follow all edges other than the matching edge. 
                        for(int valtoqueue=var_array[curnode].getMin(); valtoqueue<=var_array[curnode].getMax(); valtoqueue++)
                        {
                            // For each value, check if it terminates an odd alternating path
                            // and also queue it if it is suitable.
                            int validx=valtoqueue-dom_min+numvars;
                            if(valtoqueue!=varvalmatching[curnode]
                                && var_array[curnode].inDomain(valtoqueue)
                                && !visited.in(validx) )
                            {
                                // Does this terminate an augmenting path?
                                if(usage[valtoqueue-dom_min]<upper[valtoqueue-dom_min])
                                {
                                    // valtoqueue terminates an alternating path.
                                    // Unwind and apply the path here
                                    prev[validx]=curnode;
                                    apply_augmenting_path_reverse(validx, startvar);
                                    finished=true;
                                    break;  // get out of for loop
                                }
                                else
                                {
                                    // queue valtoqueue
                                    visited.insert(validx);
                                    prev[validx]=curnode;
                                    fifo.push_back(validx);
                                }
                            }
                        }  // end for.
                    }
                    else
                    { // popped a value from the stack.
                        D_ASSERT(curnode>=numvars && curnode < numvars+numvals);
                        int stackval=curnode+dom_min-numvars;
                        for(int vartoqueue=0; vartoqueue<numvars; vartoqueue++)
                        {
                            // For each variable which is matched to stackval, queue it.
                            if(!visited.in(vartoqueue)
                                && varvalmatching[vartoqueue]==stackval)
                            {
                                D_ASSERT(var_array[vartoqueue].inDomain(stackval));
                                // there is an edge from stackval to vartoqueue.
                                // queue vartoqueue
                                visited.insert(vartoqueue);
                                prev[vartoqueue]=curnode;
                                fifo.push_back(vartoqueue);
                            }
                        }  // end for.
                    }  // end value
                }  // end while
                if(!finished)
                {   // no augmenting path found
                    D_DATA(cout << "No augmenting path found."<<endl);
                    // restore the matching to its state before the algo was called.
                    varvalmatching=matchbac;
                    usage=usagebac;
                    return false;
                }
            }
        }
        
        cout << "maximum matching:"<<endl;
        cout << varvalmatching <<endl;
        
        return true;
    }
    
    inline void apply_augmenting_path(int unwindnode, int startnode)
    {
        D_DATA(cout << "Found augmenting path:" <<endl);
        vector<int> augpath;
        // starting at unwindnode, unwind the path and put it in augpath.
        // Then apply it.
        // Assumes prev contains vertex numbers, rather than vars and values.
        int curnode=unwindnode;
        while(curnode!=startnode)
        {
            augpath.push_back(curnode);
            curnode=prev[curnode];
        }
        augpath.push_back(curnode);
        
        std::reverse(augpath.begin(), augpath.end());
        
        D_DATA(cout << "augpath:" << augpath<<endl);
        
        // now apply the path.
        for(int i=0; i<augpath.size()-1; i++)
        {
            if(augpath[i]<numvars)
            {
                // if it's a variable
                //D_ASSERT(varvalmatching[augpath[i]]==dom_min-1);
                // varvalmatching[augpath[i]]=dom_min-1; Can't do this, it would overwrite the correct value.
                D_DATA(cout << "decrementing usage for value " << augpath[i+1]-numvars+dom_min <<endl);
                usage[augpath[i+1]-numvars]--;
            }
            else
            {   // it's a value.
                D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                varvalmatching[augpath[i+1]]=augpath[i]-numvars+dom_min;
                D_DATA(cout << "incrementing usage for value " << augpath[i]-numvars+dom_min <<endl);
                usage[augpath[i]-numvars]++;
            }
        }
        
        
        #ifndef NO_DEBUG
        cout << "varvalmatching:";
        for(int i=0; i<numvars; i++)
        {
            if(var_array[i].inDomain(varvalmatching[i]))
                cout << i << "->" << varvalmatching[i] << ", ";
        }
        cout << endl;
        #endif
    }
    
    inline void apply_augmenting_path_reverse(int unwindnode, int startnode)
    {
        D_DATA(cout << "Found augmenting path:" <<endl);
        vector<int> augpath;
        // starting at unwindnode, unwind the path and put it in augpath.
        // Then apply it.
        // Assumes prev contains vertex numbers, rather than vars and values.
        int curnode=unwindnode;
        while(curnode!=startnode)
        {
            augpath.push_back(curnode);
            curnode=prev[curnode];
        }
        augpath.push_back(curnode);
        
        std::reverse(augpath.begin(), augpath.end());
        
        // now apply the path.
        for(int i=0; i<augpath.size()-1; i++)
        {
            if(augpath[i]<numvars)
            {
                // if it's a variable
                D_ASSERT(varvalmatching[augpath[i]]==dom_min-1);
                varvalmatching[augpath[i]]=augpath[i+1]-numvars+dom_min;
                usage[augpath[i+1]-numvars]++;
            }
            else
            {   // it's a value.
                D_ASSERT(augpath[i]>=numvars && augpath[i]<numvars+numvals);
                varvalmatching[augpath[i+1]]=dom_min-1;
                usage[augpath[i]-numvars]--;
            }
        }
        
        
        #ifndef NO_DEBUG
        cout << "augpath:" << augpath<<endl;
        cout << "varvalmatching:";
        for(int i=0; i<numvars; i++)
        {
            if(var_array[i].inDomain(varvalmatching[i]))
                cout << i << "->" << varvalmatching[i] << ", ";
        }
        cout << endl;
        #endif
    }
    
    virtual string constraint_name()
    {
      return "GCC";
    }
    
    virtual triggerCollection setup_internal()
    {
        D_INFO(2, DI_SUMCON, "Setting up Constraint");
        triggerCollection t;
        int array_size = var_array.size();
        int capacity_size=capacity_array.size();
        for(int i = 0; i < array_size; ++i)
          t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
        for(int i=0; i< capacity_size; ++i)
        {
            t.push_back(make_trigger(capacity_array[i], Trigger(this, i), DomainChanged));   // should be bounds only.
        }
        return t;
    }
    
    virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
      for(unsigned i = 0; i < capacity_array.size(); ++i)
	    vars.push_back(capacity_array[i]);
	  return vars;
	}
    
    // Does not work!!!
    virtual BOOL check_assignment(vector<DomainInt> v)
	{
	  D_ASSERT(v.size() == var_array.size()+capacity_array.size());
      vector<int> occ;
      occ.resize(numvals,0);
      for(int i=0; i<numvars; i++)
      {   // count the values.
          occ[v[i]-dom_min]++;
      }
      for(int i=dom_min; i<=dom_max; i++)
      {
          if(v[i+numvars-dom_min]!=occ[i-dom_min])
              return false;
      }
	  return true;
	}
};
