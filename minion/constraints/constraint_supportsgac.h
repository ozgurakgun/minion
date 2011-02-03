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

// The algorithm iGAC or short-supports-gac

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT true

// Switches on the zeroVals array. 
// This flag is a small slowdown on qg-supportsgac-7-9 -findallsols
// 
#define SupportsGACUseZeroVals true

template<typename VarArray>
struct ShortSupportsGAC : public AbstractConstraint, Backtrackable
{
    /*struct Support {
        vector<Support*> prev;   // Size r -- some entries null.
        vector<Support*> next;   
        
        // Prev and next are indexed by variable. Must be Null if the support does
        // not include that variable. 
        
        vector<pair<int,int> > literals;
        
        Support(int numvars)
        {
            prev.resize(numvars, 0);
            next.resize(numvars, 0);
        }
        
        // Blank one for use as list header. Must resize next before use.
        Support() {}
    };*/
    
    // Memory layout of support is:
    // prev (numvars*sizeof(void*))
    // next "
    // size of literals (sizeof(int))
    // literals (numvars*sizeof(pair<int,int>))
    
    void* getNext(void* sup, int var) {
        return ((void**)sup)[var+vars.size()];
    }
    
    void* getPrev(void* sup, int var) {
        return ((void**)sup)[var];
    }
    
    void setNext(void * sup, int var, void* assign) {
        ((void**)sup)[var+vars.size()]=assign;
    }
    
    void setPrev(void* sup, int var, void* assign) {
        ((void**)sup)[var]=assign;
    }
    
    pair<int, int> getLit(void* sup, int litnum) {
        int* litsizeptr=((int*)   (((void**)sup)+(vars.size()*2)) );
        pair<int,int>* startptr=((pair<int,int>*)   (litsizeptr+1));
        D_ASSERT(litnum<(*(litsizeptr)));
        return startptr[litnum];
    }
    
    void setLit(void* sup, int litnum, pair<int,int> assign) {
        int* litsizeptr=((int*)   (((void**)sup)+(vars.size()*2)) );
        int litsize=*(litsizeptr);
        pair<int,int>* startptr=((pair<int,int>*)   (litsizeptr+1));
        D_ASSERT(litnum<litsize);
        startptr[litnum]=assign;
    }
    
    int getLitSize(void* sup) {
        int* litsizeptr=((int*)   (((void**)sup)+(vars.size()*2)) );
        return *(litsizeptr);
    }
    
    void setLitSize(void* sup, int litsize) {
        int* litsizeptr=((int*)   (((void**)sup)+(vars.size()*2)) );
        *(litsizeptr)=litsize;
    }
    
    
    
    virtual string constraint_name()
    {
        return "ShortSupportsGAC";
    }
    
    VarArray vars;
    
    int numvals;
    int dom_min;
    int dom_max;
    
    // Counters
    int supports;   // 0 to rd.  
    vector<int> supportsPerVar;
    vector<vector<int> > supportsPerLit;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<vector<void*> >  supportListPerLit;
    
    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    #if SupportsGACUseZeroVals
    vector<vector<int> > zeroVals;
    vector<vector<char> > inZeroVals;  // is a var/val in zeroVals
    #endif
    
    // Partition of variables by number of supports.
    vector<int> varsPerSupport;    // Permutation of the variables
    vector<int> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<int> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition
    
    void* supportFreeList;       // singly-linked list of spare Support objects.
    
    vector<vector<vector<vector<pair<int,int> > > > > tuple_lists;  // tuple_lists[var][val] is a vector 
    // of short supports for that var, val. Includes any supports that do not contain var at all.
    
    vector<vector<int> > tuple_list_pos;    // current position in tuple_lists (for each var and val). Wraps around.
    
    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    ShortSupportsGAC(StateObj* _stateObj, const VarArray& _var_array, TupleList* tuples) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0)
    {
        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        dom_max=vars[0].getInitialMax();
        dom_min=vars[0].getInitialMin();
        for(int i=1; i<vars.size(); i++) {
            if(vars[i].getInitialMin()<dom_min) dom_min=vars[i].getInitialMin();
            if(vars[i].getInitialMax()>dom_max) dom_max=vars[i].getInitialMax();
        }
        numvals=dom_max-dom_min+1;
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(vars.size(), 0);
        supportsPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) supportsPerLit[i].resize(numvals, 0);
        
        supportListPerLit.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            supportListPerLit[i].resize(numvals);
            for(int j=0; j<numvals; j++) supportListPerLit[i][j]=getFreeSupport();
        }
        
        #if SupportsGACUseZeroVals
        zeroVals.resize(vars.size());
        inZeroVals.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            zeroVals[i].reserve(numvals);  // reserve the maximum length.
            for(int j=dom_min; j<=dom_max; j++) zeroVals[i].push_back(j);
            inZeroVals[i].resize(numvals, true);
        }
        #endif
        
        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(int i=0; i<vars.size(); i++) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(vars.size()*numvals+1);
        supportNumPtrs[0]=0;
        for(int i=1; i<supportNumPtrs.size(); i++) supportNumPtrs[i]=vars.size();
        
        // Extract short supports from tuples if necessary.
        if(tuples->size()>1) {
            cout << "Tuple list passed to supportgac constraint should only contain one tuple, encoding a list of short supports." << endl; 
            abort();
        }
        if(tuples->size()==1) {
            vector<DomainInt> encoded = tuples->get_vector(0);
            vector<vector<pair<int, int> > > shortsupports;
            vector<pair<int, int> > temp;
            for(int i=0; i<encoded.size(); i=i+2) {
                if(encoded[i]==-1) {
                    // end of a short support.
                    if(encoded[i+1]!=-1) {
                        cout << "Split marker is -1,-1 in tuple for supportsgac." << endl;
                        abort();
                    }
                    shortsupports.push_back(temp);
                    temp.clear();
                }
                else
                {
                    if(encoded[i]<0 || encoded[i]>=vars.size()) {
                        cout << "Tuple passed into supportsgac does not correctly encode a set of short supports." << endl;
                        abort();
                    }
                    temp.push_back(make_pair(encoded[i], encoded[i+1])); 
                }
            }
            if(encoded[encoded.size()-2]!=-1 || encoded[encoded.size()-1]!=-1) {
                cout << "Last -1,-1 marker missing from tuple in supportsgac."<< endl;
                abort();
            }
            
            tuple_lists.resize(vars.size());
            tuple_list_pos.resize(vars.size());
            for(int var=0; var<vars.size(); var++) {
                tuple_lists[var].resize(numvals);
                tuple_list_pos[var].resize(numvals, 0);
                
                for(int val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                    // get short supports relevant to var,val.
                    for(int i=0; i<shortsupports.size(); i++) {
                        bool varin=false;
                        bool valmatches=true;
                        for(int j=0; j<shortsupports[i].size(); j++) {
                            if(shortsupports[i][j].first==var) {
                                varin=true;
                                if(shortsupports[i][j].second!=val) {
                                    valmatches=false;
                                }
                            }
                        }
                        
                        if(!varin || valmatches) {
                            // If the support doesn't include the var, or it 
                            // does include var,val then add it to the list.
                            tuple_lists[var][val-dom_min].push_back(shortsupports[i]);
                        }
                    }
                }
            }
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Dtor
    
    virtual ~ShortSupportsGAC() {
        //printStructures();
        set<void*> myset;
        
        // Go through supportFreeList
        for(int var=0; var<vars.size(); var++) {
            for(int val=dom_min; val<=dom_max; val++) {
                void* sup = getNext(supportListPerLit[var][val-dom_min], var);
                while(sup!=0) {
                    //vector<Support*>& prev=sup->prev;
                    //vector<Support*>& next=sup->next;
                    //vector<pair<int, int> >& litlist=sup->literals;
                    // Unstitch supList from all lists it is in.
                    for(int i=0; i<getLitSize(sup); i++) {
                        int var=getLit(sup,i).first;
                        //D_ASSERT(prev[var]!=0);  // Only for igac. Here it might not be in the list.
                        if(getPrev(sup, var)!=0) {
                            setNext(getPrev(sup, var), var, getNext(sup, var));
                            //prev[var]=0;
                        }
                        if(getNext(sup,var)!=0) {
                            setPrev(getNext(sup, var), var, getPrev(sup, var));
                            //next[var]=0;
                        }
                    }
                    
                    void* temp=sup;
                    sup=getNext(supportListPerLit[var][val-dom_min], var);
                    myset.insert(temp);
                }
            }
        }
        
        while(supportFreeList!=0) {
            void* sup=supportFreeList;
            supportFreeList=getNext(sup, 0);
            myset.insert(sup);
        }
        
        for(int i=0; i<backtrack_stack.size(); i++) {
            if(backtrack_stack[i].sup!=0) {
                myset.insert(backtrack_stack[i].sup);
            }
        }
        
        typename set<void*>::iterator it;
        for ( it=myset.begin() ; it != myset.end(); it++ ) {
            //delete *it;
            free(*it);
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Backtracking mechanism
    
    struct BTRecord {
        bool is_removal;   // removal or addition was made. 
        void* sup;
        
        /*friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec)
        {
            if(rec.sup==0) return o<<"ZeroMarker";
            o<<"BTRecord:"<<rec.is_removal<<",";
            o<< rec.sup->literals;
            return o;
        }*/
    };
    
    vector<BTRecord> backtrack_stack;
    
    void mark() {
        struct BTRecord temp = { false, 0 };
        backtrack_stack.push_back(temp);  // marker.
    }
    
    void pop() {
        //cout << "BACKTRACKING:" << endl;
        //cout << backtrack_stack <<endl;
        while(backtrack_stack.back().sup != 0) {
            BTRecord temp=backtrack_stack.back();
            backtrack_stack.pop_back();
            if(temp.is_removal) {
                addSupportInternal(0, temp.sup);
            }
            else {
                deleteSupportInternal(temp.sup, true);
            }
        }
        
        backtrack_stack.pop_back();  // Pop the marker.
        //cout << "END OF BACKTRACKING." << endl;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    void* addSupport(box<pair<int, DomainInt> >* litlist)
    {
        void* newsup=addSupportInternal(litlist, 0);
        struct BTRecord temp;
        temp.is_removal=false;
        temp.sup=newsup;
        backtrack_stack.push_back(temp);
        return newsup;
    }
    
    // Can take either a box or a support object (for use when backtracking). 
    void* addSupportInternal(box<pair<int, DomainInt> >* litbox, void* sup)
    {
        /*cout << "Entering addSupportInternal." << endl;
        if(litbox!=0) cout << "Adding a brand-new support." <<endl;
        printStructures();*/
        
        // add a new support given as a vector of literals.
        void* sup_internal;
        
        if(litbox!=0) {
            // copy.
            sup_internal=getFreeSupport();
            setLitSize(sup_internal, (*litbox).size());
            for(int i=0; i<litbox->size(); i++) setLit(sup_internal, i, (*litbox)[i]);
        }
        else {
            sup_internal=sup;
        }
        
        
        // vector<pair<int, int> >& litlist_internal=sup_internal->literals;
        
        //cout << "Adding support (internal) :" << litlist_internal << endl;
        //D_ASSERT(litlist_internal.size()>0);  // It should be possible to deal with empty supports, but currently they wil
        // cause a memory leak. 
        
        int litsize=getLitSize(sup_internal);
        for(int i=0; i<litsize; i++) {
            pair<int, int> temp=getLit(sup_internal, i);
            int var=temp.first;
            int val=temp.second-dom_min;
            
            // Stitch it into supportListPerLit
            setPrev(sup_internal, var, supportListPerLit[var][val]);
            setNext(sup_internal, var, getNext(supportListPerLit[var][val], var));
            setNext(supportListPerLit[var][val], var, sup_internal);
            if(getNext(sup_internal, var) != 0) {
                setPrev(getNext(sup_internal,var), var, sup_internal);
            }
            
            //update counters
            supportsPerVar[var]++;
            supportsPerLit[var][val]++;
                                     
            // Attach trigger if this is the first support containing var,val.
            if(SupportsGACUseDT && supportsPerLit[var][val]==1) {
                attach_trigger(var, val+dom_min);
            }
            
            // Update partition
            // swap var to the end of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]]-1]);
            // Move the boundary so var is now in the higher cell.
            supportNumPtrs[supportsPerVar[var]]--;
        }
        supports++;
        
        return sup_internal;
    }
    
    void deleteSupport(void* sup) {
        struct BTRecord temp;
        temp.is_removal=true;
        temp.sup=sup;
        backtrack_stack.push_back(temp);
        
        deleteSupportInternal(sup, false);
    }
    
    void deleteSupportInternal(void* sup, bool Backtracking) {
        D_ASSERT(sup!=0);
        
        // Remove sup from supportListPerLit
        //vector<Support*>& prev=sup->prev;
        //vector<Support*>& next=sup->next;
        //vector<pair<int, int> >& litlist=sup->literals;
        //cout << "Removing support (internal) :" << litlist << endl;
        int litlistsize=getLitSize(sup);
        for(int i=0; i<litlistsize; i++) {
            pair<int, int> lit=getLit(sup,i);
            int var=lit.first;
            D_ASSERT(getPrev(sup, var)!=0);
            setNext(getPrev(sup, var), var, getNext(sup, var));
            if(getNext(sup, var)!=0) {
                setPrev(getNext(sup,var), var, getPrev(sup, var));
            }
            
            // decrement counters
            supportsPerVar[var]--;
            supportsPerLit[var][lit.second-dom_min]--;
            D_ASSERT(supportsPerLit[var][lit.second-dom_min] >= 0);
            
            #if SupportsGACUseZeroVals
            if(supportsPerLit[var][lit.second-dom_min]==0) {
                if(!inZeroVals[var][lit.second-dom_min]) {
                    inZeroVals[var][lit.second-dom_min]=true;
                    zeroVals[var].push_back(lit.second);
                }
            }
            #endif
            
            // Remove trigger if this is the last support containing var,val.
            if(SupportsGACUseDT && supportsPerLit[var][lit.second-dom_min]==0) {
                detach_trigger(var, lit.second);
            }
            
            // Update partition
            // swap var to the start of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]+1]]);
            // Move the boundary so var is now in the lower cell.
            supportNumPtrs[supportsPerVar[var]+1]++;
        }
        supports--;
        
        //printStructures();
        
        if(Backtracking) {
            // Can re-use the support when it is removed by BT. 
            // Stick it on the free list using next[0] as the next ptr.
            setNext(sup, 0, supportFreeList);
            supportFreeList=sup;
        }
        // else can't re-use it because a ptr to it is on the BT stack. 
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // 
    
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        cout << "supports:" << supports <<endl;
        cout << "supportsPerVar:" << supportsPerVar << endl;
        cout << "supportsPerLit:" << supportsPerLit << endl;
        cout << "partition:" <<endl;
        for(int i=0; i<supportNumPtrs.size()-1; i++) {
            cout << "supports: "<< i<< "  vars: ";
            for(int j=supportNumPtrs[i]; j<supportNumPtrs[i+1]; j++) {
                cout << varsPerSupport[j]<< ", ";
            }
            cout << endl;
            if(supportNumPtrs[i+1]==vars.size()) break;
        }
        #if SupportsGACUseZeroVals
        cout << "zeroVals:" << zeroVals << endl;
        cout << "inZeroVals:" << inZeroVals << endl;
        #endif
        
        cout << "Supports for each literal:"<<endl;
        for(int var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(int val=dom_min; val<=dom_max; val++) {
                cout << "Value: "<<val<<endl;
                void* sup=getNext(supportListPerLit[var][val-dom_min], var);
                while(sup!=0) {
                    cout << "Support: " << endl;
                    for(int i=0; i<getLitSize(sup); i++) cout << getLit(sup, i) << ", " <<endl;
                    bool contains_varval=false;
                    for(int i=0; i<getLitSize(sup); i++) {
                        if(getLit(sup, i).first==var && getLit(sup,i).second==val)
                            contains_varval=true;
                    }
                    D_ASSERT(contains_varval);
                    
                    sup=getNext(sup, var);
                }
            }
        }
    }
    
    #if !SupportsGACUseDT
        virtual triggerCollection setup_internal()
        {
            triggerCollection t;
            int array_size = vars.size();
            for(int i = 0; i < array_size; ++i)
              t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
            return t;
        }
    #endif
    
    void partition_swap(int xi, int xj)
    {
        if(xi != xj) {
            varsPerSupport[varsPerSupInv[xj]]=xi;
            varsPerSupport[varsPerSupInv[xi]]=xj;
            int temp=varsPerSupInv[xi];
            varsPerSupInv[xi]=varsPerSupInv[xj];
            varsPerSupInv[xj]=temp;
        }
    }
    
    void findSupports()
    {
        // For each variable where the number of supports is equal to the total...
    restartloop:
        for(int i=supportNumPtrs[supports]; i<supportNumPtrs[supports+1]; i++) {
            int var=varsPerSupport[i];
            
            #if !SupportsGACUseZeroVals
            for(int val=vars[var].getMin(); val<=vars[var].getMax(); val++) {
            #else
            for(int j=0; j<zeroVals[var].size(); j++) {
                int val=zeroVals[var][j];
                if(supportsPerLit[var][val-dom_min]>0) {
                    // No longer a zero val. remove from vector.
                    zeroVals[var][j]=zeroVals[var][zeroVals[var].size()-1];
                    zeroVals[var].pop_back();
                    inZeroVals[var][val-dom_min]=false;
                    j--;
                    continue;
                }
            #endif
                
                if(vars[var].inDomain(val) && supportsPerLit[var][val-dom_min]==0) {
                    // val has no support. Find a new one. 
                    typedef pair<int,DomainInt> temptype;
                    MAKE_STACK_BOX(newsupportbox, temptype, vars.size()); 
                    bool foundsupport=findNewSupport(newsupportbox, var, val);
                    
                    if(!foundsupport) {
                        vars[var].removeFromDomain(val);
                    }
                    else {
                        addSupport(&newsupportbox);
                        
                        #if SupportsGACUseZeroVals
                        if(supportsPerLit[var][val-dom_min]>0) {
                            // No longer a zero val. remove from vector.
                            zeroVals[var][j]=zeroVals[var][zeroVals[var].size()-1];
                            zeroVals[var].pop_back();
                            inZeroVals[var][val-dom_min]=false;
                        }
                        #endif
                        
                        // supports has changed and so has supportNumPtrs so start again. 
                        // Tail recursion might be optimised?
                        // Should be a goto.
                        goto restartloop;
                        //findSupports();
                        //return;
                    }
                }
            }
        }
    }
    
    inline void updateCounters(int var, int val) {
        void* supList = getNext(supportListPerLit[var][val-dom_min], var);
        while(supList != 0) {
            void* next=getNext(supList, var);
            deleteSupport(supList);
            supList=next;
        }
    }
    
    
    #if SupportsGACUseDT
        int dynamic_trigger_count() { 
            return vars.size()*numvals;
        }
    #endif
    
  inline void attach_trigger(int var, int val)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+(var*numvals)+(val-dom_min);
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  inline void detach_trigger(int var, int val)
  {
      //P("Detach Triggers");
      
      D_ASSERT(supportsPerLit[var][val-dom_min] == 0);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      dt=dt+(var*numvals)+(val-dom_min);
      releaseTrigger(stateObj, dt );   // BT_CALL_BACKTRACK
  }
    
  virtual void propagate(int prop_var, DomainDelta)
  {
    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    //printStructures();
    D_ASSERT(!SupportsGACUseDT);  // Should not be here if using dynamic triggers.
    
    for(int val=dom_min; val<=dom_max; val++) {
        if(!vars[prop_var].inDomain(val) && supportsPerLit[prop_var][val-dom_min]>0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupports();
  }
  
    virtual void propagate(DynamicTrigger* dt)
  {
      int pos=dt-dynamic_trigger_start();
      int var=pos/numvals;
      int val=pos-(var*numvals)+dom_min;
      
      updateCounters(var, val);
      
      findSupports();
      
  }

    
    #define ADDTOASSIGNMENT(var, val) if(!vars[var].isAssigned()) assignment.push_back(make_pair(var,val));
    
    // For full-length support variant:
    #define ADDTOASSIGNMENTFL(var, val) assignment.push_back(make_pair(var,val));
    
    
    // Macro to add either the lower bound or the specified value for a particular variable vartopad
    // Intended to pad out an assignment to a full-length support.
    #define PADOUT(vartopad) if(var==vartopad) assignment.push_back(make_pair(var, val)); else assignment.push_back(make_pair(vartopad, vars[vartopad].getMin()));
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for pair-equals. a=b or c=d.
    
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        // a=b or c=d
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()==4);
        int othervar;
        if(var<=1) othervar=1-var;
        else othervar=(var==2? 3: 2);
        
        if(vars[othervar].inDomain(val)) {
            // If can satisfy the equality with var in it
            assignment.push_back(make_pair(var, val));
            assignment.push_back(make_pair(othervar, val));
            return true;
        }
        
        // Otherwise, try to satisfy the other equality.
        if(var<=1) {
            for(int otherval=vars[2].getMin(); otherval<=vars[2].getMax(); otherval++) {
                if(vars[2].inDomain(otherval) && vars[3].inDomain(otherval)) {
                    assignment.push_back(make_pair(2, otherval));
                    assignment.push_back(make_pair(3, otherval));
                    return true;
                }
            }
        }
        else {
            for(int otherval=vars[0].getMin(); otherval<=vars[0].getMax(); otherval++) {
                if(vars[0].inDomain(otherval) && vars[1].inDomain(otherval)) {
                    assignment.push_back(make_pair(0, otherval));
                    assignment.push_back(make_pair(1, otherval));
                    return true;
                }
            }
        }
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
      D_ASSERT(array_size == 4);
      
      if(v[0]==v[1] || v[2]==v[3]) return true;
      return false;
      
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for element
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        typedef typename VarArray::value_type VarRef;
        VarRef idxvar=vars[vars.size()-2];
        VarRef resultvar=vars[vars.size()-1];
        D_ASSERT(vars[var].inDomain(val));
        
        if(var<vars.size()-2) {
            // var is in the vector.
            
            for(int i=idxvar.getMin(); i<=idxvar.getMax(); i++) {
                if(idxvar.inDomain(i) && i>=0 && i<vars.size()-2) {
                    for(int j=resultvar.getMin(); j<=resultvar.getMax(); j++) {
                        if(resultvar.inDomain(j) && vars[i].inDomain(j) &&
                            (i!=var || j==val) ) {   // Either the support includes both var, val or neither -- if neither, it will be a support for var,val.
                            ADDTOASSIGNMENT(i,j);
                            ADDTOASSIGNMENT(vars.size()-2, i);
                            ADDTOASSIGNMENT(vars.size()-1, j);
                            return true;
                        }
                    }
                }
            }
        }
        else if(var==vars.size()-2) {
            // It's the index variable.
            if(val<0 || val>=vars.size()-2){
                return false;
            }
            
            for(int i=resultvar.getMin(); i<=resultvar.getMax(); i++) {
                if(resultvar.inDomain(i) && vars[val].inDomain(i)) {
                    ADDTOASSIGNMENT(vars.size()-2, val);
                    ADDTOASSIGNMENT(vars.size()-1, i);
                    ADDTOASSIGNMENT(val, i);
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(int i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    ADDTOASSIGNMENT(vars.size()-2, i);
                    ADDTOASSIGNMENT(vars.size()-1, val);
                    ADDTOASSIGNMENT(i, val);
                    return true;
                }
            }
        }
        return false;
        
        
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        int idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }
    */
    ////////////////////////////////////////////////////////////////////////////
    // ELEMENT - FULL LENGTH TUPLES VERSION.
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        typedef typename VarArray::value_type VarRef;
        VarRef idxvar=vars[vars.size()-2];
        VarRef resultvar=vars[vars.size()-1];
        D_ASSERT(vars[var].inDomain(val));
        
        if(var<vars.size()-2) {
            // var is in the vector.
            
            for(int i=idxvar.getMin(); i<=idxvar.getMax(); i++) {
                if(idxvar.inDomain(i) && i>=0 && i<vars.size()-2) {
                    for(int j=resultvar.getMin(); j<=resultvar.getMax(); j++) {
                        if(resultvar.inDomain(j) && vars[i].inDomain(j) &&
                            (i!=var || j==val) ) {   // Either the support includes both var, val or neither -- if neither, it will be a support for var,val.
                            assignment.push_back(make_pair(i, j));
                            assignment.push_back(make_pair(vars.size()-2, i));
                            assignment.push_back(make_pair(vars.size()-1, j));
                            for(int k=0; k<vars.size()-2; k++) {
                                if(k!=i) {
                                    if(k==var)
                                        assignment.push_back(make_pair(k, val));
                                    else
                                        assignment.push_back(make_pair(k, vars[k].getMin()));
                                }
                            }
                            return true;
                        }
                    }
                }
            }
        }
        else if(var==vars.size()-2) {
            // It's the index variable.
            if(val<0 || val>=vars.size()-2){
                return false;
            }
            
            for(int i=resultvar.getMin(); i<=resultvar.getMax(); i++) {
                if(resultvar.inDomain(i) && vars[val].inDomain(i)) {
                    assignment.push_back(make_pair(vars.size()-2, val));
                    assignment.push_back(make_pair(vars.size()-1, i));
                    assignment.push_back(make_pair(val, i));
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=val) assignment.push_back(make_pair(k, vars[k].getMin()));
                    }
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(int i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    assignment.push_back(make_pair(vars.size()-2, i));
                    assignment.push_back(make_pair(vars.size()-1, val));
                    assignment.push_back(make_pair(i, val));
                    for(int k=0; k<vars.size()-2; k++) {
                        if(k!=i) assignment.push_back(make_pair(k, vars[k].getMin()));
                    }
                    return true;
                }
            }
        }
        return false;
        
        
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        int idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }
    */
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for lexleq
    
    
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()%2==0);
        // First part of vars is vector 1.
        int vecsize=vars.size()/2;
        
        for(int i=0; i<vecsize; i++) {
            int j=i+vecsize;
            int jmax=vars[j].getMax();
            int imin=vars[i].getMin();
            
            // CASE 1   It is not possible for the pair to be equal or less.
            if(imin>jmax) {
                return false;
            }
            
            // CASE 2    It is only possible to make the pair equal.
            if(imin==jmax) {
                // check against var, val here.
                if(i==var && imin!=val) {
                    return false;
                }
                if(j==var && jmax!=val) {
                    return false;
                }
                
                ADDTOASSIGNMENT(i, imin);
                ADDTOASSIGNMENT(j, jmax);
                
                // Do not return, continue along the vector.
                continue;
            }
            
            // CASE 3    It is possible make the pair less.
            if(imin<jmax) {
                if(i==var) {
                    if(val==jmax) {
                        ADDTOASSIGNMENT(i, val);
                        ADDTOASSIGNMENT(j, val);
                        continue;
                    }
                    else if(val>jmax) {
                        return false;
                    }
                    else {   //  val<jmax
                        ADDTOASSIGNMENT(var, val);
                        ADDTOASSIGNMENT(j, jmax);
                        return true;
                    }
                }
                
                if(j==var) {
                    if(val==imin) {
                        ADDTOASSIGNMENT(i, val);
                        ADDTOASSIGNMENT(j, val);
                        continue;
                    }
                    else if(val<imin) {
                        return false;
                    }
                    else {   //  val>imin
                        ADDTOASSIGNMENT(var, val);
                        ADDTOASSIGNMENT(i, imin);
                        return true;
                    }
                }
                
                
                // BETTER NOT TO USE min and max here, should watch something in the middle of the domain...
                //int mid=imin + (jmax-imin)/2;
                //if(vars[i].inDomain(mid-1) && vars[j].inDomain(mid)) {
                //    ADDTOASSIGNMENT(i,mid-1);
                //    ADDTOASSIGNMENT(j,mid);
                //}
                //else {
                    ADDTOASSIGNMENT(i,imin);
                    ADDTOASSIGNMENT(j,jmax);
                
                return true;
            }
            
        }
        
        // Got to end of vector without finding a pair that can satisfy
        // the ct. However this is equal....
        return true;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        D_ASSERT(array_size%2==0);
        for(int i=0; i<array_size/2; i++)
        {
            if(v[i]<v[i+array_size/2]) return true;
            if(v[i]>v[i+array_size/2]) return false;
        }
        return true;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Lexleq with full-length supports.
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()%2==0);
        // First part of vars is vector 1.
        int vecsize=vars.size()/2;
        
        for(int i=0; i<vecsize; i++) {
            int j=i+vecsize;
            int jmax=vars[j].getMax();
            int imin=vars[i].getMin();
            
            // CASE 1   It is not possible for the pair to be equal or less.
            if(imin>jmax) {
                return false;
            }
            
            // CASE 2    It is only possible to make the pair equal.
            if(imin==jmax) {
                // check against var, val here.
                if(i==var && imin!=val) {
                    return false;
                }
                if(j==var && jmax!=val) {
                    return false;
                }
                
                ADDTOASSIGNMENTFL(i, imin);
                ADDTOASSIGNMENTFL(j, jmax);
                
                // Do not return, continue along the vector.
                continue;
            }
            
            // CASE 3    It is possible make the pair less.
            if(imin<jmax) {
                if(i==var) {
                    if(val==jmax) {
                        ADDTOASSIGNMENTFL(i, val);
                        ADDTOASSIGNMENTFL(j, val);
                        continue;
                    }
                    else if(val>jmax) {
                        return false;
                    }
                    else {   //  val<jmax
                        ADDTOASSIGNMENTFL(var, val);
                        ADDTOASSIGNMENTFL(j, jmax);
                        for(int k=i+1; k<vecsize; k++) {
                            PADOUT(k);
                            PADOUT(k+vecsize);
                        }
                        
                        return true;
                    }
                }
                
                if(j==var) {
                    if(val==imin) {
                        ADDTOASSIGNMENTFL(i, val);
                        ADDTOASSIGNMENTFL(j, val);
                        continue;
                    }
                    else if(val<imin) {
                        return false;
                    }
                    else {   //  val>imin
                        ADDTOASSIGNMENTFL(var, val);
                        ADDTOASSIGNMENTFL(i, imin);
                        for(int k=i+1; k<vecsize; k++) {
                            PADOUT(k);
                            PADOUT(k+vecsize);
                        }
                        
                        return true;
                    }
                }
                
                ADDTOASSIGNMENTFL(i,imin);
                ADDTOASSIGNMENTFL(j,jmax);
                for(int k=i+1; k<vecsize; k++) {
                    PADOUT(k);
                    PADOUT(k+vecsize);
                }
                
                return true;
            }
            
        }
        
        // Got to end of vector without finding a pair that can satisfy
        // the ct. However this is equal....
        return true;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        D_ASSERT(array_size%2==0);
        for(int i=0; i<array_size/2; i++)
        {
            if(v[i]<v[i+array_size/2]) return true;
            if(v[i]>v[i+array_size/2]) return false;
        }
        return true;
    }
    */
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Table of short supports passed in.
    
    /*bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(tuple_lists.size()==vars.size());
        
        const vector<vector<pair<int, int> > >& tuplist=tuple_lists[var][val-dom_min]; 
        
        int listsize=tuplist.size();
        for(int i=tuple_list_pos[var][val-dom_min]; i<listsize; i++) {
            
            int supsize=tuplist[i].size();
            bool valid=true;
            
            for(int j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(int j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val-dom_min]=i;
                return true;
            }
        }
        
        
        for(int i=0; i<tuple_list_pos[var][val-dom_min]; i++) {
            
            int supsize=tuplist[i].size();
            bool valid=true;
            
            for(int j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(int j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val-dom_min]=i;
                return true;
            }
        }
        return false;
    }
    
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        // argh, how to do this.
        // test with element first
        
        int idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing.
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant).
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        int i=vars[4].getAssignedValue();
        int j=vars[5].getAssignedValue();
        
        // If objects totally disjoint in either dimension...
        // x
        if( (vars[0].getMax()+i <= vars[2].getMin()) 
            || (vars[2].getMax()+j <= vars[0].getMin())
        // y
            || (vars[1].getMax()+i <= vars[3].getMin()) 
            || (vars[3].getMax()+j <= vars[1].getMin()) )
        {
            return true;
        }
        
        // object i below object j.
        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    ADDTOASSIGNMENT(1, val);
                    ADDTOASSIGNMENT(3, vars[3].getMax());
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    ADDTOASSIGNMENT(1, vars[1].getMin());
                    ADDTOASSIGNMENT(3, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(1, vars[1].getMin());
                ADDTOASSIGNMENT(3, vars[3].getMax());
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    ADDTOASSIGNMENT(1, val);
                    ADDTOASSIGNMENT(3, vars[3].getMin());
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    ADDTOASSIGNMENT(1, vars[1].getMax());
                    ADDTOASSIGNMENT(3, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(1, vars[1].getMax());
                ADDTOASSIGNMENT(3, vars[3].getMin());
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    ADDTOASSIGNMENT(0, val);
                    ADDTOASSIGNMENT(2, vars[2].getMax());
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    ADDTOASSIGNMENT(0, vars[0].getMin());
                    ADDTOASSIGNMENT(2, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(0, vars[0].getMin());
                ADDTOASSIGNMENT(2, vars[2].getMax());
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    ADDTOASSIGNMENT(0, val);
                    ADDTOASSIGNMENT(2, vars[2].getMin());
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    ADDTOASSIGNMENT(0, vars[0].getMax());
                    ADDTOASSIGNMENT(2, val);
                    return true;
                }
            }
            else {
                ADDTOASSIGNMENT(0, vars[0].getMax());
                ADDTOASSIGNMENT(2, vars[2].getMin());
                return true;
            }
        }
        
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        // argh, how to do this.
        // test with element first
        
        // object i above object j.
        if(v[1]+v[4] <=v[3]) {
            return true;
        }
        
        // object i below object j
        if(v[3]+v[5] <= v[1]) {
            return true;
        }
        
        // object i left of object j.
        if(v[0]+v[4] <=v[2]) {
            return true;
        }
        
        // object i right of object j
        if(v[2]+v[5] <= v[0]) {
            return true;
        }
        return false;
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing with full-length supports
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant).
    /*
    bool findNewSupport(box<pair<int, DomainInt> >& assignment, int var, int val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        int i=vars[4].getAssignedValue();
        int j=vars[5].getAssignedValue();
        
        // object i below object j.
        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    assignment.push_back(make_pair(1, val));
                    assignment.push_back(make_pair(3, vars[3].getMax()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    assignment.push_back(make_pair(1, vars[1].getMin()));
                    assignment.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(1, vars[1].getMin()));
                assignment.push_back(make_pair(3, vars[3].getMax()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    assignment.push_back(make_pair(1, val));
                    assignment.push_back(make_pair(3, vars[3].getMin()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    assignment.push_back(make_pair(1, vars[1].getMax()));
                    assignment.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(1, vars[1].getMax()));
                assignment.push_back(make_pair(3, vars[3].getMin()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    assignment.push_back(make_pair(0, val));
                    assignment.push_back(make_pair(2, vars[2].getMax()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    assignment.push_back(make_pair(0, vars[0].getMin()));
                    assignment.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(0, vars[0].getMin()));
                assignment.push_back(make_pair(2, vars[2].getMax()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    assignment.push_back(make_pair(0, val));
                    assignment.push_back(make_pair(2, vars[2].getMin()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    assignment.push_back(make_pair(0, vars[0].getMax()));
                    assignment.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                assignment.push_back(make_pair(0, vars[0].getMax()));
                assignment.push_back(make_pair(2, vars[2].getMin()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, int array_size)
    {
        // argh, how to do this.
        // test with element first
        
        // object i above object j.
        if(v[1]+v[4] <=v[3]) {
            return true;
        }
        
        // object i below object j
        if(v[3]+v[5] <= v[1]) {
            return true;
        }
        
        // object i left of object j.
        if(v[0]+v[4] <=v[2]) {
            return true;
        }
        
        // object i right of object j
        if(v[2]+v[5] <= v[0]) {
            return true;
        }
        return false;
    }
    */
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    void* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            void* newsup= malloc( sizeof(void*)*vars.size()*2 +sizeof(int)+ sizeof(pair<int,int>)*vars.size() );
            D_ASSERT(newsup!= 0);
            for(int i=0; i<vars.size(); i++) {
                setNext(newsup, i, 0);
                setPrev(newsup, i, 0);
            }
            return newsup;
        }
        else {
            void* temp=supportFreeList;
            //setNext(supportFreeList, 0, getNext(supportFreeList, 0)); garbage.
            supportFreeList=getNext(supportFreeList,0);
            return temp;
        }
    }
    
    virtual void full_propagate()
    {
        findSupports();
    }
    
    virtual vector<AnyVarRef> get_vars()
    {
      vector<AnyVarRef> ret;
      ret.reserve(vars.size());
      for(unsigned i = 0; i < vars.size(); ++i)
        ret.push_back(vars[i]);
      return ret;
    }
};  // end of class


