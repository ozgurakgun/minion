// CHECK full length and short support segments of code
//
//
// LIST BASED CODE WONT BE WORKING

// Started on git branch  supportsgac+bstable+adaptive
//      intended for supportsgac 
//      + long supports 
//      + better memory 
//      + backtrack stability
//      + adaptive use or ignoring of full length supports

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

#include <algorithm>
#include "boost/tuple/tuple_comparison.hpp"
#include "../constraints/constraint_checkassign.h"

// Default will be List.   
// If any special case is defined list will be switched off
// If two options given compile errors are expected to result.

#define UseElementShort false
#define UseElementLong false
#define UseLexLeqShort false
#define UseLexLeqLong false
#define UseSquarePackingShort false
#define UseSquarePackingLong false
#define UseList true
#define UseNDOneList false
#define SupportsGacNoCopyList true

#ifdef SUPPORTSGACELEMENT
#undef UseElementShort
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseElementShort true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACELEMENTLONG
#undef UseElementLong
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseElementLong true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACLEX
#undef UseLexLeqShort
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseLexLeqShort true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACLEXLONG
#undef UseLexLeqLong
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseLexLeqLong true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACSQUAREPACK
#undef UseSquarePackingShort
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseSquarePackingShort true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACSQUAREPACKLONG
#undef UseSquarePackingLong
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseSquarePackingLong true
#define UseList false
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACLIST
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseList true
#define UseNDOneList false
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACNDLIST
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseList false
#define UseNDOneList true
#define SupportsGacNoCopyList false
#endif

#ifdef SUPPORTSGACLISTNOCOPY
#undef UseList
#undef UseNDOneList
#undef SupportsGacNoCopyList
#define UseList true
#define UseNDOneList false
#define SupportsGacNoCopyList true
#endif

// The algorithm iGAC or short-supports-gac

// Does it place dynamic triggers for the supports.
#define SupportsGACUseDT true

// Switches on the zeroLits array. 
// This flag is a small slowdown on qg-supportsgac-7-9 -findallsols
// 

template<typename VarArray>
struct HaggisGACStable : public AbstractConstraint, Backtrackable
{
    virtual AbstractConstraint* reverse_constraint()
    { return forward_check_negation(stateObj, this); }

    struct Support ; 

    struct SupportCell { 
            SysInt literal ; 
            Support* sup ; 
            SupportCell* next ; 
            SupportCell* prev ; 
    };

    struct Literal { 
        SysInt var ; 
        DomainInt val ;
        SupportCell* supportCellList; 
//      Literal() { supportCellList = 0 ;} 
    };

    struct Support {
        vector<SupportCell> supportCells ;   // Size can't be more than r, but can be less.

        SysInt arity;              // could use vector.size() but don't want to destruct SupportCells when arity decreases
                                // or reconstruct existing ones when it increases.

        Support* nextFree ; // for when Support is in Free List.
        bool active;  
        SysInt  numLastSupported;
        
        Support()
        {
            supportCells.resize(0);
            arity=0;
            nextFree=0;
            active = true;  
            numLastSupported=0;
        }
    };
    
    virtual string constraint_name()
    {
        return "HaggisGACStable";
    }
    
    VarArray vars;

    vector<pair<SysInt,DomainInt> > literalsScratch;   // used instead of per-Support list, as scratch space
    
    SysInt numvals;
    SysInt numlits;
    
    // Counters
    SysInt supports;   // 0 to rd.  
    vector<SysInt> supportsPerVar;

    vector<SysInt> litsWithLostExplicitSupport;
    vector<SysInt> varsWithLostImplicitSupport;
    
    // 2d array (indexed by var then val) of sentinels,
    // at the head of list of supports. 
    // Needs a sentinel at the start so that dlx-style removals work correctly.
    vector<Literal>  literalList;
    vector<SysInt> firstLiteralPerVar;
  
    vector<Support*> lastSupportPerLit; // could be in Literal type
    vector<Support*> lastSupportPerVar;  
    vector<Support*> deletedSupports;

    // For each variable, a vector of values with 0 supports (or had 0 supports
    // when added to the vector).
    vector<vector<SysInt> > zeroLits;
    vector<char> inZeroLits;  // is a literal in zeroVals
    
    // Partition of variables by number of supports.
    vector<SysInt> varsPerSupport;    // Permutation of the variables
    vector<SysInt> varsPerSupInv;   // Inverse mapping of the above.
    
    vector<SysInt> supportNumPtrs;   // rd+1 indices into varsPerSupport representing the partition
    
    Support* supportFreeList;       // singly-linked list of spare Support objects.
    
    #if UseList
    #if !SupportsGacNoCopyList
    vector<vector<vector<vector<pair<SysInt, DomainInt> > > > > tuple_lists;  // tuple_lists[var][val] is a vector 
    // of short supports for that var, val. Includes any supports that do not contain var at all.
    #else
    vector<vector<vector<vector<pair<SysInt, DomainInt> > * > > > tuple_lists;
    #endif
    #endif
    
    #if UseNDOneList
    vector<vector<tuple<SysInt,SysInt,SysInt> > > tuple_nd_list; // The inner type is var,val,next-different-pos.
    #endif
    
    vector<vector<SysInt> > tuple_list_pos;    // current position in tuple_lists (for each var and val). Wraps around.
    
    struct SupportDeref
    {
        template<typename T>
        bool operator()(const T& lhs, const T& rhs)
        #if SupportsGacNoCopyList
        { return *lhs < *rhs; }
        #else
        { return lhs < rhs; }
        #endif
    };
    ////////////////////////////////////////////////////////////////////////////
    // Ctor
    
    HaggisGACStable(StateObj* _stateObj, const VarArray& _var_array, TupleList* tuples) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0)
    {
        SysInt numvars = vars.size(); 
        
        // literalsScratch.reserve(numvars);

        literalsScratch.resize(0);

        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(numvars, 0);
        
        firstLiteralPerVar.resize(numvars); 

    {
        DomainInt litCounter = 0 ; 
        numvals = 0 ;           // only used now by tuple list stuff

        for(SysInt i=0; i<numvars; i++) {

            firstLiteralPerVar[i] = checked_cast<SysInt>(litCounter); 
            DomainInt thisvalmin = vars[i].getInitialMin();
            DomainInt numvals_i = vars[i].getInitialMax()-thisvalmin+1;
            if(numvals_i > numvals) numvals = checked_cast<SysInt>(numvals_i);
            litCounter += numvals_i; 
        }

        literalList.resize(checked_cast<SysInt>(litCounter)); 
    }

    {
           SysInt litCounter = 0 ; 
        for(SysInt i=0; i<numvars; i++) {
            DomainInt thisvalmin = vars[i].getInitialMin();
            DomainInt numvals_i = vars[i].getInitialMax()-thisvalmin+1;
        for(DomainInt j=0; j<numvals_i; j++) {
                    literalList[litCounter].var = i; 
                    literalList[litCounter].val = j+thisvalmin; 
                    literalList[litCounter].supportCellList = 0;
                    litCounter++;
            }
        }

        numlits = litCounter;
    }

        
        zeroLits.resize(numvars);
        for(SysInt i=0 ; i < numvars ; i++) {
            SysInt numvals_i = checked_cast<SysInt>(vars[i].getInitialMax()- vars[i].getInitialMin()+1); 
            zeroLits[i].reserve(numvals_i);  // reserve the maximum length.
            zeroLits[i].resize(0); 
            SysInt thisvarstart = firstLiteralPerVar[i];
            for(SysInt j=0 ; j < numvals_i; j++) zeroLits[i].push_back(j+thisvarstart);
        }
        inZeroLits.resize(numlits,true); 
        
        // Lists (vectors) of literals/vars that have lost support.
        // Set this up to insist that everything needs to have support found for it on full propagate.
        
        // litsWithLostExplicitSupport.reserve(numlits); // max poss size, not necessarily optimal choice here
        varsWithLostImplicitSupport.reserve(vars.size());

        // Pointers to the last implicit/explicit support for a var/literal
        //
        lastSupportPerVar.resize(numvars,0);    // actually don't think we care what initial val is
        lastSupportPerLit.resize(numlits,0);
    
        deletedSupports.reserve(numlits);   // max poss size, not necessarily best choice

        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(SysInt i=vars.size()-1; i>=0 ; i--) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(numlits+1);
        supportNumPtrs[0]=0;
        for(SysInt i=1; i<= numlits; i++) supportNumPtrs[i]=vars.size();
        
        // Extract short supports from tuples if necessary.
        if(tuples->size()>1) {
            cout << "Tuple list passed to supportgac constraint should only contain one tuple, encoding a list of short supports." << endl; 
            abort();
        }
        
#if UseList
        // Read in the short supports.
        D_ASSERT(tuples->size()==1);
        vector<DomainInt> encoded = tuples->get_vector(0);
        
        #if UseList && SupportsGacNoCopyList
        vector<vector<pair<SysInt, DomainInt> > * > shortsupports;
        #else
        vector<vector<pair<SysInt, DomainInt> > > shortsupports;
        #endif
        
        vector<pair<SysInt, DomainInt> > temp;
        for(SysInt i=0; i<encoded.size(); i=i+2) {
            if(encoded[i]==-1) {
                // end of a short support.
                if(encoded[i+1]!=-1) {
                    cout << "Split marker is -1,-1 in tuple for supportsgac." << endl;
                    abort();
                }
                #if UseList && SupportsGacNoCopyList
                shortsupports.push_back(new vector<pair<SysInt, DomainInt> >(temp));
                #else
                shortsupports.push_back(temp);
                #endif
                temp.clear();
            }
            else
            {
                if(encoded[i]<0 || encoded[i]>=vars.size()) {
                    cout << "Tuple passed into supportsgac does not correctly encode a set of short supports." << endl;
                    abort();
                }
                temp.push_back(make_pair(checked_cast<SysInt>(encoded[i]), encoded[i+1])); 
            }
        }
        if(encoded[encoded.size()-2]!=-1 || encoded[encoded.size()-1]!=-1) {
            cout << "Last -1,-1 marker missing from tuple in supportsgac."<< endl;
            abort();
        }
        
        // Sort it. Might not work when it's pointers.
        for(SysInt i=0; i<shortsupports.size(); i++) {
            // Sort each short support
            #if UseList && SupportsGacNoCopyList
            sort(shortsupports[i]->begin(), shortsupports[i]->end());
            #else
            sort(shortsupports[i].begin(), shortsupports[i].end());
            #endif
        }
        sort(shortsupports.begin(), shortsupports.end(), SupportDeref());
        
        tuple_lists.resize(vars.size());
        tuple_list_pos.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            const SysInt domsize = checked_cast<SysInt>(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            tuple_lists[var].resize(domsize);
            tuple_list_pos[var].resize(domsize, 0);
            
            for(DomainInt val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                // get short supports relevant to var,val.
                for(SysInt i=0; i<shortsupports.size(); i++) {
                    bool varin=false;
                    bool valmatches=true;
                    
                    #if SupportsGacNoCopyList
                    vector<pair<SysInt, DomainInt> > & shortsup=*(shortsupports[i]);
                    #else
                    vector<pair<SysInt, DomainInt> > & shortsup=shortsupports[i];
                    #endif
                    
                    for(SysInt j=0; j<shortsup.size(); j++) {
                        if(shortsup[j].first==var) {
                            varin=true;
                            if(shortsup[j].second!=val) {
                                valmatches=false;
                            }
                        }
                    }
                    
                    if(!varin || valmatches) {
                        // If the support doesn't include the var, or it 
                        // does include var,val then add it to the list.
                        tuple_lists[var][checked_cast<SysInt>(val-vars[var].getInitialMin())].push_back(shortsupports[i]);
                    }
                }
            }
        }
#endif
        
#if UseNDOneList
        D_ASSERT(tuples->size()==1);
        vector<DomainInt> encoded = tuples->get_vector(0);
        
        vector<tuple<SysInt, SysInt,SysInt> > temp;
        for(SysInt i=0; i<encoded.size(); i=i+2) {
            if(encoded[i]==-1) {
                // end of a short support.
                if(encoded[i+1]!=-1) {
                    cout << "Split marker is -1,-1 in tuple for supportsgac." << endl;
                    abort();
                }
                tuple_nd_list.push_back(temp);
                temp.clear();
            }
            else
            {
                if(encoded[i]<0 || encoded[i]>=vars.size()) {
                    cout << "Tuple passed into supportsgac does not correctly encode a set of short supports." << endl;
                    abort();
                }
                temp.push_back(make_tuple(encoded[i], encoded[i+1], 0)); 
            }
        }
        if(encoded[encoded.size()-2]!=-1 || encoded[encoded.size()-1]!=-1) {
            cout << "Last -1,-1 marker missing from tuple in supportsgac."<< endl;
            abort();
        }
        
        // Sort it. 
        for(SysInt i=0; i<tuple_nd_list.size(); i++) {
            // Sort each short support
            sort(tuple_nd_list[i].begin(), tuple_nd_list[i].end());
        }
        sort(tuple_nd_list.begin(), tuple_nd_list.end());
        
        setup_tuple_list();
        
        tuple_list_pos.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            tuple_list_pos[var].resize(vars[var].getInitialMax()-vars[var].getInitialMin()+1, 0);
        }
        
#endif
    }
    
    // A Second constructor for supportsgaclist constraint, that takes a list of
    // full-length tuples and should be identical in behaviour to the table constraint.
    
    HaggisGACStable(StateObj* _stateObj, const VarArray& _var_array, TupleList* tuples, SysInt) : AbstractConstraint(_stateObj), 
    vars(_var_array), supportFreeList(0)
    {
        SysInt numvars = vars.size(); 
        
        // literalsScratch.reserve(numvars);

        literalsScratch.resize(0);

        // Register this with the backtracker.
        getState(stateObj).getGenericBacktracker().add(this);
        
        // Initialise counters
        supports=0;
        supportsPerVar.resize(numvars, 0);
        
        firstLiteralPerVar.resize(numvars); 

        SysInt litCounter = 0 ; 
        numvals = 0 ;           // only used now by tuple list stuff

        for(SysInt i=0; i<numvars; i++) {

            firstLiteralPerVar[i] = litCounter; 
            SysInt thisvalmin = vars[i].getInitialMin();
            SysInt numvals_i = vars[i].getInitialMax()-thisvalmin+1;
            if(numvals_i > numvals) numvals = numvals_i;
            litCounter += numvals_i; 
        }

        literalList.resize(litCounter); 

        litCounter = 0 ; 
        for(SysInt i=0; i<numvars; i++) {
            SysInt thisvalmin = vars[i].getInitialMin();
            SysInt numvals_i = vars[i].getInitialMax()-thisvalmin+1;
            for(SysInt j=0; j<numvals_i; j++) {
                    literalList[litCounter].var = i; 
                    literalList[litCounter].val = j+thisvalmin; 
                    literalList[litCounter].supportCellList = 0;
                    litCounter++;
            }
        }

        numlits = litCounter;
        
        zeroLits.resize(numvars);
        for(SysInt i=0 ; i < numvars ; i++) {
            SysInt numvals_i = vars[i].getInitialMax()- vars[i].getInitialMin()+1; 
            zeroLits[i].reserve(numvals_i);  // reserve the maximum length.
            zeroLits[i].resize(0); 
            SysInt thisvarstart = firstLiteralPerVar[i];
            for(SysInt j=0 ; j < numvals_i; j++) zeroLits[i].push_back(j+thisvarstart);
        }
        inZeroLits.resize(numlits,true); 
        
        // Lists (vectors) of literals/vars that have lost support.
        // Set this up to insist that everything needs to have support found for it on full propagate.
        
        // litsWithLostExplicitSupport.reserve(numlits); // max poss size, not necessarily optimal choice here
        varsWithLostImplicitSupport.reserve(vars.size());

        // Pointers to the last implicit/explicit support for a var/literal
        //
        lastSupportPerVar.resize(numvars,0);    // actually don't think we care what initial val is
        lastSupportPerLit.resize(numlits,0);
    
        deletedSupports.reserve(numlits);   // max poss size, not necessarily best choice

        // Partition
        varsPerSupport.resize(vars.size());
        varsPerSupInv.resize(vars.size());
        for(SysInt i=vars.size()-1; i>=0 ; i--) {
            varsPerSupport[i]=i;
            varsPerSupInv[i]=i;
        }
        
        // Start with 1 cell in partition, for 0 supports. 
        supportNumPtrs.resize(numlits+1);
        supportNumPtrs[0]=0;
        for(SysInt i=1; i<= numlits; i++) supportNumPtrs[i]=vars.size();
        
        CHECK( (UseList || UseNDOneList), "Attempt to use supportsgaclist with wrong version of supportsgac");
        
#if UseList
        // Read in the full-length supports.
        vector<DomainInt> encoded = tuples->get_vector(0);
        
        #if UseList && SupportsGacNoCopyList
        vector<vector<pair<SysInt, SysInt> > * > shortsupports;
        #else
        vector<vector<pair<SysInt, SysInt> > > shortsupports;
        #endif
        
        vector<pair<SysInt, SysInt> > temp;
        for(SysInt i=0; i<tuples->size(); i++) {
            const SysInt* tup =tuples->get_tupleptr(i);
            
            for(SysInt j=0; j<vars.size(); j++) {
                temp.push_back(make_pair(j,tup[j]));
            }
            
            #if UseList && SupportsGacNoCopyList
            shortsupports.push_back(new vector<pair<SysInt, SysInt> >(temp));
            #else
            shortsupports.push_back(temp);
            #endif
            
            temp.clear();
        }
        // Same as other ctor from here.
        // Sort it. Might not work when it's pointers.
        for(SysInt i=0; i<shortsupports.size(); i++) {
            // Sort each short support
            #if UseList && SupportsGacNoCopyList
            sort(shortsupports[i]->begin(), shortsupports[i]->end());
            #else
            sort(shortsupports[i].begin(), shortsupports[i].end());
            #endif
        }
        sort(shortsupports.begin(), shortsupports.end(), SupportDeref());
        
        tuple_lists.resize(vars.size());
        tuple_list_pos.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            tuple_lists[var].resize(numvals);
            tuple_list_pos[var].resize(numvals, 0);
            
            for(SysInt val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                // get short supports relevant to var,val.
                for(SysInt i=0; i<shortsupports.size(); i++) {
                    bool varin=false;
                    bool valmatches=true;
                    
                    #if SupportsGacNoCopyList
                    vector<pair<SysInt,SysInt> > & shortsup=*(shortsupports[i]);
                    #else
                    vector<pair<SysInt,SysInt> > & shortsup=shortsupports[i];
                    #endif
                    
                    for(SysInt j=0; j<shortsup.size(); j++) {
                        if(shortsup[j].first==var) {
                            varin=true;
                            if(shortsup[j].second!=val) {
                                valmatches=false;
                            }
                        }
                    }
                    
                    if(!varin || valmatches) {
                        // If the support doesn't include the var, or it 
                        // does include var,val then add it to the list.
                        tuple_lists[var][val-vars[var].getInitialMin()].push_back(shortsupports[i]);
                    }
                }
            }
        }
#endif
        
#if UseNDOneList
        
        vector<tuple<SysInt, SysInt,SysInt> > temp;
        for(SysInt i=0; i<tuples->size(); i++) {
            const SysInt* tup =tuples->get_tupleptr(i);
            
            for(SysInt j=0; j<vars.size(); j++) {
                temp.push_back(make_tuple(j,tup[j], 0));
            }
            
            tuple_nd_list.push_back(temp);
            temp.clear();
        }
        
        // Sort it. 
        for(SysInt i=0; i<tuple_nd_list.size(); i++) {
            // Sort each short support
            sort(tuple_nd_list[i].begin(), tuple_nd_list[i].end());
        }
        sort(tuple_nd_list.begin(), tuple_nd_list.end());
        
        setup_tuple_list();
        
        tuple_list_pos.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            tuple_list_pos[var].resize(numvals, 0);
        }
        
#endif
    }
    
    
    
    ////////////////////////////////////////////////////////////////////////////
    // Dtor
    
    virtual ~HaggisGACStable() {
        //printStructures();
        set<Support*> myset;

        /* 
        for(SysInt i=0; i<vars.size(); i++) {
            cout << "     i " << i << " Initial Max " << vars[i].getInitialMax() << endl ; 
            SysInt numvals_i = vars[i].getInitialMax()-vars[i].getInitialMin()+1;
            for(SysInt j=0; j<numvals_i; j++) {
              cout << "     i j SupportListPerLit[var][val].next = " << i << " " << j << " " << supportListPerLit[i][j].next << endl ; 
            }
        }
        */
        

        // Want to find all active support objects so we can delete them 
        for(SysInt lit=0; lit<numlits; lit++) {
               SupportCell* supCell = literalList[lit].supportCellList; 

              // cout << "     destructor 2: sup*= " << sup << endl ; 
                while(supCell!=0) {
                    myset.insert(supCell->sup); // may get inserted multiple times but it's a set.
                    supCell = supCell->next;
                }
        }
        
        // Go through supportFreeList
        
        while(supportFreeList!=0) {
            Support* sup=supportFreeList;
            supportFreeList=sup->nextFree;
            myset.insert(sup);
        }
        
        // Anything remaining on bracktrack stack
        for(SysInt i=0; i<backtrack_stack.size(); i++) {
            if(backtrack_stack[i].sup!=0) {
                myset.insert(backtrack_stack[i].sup);
            }
        }
        
        typename set<Support*>::iterator it;
        for ( it=myset.begin() ; it != myset.end(); it++ ) {
            delete *it;
        }
    }

    
    ////////////////////////////////////////////////////////////////////////////
    // Backtracking mechanism
    
    struct BTRecord {
        // bool is_removal;   // removal or addition was made. 
        SysInt var;  
        SysInt lit;
        Support* sup;
        
        friend std::ostream& operator<<(std::ostream& o, const BTRecord& rec)
        {
            if(rec.sup==0) return o<<"ZeroMarker";
            o<<"BTRecord:" ; 
            o<<rec.var<<","<<rec.lit;
            // o<< rec.sup->literals;
            return o;
        }
    };


    // Have support stack 
    // shove deletions onto them. 
    // Shove deleted literals onto bt record
    // go through support stack and destroy any zero supports
    //
    // on backtrack destroy support if it gets to zero and inactive
    
    vector<BTRecord> backtrack_stack;
    
    void mark() {
        struct BTRecord temp = { 0, 0, 0 };
        backtrack_stack.push_back(temp);  // marker.
    }
    
    void pop() {
        //cout << "BACKTRACKING:" << endl;
        //cout << backtrack_stack <<endl;
        while(backtrack_stack.back().sup != 0) {
            BTRecord temp=backtrack_stack.back();
            backtrack_stack.pop_back();
            // BTStable Change
            if (! (temp.sup->active)) {
                 if (hasNoKnownSupport(temp.var,temp.lit)) {
                         // we need to add support back in
                         addSupportInternal(temp.sup); 
                 }
                 else {
                         // could be clever with -- here but let's play safe
                         if(temp.sup->numLastSupported == 1){ 
                                 // we can add tempsup to supportFreeList
                                 // cout << "adding support to Free List " << temp.sup->literals << endl ; 
                                 addToSupportFreeList(temp.sup);
                         }
                         else { 
                                 temp.sup->numLastSupported--;
                         }
                 }
            }
            // Might possibly have to add lit back onto zeroLits 
            //
            // Hard to be certain zeroLits is correct so play safe and test it
            if(!inZeroLits[temp.lit] && literalList[temp.lit].supportCellList == 0){
                    inZeroLits[temp.lit]=true; 
                    zeroLits[temp.var].push_back(temp.lit); 
            }

        }
        
        backtrack_stack.pop_back();  // Pop the marker.
        //cout << "END OF BACKTRACKING." << endl;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Add and delete support
    
    // don't need argument?   Just use litlist member?  
    //
    //Support* addSupport(box<pair<SysInt, DomainInt> >* litlist)
    void addSupport()
    {
       Support* newsup = getFreeSupport(); 
       vector<SupportCell>& supCells=newsup->supportCells;
       SysInt oldsize = supCells.size() ;
       SysInt newsize = literalsScratch.size() ;

       newsup->arity = newsize;

       if(newsize > oldsize) { 
               supCells.resize(newsize) ; 
               // make sure pointers to support cell are correct
               // need only be done once as will always point to
               // its own support
               for(SysInt i=oldsize; i < newsize ; i++) { 
                       supCells[i].sup = newsup; 
               }
       }

       for(SysInt i=0; i<newsize ; i++) {
            SysInt var=literalsScratch[i].first;
            DomainInt valoriginal=literalsScratch[i].second;
            const SysInt lit=checked_cast<SysInt>(firstLiteralPerVar[var]+valoriginal-vars[var].getInitialMin());
            supCells[i].literal = lit;
       }
        // now have enough supCells, and sup and literal of each is correct

        addSupportInternal(newsup);
    }

    // these guys can be void 
    //
    //
    
    // Takes a support which has: 
    //          arity correct
    //          supCells containing at least arity elements
    //          each supCells[i[ in range has 
    //                literal correct
    //                sup correct

    void addSupportInternal(Support* sup_internal)
    {
        // add a new support given literals but not pointers in place


        //cout << "Adding support (internal) :" << litlist_internal << endl;
        //D_ASSERT(litlist_internal.size()>0);  
        //// It should be possible to deal with empty supports, but currently they wil
        // cause a memory leak. 
        
        vector<SupportCell>& supCells=sup_internal->supportCells;

        SysInt litsize = sup_internal->arity;
        
        sup_internal->active = true;   
                
        if(litsize < vars.size() ) {    
                // it's a short support, so update supportsPerVar and supports 
          for(SysInt i=0; i<litsize; i++) {

            SysInt lit=supCells[i].literal;
            SysInt var=literalList[lit].var;
            
            // Stitch it into the start of literalList.supportCellList
            
            supCells[i].prev = 0;
            supCells[i].next = literalList[lit].supportCellList;  
            if(literalList[lit].supportCellList!=0) {
                literalList[lit].supportCellList->prev = &(supCells[i]);
            }
            else { 
            // Attach trigger if this is the first support containing var,val.
                attach_trigger(var, literalList[lit].val, lit);
            }
            literalList[lit].supportCellList = &(supCells[i]);

            //update counters
            supportsPerVar[var]++;
            // Update partition
            // swap var to the end of its cell.
            partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]]-1]);
            // Move the boundary so var is now in the higher cell.
            supportNumPtrs[supportsPerVar[var]]--; 
          }
          supports++;
        }
        else {
                // it's a full length support so don't update those counters
          for(SysInt i=0; i<litsize; i++) {

            SysInt lit=supCells[i].literal;
            SysInt var=literalList[lit].var;
            
            // Stitch it into the start of literalList.supportCellList
            
            supCells[i].prev = 0;
            supCells[i].next = literalList[lit].supportCellList;  
            if(literalList[lit].supportCellList!=0) {
                literalList[lit].supportCellList->prev = &(supCells[i]);
            }
            else { 
            // Attach trigger if this is the first support containing var,val.
                attach_trigger(var, literalList[lit].val, lit);
            }
            literalList[lit].supportCellList = &(supCells[i]);
          }
        }


        
        //printStructures();
        
        // return sup_internal;
    }
    
    void deleteSupport(Support* sup) {
        deleteSupportInternal(sup, false);
    }
    
    void deleteSupportInternal(Support* sup, bool Backtracking) {
        D_ASSERT(sup!=0);
                
        sup->active = false;            
        sup->numLastSupported = 0; 
        
        vector<SupportCell>& supCells=sup->supportCells;
        SysInt supArity = sup->arity; 

        if(supArity < vars.size() ) { 
                // it's a short support 

                SysInt oldIndex  = supportNumPtrs[supports];
                
                for(SysInt i=0; i<supArity; i++) {

                    SupportCell& supCell = supCells[i];
                    SysInt lit=supCell.literal;
                    SysInt var=literalList[lit].var ;

                    // D_ASSERT(prev[var]!=0);
                    // decrement counters
                    supportsPerVar[var]--;

                    if(supCell.prev==0) {       // this was the first support in list

                            literalList[lit].supportCellList = supCell.next; 

                            if(supCell.next==0) { 
                                    // We have lost the last support for lit
                                    //
                    // I believe that each literal can only be marked once here in a call to update_counters.
                    // so we should be able to push it onto a list
                    //
                    // As long as we do not actually call find_new_support.
                    // So probably should shove things onto a list and then call find supports later

                                if (!Backtracking && supportsPerVar[var] == (supports - 1)) {   // since supports not decremented yet
                                        litsWithLostExplicitSupport.push_back(lit);
                                        lastSupportPerLit[lit] = sup;
                                }
                                else 
                                
                                // PREVIOUSLY there was no else here.   We had to add to zerolits even if above true
                                //
                                // But now we don't because if we remove the value then we would remove it from zerolits
                                // and put it back on going back throug hthe backtrack stack.
                                //
                                // Only case where we need to add it is if we DO find a new support which is implicit
                                // And that case is covered elsewhere - search for NOTEAAA.   
                                //
                                // However if test above is false then we have to check for zeroLits 
                                //

                                if(!inZeroLits[lit]) {
                                    inZeroLits[lit]=true;
                                    zeroLits[var].push_back(lit);
                                }
                            // Remove trigger since this is the last support containing var,val.
                               if(SupportsGACUseDT) { detach_trigger(lit); }
                            }
                            else { 
                                    supCell.next->prev=0;
                            }
                    }
                    else {
                            supCell.prev->next = supCell.next;
                            if(supCell.next!=0){
                                    supCell.next->prev = supCell.prev;
                            }
                    }
                    
                    

                    // Update partition
                    // swap var to the start of its cell.  
                    // This plays a crucial role in moving together the vars which previously
                    // had 1 less than numsupports and soon will have numsupports.

                    partition_swap(var, varsPerSupport[supportNumPtrs[supportsPerVar[var]+1]]);
                    //
                    // Move the boundary so var is now in the lower cell.
                    supportNumPtrs[supportsPerVar[var]+1]++;

                    
                }
                supports--;
                
                // For following code it is essential that partition swaps compress 
                // vars together which used to have SupportsPerVar[i] = supports-1 and 
                // now have supportsPerVar[i] = supports (because supports has been decremented
                // 
                //
                    //
                    // Similarly to the above, each var can only be added to this list once per call to update_counters
                    // Because it can only lose its last implicit support once since we are only deleting supports.
                    //
                
                // I hope we only need to do this when NOT backtracking, at least for non backtrack-stable version
                // When we backtrack we will add supports which did support it so there is no need to find new supports

        //      cout << supportNumPtrs[supports] << " " << oldIndex << endl;
                
                if (!Backtracking) {
                        for(SysInt i=supportNumPtrs[supports]; i < oldIndex; i++) { 
                                varsWithLostImplicitSupport.push_back(varsPerSupport[i]);
                                lastSupportPerVar[varsPerSupport[i]] = sup ;
                        }
                        deletedSupports.push_back(sup);
                } 
                else { 
                    // We are Backtracking 
                    // Can re-use the support when it is removed by BT. 
                    // Stick it on the free list 
                    addToSupportFreeList(sup); 
                }
            }
        else {
                // it's a full length support 

                for(SysInt i=0; i<supArity; i++) {

                    SupportCell& supCell = supCells[i];
                    SysInt lit=supCell.literal;
                    SysInt var=literalList[lit].var ;

                    if(supCell.prev==0) {       // this was the first support in list

                            literalList[lit].supportCellList = supCell.next; 

                            if(supCell.next==0) { 
                                    // We have lost the last support for lit
                                    //
                    // I believe that each literal can only be marked once here in a call to update_counters.
                    // so we should be able to push it onto a list
                    //
                    // As long as we do not actually call find_new_support.
                    // So probably should shove things onto a list and then call find supports later

                                if (!Backtracking && supportsPerVar[var] == supports) {         // supports won't be decremented
                                        litsWithLostExplicitSupport.push_back(lit);
                                        lastSupportPerLit[lit] = sup;
                                }
                                else 
                                
                                // PREVIOUSLY there was no else here.   We had to add to zerolits even if above true
                                //
                                // But now we don't because if we remove the value then we would remove it from zerolits
                                // and put it back on going back throug hthe backtrack stack.
                                //
                                // Only case where we need to add it is if we DO find a new support which is implicit
                                // And that case is covered elsewhere - search for NOTEAAA.   
                                //
                                // However if test above is false then we have to check for zeroLits 
                                //

                                if(!inZeroLits[lit]) {
                                    inZeroLits[lit]=true;
                                    zeroLits[var].push_back(lit);
                                }
                            // Remove trigger since this is the last support containing var,val.
                               if(SupportsGACUseDT) { detach_trigger(lit); }
                            }
                            else { 
                                    supCell.next->prev=0;
                            }
                    }
                    else {
                            supCell.prev->next = supCell.next;
                            if(supCell.next!=0){
                                    supCell.next->prev = supCell.prev;
                            }
                    }
                    
                }
                
                // Since this was a full length supports no var has lost its last implicit support
                
                if (!Backtracking) {
                        deletedSupports.push_back(sup);
                } 
                else { 
                    // We are Backtracking 
                    // Can re-use the support when it is removed by BT. 
                    // Stick it on the free list 
                    addToSupportFreeList(sup); 
                }
        }
    }


    BOOL hasNoKnownSupport(SysInt var,SysInt lit) {
            //
            // Either implicitly supported or counter is non zero
            // Note that even if we have an explicit support which may be invalid, we can return true
            // i.e. code does not guarantee that it has a valid support, only that it has a support.
            // If we have no valid supports then (if algorithms are right) we will eventually delete
            // the last known valid support and at that time start looking for a new one.

            D_ASSERT(var == literalList[lit].var); 

            return supportsPerVar[var] == supports && (literalList[lit].supportCellList == 0);
    }
    //
    ////////////////////////////////////////////////////////////////////////////
    // 
    void printStructures()
    {
        cout << "PRINTING ALL DATA STRUCTURES" <<endl;
        cout << "supports:" << supports <<endl;
        cout << "supportsPerVar:" << supportsPerVar << endl;
        cout << "partition:" <<endl;
        for(SysInt i=0; i<supportNumPtrs.size()-1; i++) {
            cout << "supports: "<< i<< "  vars: ";
            for(SysInt j=supportNumPtrs[i]; j<supportNumPtrs[i+1]; j++) {
                cout << varsPerSupport[j]<< ", ";
            }
            cout << endl;
            if(supportNumPtrs[i+1]==vars.size()) break;
        }
        cout << "zeroLits:" << zeroLits << endl;
        cout << "inZeroLits:" << inZeroLits << endl;
        /*
        
        cout << "Supports for each literal:"<<endl;
        for(SysInt var=0; var<vars.size(); var++) {
            cout << "Variable: "<<var<<endl;
            for(SysInt val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                cout << "Value: "<<val<<endl;
                Support* sup=supportListPerLit[var][val-vars[var].getInitialMin()].next[var];
                while(sup!=0) {
                    cout << "Support: " << sup->literals << endl;
                    bool contains_varval=false;
                    for(SysInt i=0; i<sup->literals.size(); i++) {
                        if(sup->literals[i].first==var && sup->literals[i].second==val)
                            contains_varval=true;
                    }
                    D_ASSERT(contains_varval);
                    
                    sup=sup->next[var];
                }
            }
        }
        */
    }
    
    #if !SupportsGACUseDT
        virtual triggerCollection setup_internal()
        {
            triggerCollection t;
            SysInt array_size = vars.size();
            for(SysInt i = 0; i < array_size; ++i)
              t.push_back(make_trigger(vars[i], Trigger(this, i), DomainChanged));
            return t;
        }
    #endif
    
    void partition_swap(SysInt xi, SysInt xj)
    {
        if(xi != xj) {
            varsPerSupport[varsPerSupInv[xj]]=xi;
            varsPerSupport[varsPerSupInv[xi]]=xj;
            SysInt temp=varsPerSupInv[xi];
            varsPerSupInv[xi]=varsPerSupInv[xj];
            varsPerSupInv[xj]=temp;
        }
    }

    bool findSupportsIncrementalHelper(SysInt var, DomainInt val) { 

            typedef pair<SysInt,DomainInt> temptype;
            // MAKE_STACK_BOX(newsupportbox, temptype, vars.size()); 
            literalsScratch.clear(); 
            // bool foundsupport=findNewSupport(newsupportbox, var, val);
            bool foundsupport=findNewSupport(var, val);
            
            if(!foundsupport) {
                vars[var].removeFromDomain(val);        
                        // note we are not doing this internally, 
                        // i.e. trying to update counters etc. 
                        // So counters won't have changed until we are retriggered on the removal
            }
            else {
                // addSupport(&newsupportbox);
                addSupport();
            }
            return foundsupport;
    }


    
    void findSupportsInitial()
    {
        // called from Full Propagate
        // We do not assign responsibility for removals as this is called at the root.

        for(SysInt i = varsWithLostImplicitSupport.size()-1; i >= 0; i--) { 

            SysInt var= varsWithLostImplicitSupport[i];
            varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary - will get resized to 0 later

            if (supportsPerVar[var] == supports) {      // otherwise has found implicit support in the meantime
                    for(SysInt j=0; j<zeroLits[var].size(); j++) {
                        SysInt lit=zeroLits[var][j];
                        if(literalList[lit].supportCellList != 0){
                            // No longer a zero val. remove from vector.
                            zeroLits[var][j]=zeroLits[var][zeroLits[var].size()-1];
                            zeroLits[var].pop_back();
                            inZeroLits[lit]=false;
                            j--;
                            continue;
                        }
                        DomainInt val=literalList[lit].val;

                        if(vars[var].inDomain(val)) {   // tested supportCellList  above
                            findSupportsIncrementalHelper(var,val) ;
                        } 
                    }    
                }
        }
    }

    void findSupportsIncremental()
    {
        // For list of vars which have lost their last implicit support
        // do this ... 
        // but don't need to redo if we have stored that list ahead of time 
        //  ... and we can check if it still has support 
        //
        // For each variable where the number of supports is equal to the total...


        for(SysInt i=litsWithLostExplicitSupport.size()-1; i >= 0; i--) { 
            SysInt lit=litsWithLostExplicitSupport[i];
            SysInt var=literalList[lit].var;
            DomainInt val=literalList[lit].val;
            
            litsWithLostExplicitSupport.pop_back(); // actually probably unnecessary - will get resized to 0 later
            
            
            if(vars[var].inDomain(val)) {
                    if (hasNoKnownSupport(var,lit) && ! findSupportsIncrementalHelper(var,val) ) { 
                            // removed val so must annotate why
                            lastSupportPerLit[lit]->numLastSupported++ ;
                            struct BTRecord backtrackInfo = { var, lit, lastSupportPerLit[lit] };
                            backtrack_stack.push_back(backtrackInfo);
                    }
                    else {  // We now have an alternative support for this literal 
                            // so we need to do nothing EXCEPT ... 
                            //  ... add it to zeroLits if necessary
                            //
                            //  Note this is the only case where we need to add the lit to zeroLits from NOTEAAA

                            if(!inZeroLits[lit] && literalList[lit].supportCellList == 0){
                                    inZeroLits[lit]=true;
                                    zeroLits[var].push_back(lit); 
                            }
                    }
            }
            else {  // Need to cover out of domain lits but has known support so it will have support on BT.
                  lastSupportPerLit[lit]->numLastSupported++ ;
                  struct BTRecord backtrackInfo = { var, lit, lastSupportPerLit[lit] };
                  backtrack_stack.push_back(backtrackInfo);
            }
        }

        for(SysInt i = varsWithLostImplicitSupport.size()-1; i >= 0; i--) { 

            SysInt var= varsWithLostImplicitSupport[i];
            varsWithLostImplicitSupport.pop_back(); // actually probably unnecessary - will get resized to 0 later

            if (supportsPerVar[var] == supports) {      // otherwise has found implicit support in the meantime
                    for(SysInt j=0; j<zeroLits[var].size() && supportsPerVar[var] == supports; j++) {
                        SysInt lit=zeroLits[var][j];
                        if(literalList[lit].supportCellList != 0){
                            // No longer a zero val. remove from vector.
                            zeroLits[var][j]=zeroLits[var][zeroLits[var].size()-1];
                            zeroLits[var].pop_back();
                            inZeroLits[lit]=false;
                            j--;
                            continue;
                        }
                        DomainInt val=literalList[lit].val;

                        if(vars[var].inDomain(val)) {   // tested literalList  above
                            if (! findSupportsIncrementalHelper(var,val) ) {
                                    lastSupportPerVar[var]->numLastSupported++;
                                    struct BTRecord backtrackInfo = { var, lit, lastSupportPerVar[var] };
                                    backtrack_stack.push_back(backtrackInfo);
                                    // No longer in domain.  remove from zeroLits vector.
                                    zeroLits[var][j]=zeroLits[var][zeroLits[var].size()-1];
                                    zeroLits[var].pop_back();
                                    inZeroLits[lit]=false;
                                    j--;
                                    continue;
                            }
                            // Else we have found new implicit or explicit support is found 
                            //
                            // No longer do we remove j from zerovals in this case if explicit support is found.
                            // However this is correct as it can be removed lazily next time the list is traversed
                            // And we might even get lucky and save this small amount of work.
                        } 
                        else {
                            // This is a nasty case because we are doing backtrack stability
                            // var=val has been removed, and it must have been by something outside this 
                            // constraint.  Therefore when it is restored we need to make sure it has 
                            // support in this constraint.   Since it has no explicit support, its last 
                            // support must be this implicit support we are deleting.  So we have to restore
                            // it on bracktracking.
                            lastSupportPerVar[var]->numLastSupported++;
                            struct BTRecord backtrackInfo = { var, lit, lastSupportPerVar[var]};
                            backtrack_stack.push_back(backtrackInfo);
                            // No longer in domain. remove from vector.
                            zeroLits[var][j]=zeroLits[var][zeroLits[var].size()-1];
                            zeroLits[var].pop_back();
                            inZeroLits[lit]=false;
                            j--;
                            continue;
                        }
                    }    
                }
        }
    }

    inline void updateCounters(SysInt lit) {

        SupportCell* supCellList = literalList[lit].supportCellList ;

        litsWithLostExplicitSupport.resize(0);   
        varsWithLostImplicitSupport.resize(0);

        while(supCellList != 0) {
            SupportCell* next=supCellList->next;
            deleteSupport(supCellList->sup);
            supCellList=next;
        }
    }
    
    
    #if SupportsGACUseDT
        SysInt dynamic_trigger_count() { 
            return literalList.size();
        }
    #endif
    
  inline void attach_trigger(SysInt var, DomainInt val, SysInt lit)
  {
      //P("Attach Trigger: " << i);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      // find the trigger for var, val.
      dt=dt+lit;
      D_ASSERT(!dt->isAttached());
      
      vars[var].addDynamicTrigger(dt, DomainRemoval, val );   //BT_CALL_BACKTRACK
  }
  
  inline void detach_trigger(SysInt lit)
  {
      //P("Detach Triggers");
      
      // D_ASSERT(supportListPerLit[var][val-vars[var].getInitialMin()].next[var] == 0);
      
      DynamicTrigger* dt = dynamic_trigger_start();
      dt=dt+lit;
      releaseTrigger(stateObj, dt);   // BT_CALL_BACKTRACK
  }
    
  virtual void propagate(SysInt prop_var, DomainDelta)
  {
  /* 
   Probably won't work
   */
    cout << "Have given up trying to make this work without dynamic triggers" << endl ;
    /*
     *
    D_ASSERT(prop_var>=0 && prop_var<vars.size());
    // Really needs triggers on each value, or on the supports. 
    
    //printStructures();
    D_ASSERT(!SupportsGACUseDT);  // Should not be here if using dynamic triggers.
    
    for(SysInt val=vars[prop_var].getInitialMin(); val<=vars[prop_var].getInitialMax(); val++) {
        if(!vars[prop_var].inDomain(val) && supportListPerLit[prop_var][val-vars[prop_var].getInitialMin()].next[prop_var]!=0) {
            updateCounters(prop_var, val);
        }
    }
    
    findSupportsIncremental();
    */
  }
  
    virtual void propagate(DynamicTrigger* dt)
  {
      SysInt lit=dt-dynamic_trigger_start();

    //  cout << "Propagate called: var= " << var << "val = " << val << endl;
      //printStructures();

      deletedSupports.resize(0);
      
      updateCounters(lit);

      findSupportsIncremental();

      while(deletedSupports.size() > 0) { 
              if (deletedSupports.back()->numLastSupported == 0) {
                      addToSupportFreeList(deletedSupports.back());
              }
              deletedSupports.pop_back();
      }
  }

    // for backtrack stability we can't delete assigned variables, i.e. added even if isAssigned. 
    
    #define ADDTOASSIGNMENT(var, val) literalsScratch.push_back(make_pair(var,val));
    
    // For full-length support variant:
    #define ADDTOASSIGNMENTFL(var, val) literalsScratch.push_back(make_pair(var,val));
    
    
    // Macro to add either the lower bound or the specified value for a particular variable vartopad
    // Intended to pad out an assignment to a full-length support.
    #define PADOUT(vartopad) if(var==vartopad) literalsScratch.push_back(make_pair(var, val)); else literalsScratch.push_back(make_pair(vartopad, vars[vartopad].getMin()));
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for pair-equals. a=b or c=d.
    
    /*
    bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
        // a=b or c=d
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()==4);
        SysInt othervar;
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
            for(SysInt otherval=vars[2].getMin(); otherval<=vars[2].getMax(); otherval++) {
                if(vars[2].inDomain(otherval) && vars[3].inDomain(otherval)) {
                    assignment.push_back(make_pair(2, otherval));
                    assignment.push_back(make_pair(3, otherval));
                    return true;
                }
            }
        }
        else {
            for(SysInt otherval=vars[0].getMin(); otherval<=vars[0].getMax(); otherval++) {
                if(vars[0].inDomain(otherval) && vars[1].inDomain(otherval)) {
                    assignment.push_back(make_pair(0, otherval));
                    assignment.push_back(make_pair(1, otherval));
                    return true;
                }
            }
        }
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
      D_ASSERT(array_size == 4);
      
      if(v[0]==v[1] || v[2]==v[3]) return true;
      return false;
      
    }*/
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for element

#if UseElementShort
    
    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        typedef typename VarArray::value_type VarRef;
        VarRef idxvar=vars[vars.size()-2];
        VarRef resultvar=vars[vars.size()-1];
        D_ASSERT(vars[var].inDomain(val));
        
        if(var<vars.size()-2) {
            // var is in the vector.
            
            for(SysInt i=idxvar.getMin(); i<=idxvar.getMax(); i++) {
                if(idxvar.inDomain(i) && i>=0 && i<vars.size()-2) {
                    for(SysInt j=resultvar.getMin(); j<=resultvar.getMax(); j++) {
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
            
            for(SysInt i=resultvar.getMin(); i<=resultvar.getMax(); i++) {
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
            for(SysInt i=0; i<vars.size()-2; i++) {
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
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        SysInt idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }

#endif
    //
    ////////////////////////////////////////////////////////////////////////////
    // ELEMENT - FULL LENGTH TUPLES VERSION.

#if UseElementLong

    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        typedef typename VarArray::value_type VarRef;
        VarRef idxvar=vars[vars.size()-2];
        VarRef resultvar=vars[vars.size()-1];
        D_ASSERT(vars[var].inDomain(val));
        
        if(var<vars.size()-2) {
            // var is in the vector.
            
            for(SysInt i=idxvar.getMin(); i<=idxvar.getMax(); i++) {
                if(idxvar.inDomain(i) && i>=0 && i<vars.size()-2) {
                    for(SysInt j=resultvar.getMin(); j<=resultvar.getMax(); j++) {
                        if(resultvar.inDomain(j) && vars[i].inDomain(j) &&
                            (i!=var || j==val) ) {   // Either the support includes both var, val or neither -- if neither, it will be a support for var,val.
                            literalsScratch.push_back(make_pair(i, j));
                            literalsScratch.push_back(make_pair(vars.size()-2, i));
                            literalsScratch.push_back(make_pair(vars.size()-1, j));
                            for(SysInt k=0; k<vars.size()-2; k++) {
                                if(k!=i) {
                                    if(k==var)
                                        literalsScratch.push_back(make_pair(k, val));
                                    else
                                        literalsScratch.push_back(make_pair(k, vars[k].getMin()));
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
            
            for(SysInt i=resultvar.getMin(); i<=resultvar.getMax(); i++) {
                if(resultvar.inDomain(i) && vars[val].inDomain(i)) {
                    literalsScratch.push_back(make_pair(vars.size()-2, val));
                    literalsScratch.push_back(make_pair(vars.size()-1, i));
                    literalsScratch.push_back(make_pair(val, i));
                    for(SysInt k=0; k<vars.size()-2; k++) {
                        if(k!=val) literalsScratch.push_back(make_pair(k, vars[k].getMin()));
                    }
                    return true;
                }
            }
            
        }
        else if(var==vars.size()-1) {
            // The result variable.
            for(SysInt i=0; i<vars.size()-2; i++) {
                if(vars[i].inDomain(val) && idxvar.inDomain(i)) {
                    literalsScratch.push_back(make_pair(vars.size()-2, i));
                    literalsScratch.push_back(make_pair(vars.size()-1, val));
                    literalsScratch.push_back(make_pair(i, val));
                    for(SysInt k=0; k<vars.size()-2; k++) {
                        if(k!=i) literalsScratch.push_back(make_pair(k, vars[k].getMin()));
                    }
                    return true;
                }
            }
        }
        return false;
        
        
    }
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        SysInt idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }


#endif 
    
    ////////////////////////////////////////////////////////////////////////////
    // Methods for lexleq
    
#if UseLexLeqShort
    
    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()%2==0);
        // First part of vars is vector 1.
        SysInt vecsize=vars.size()/2;
        
        for(SysInt i=0; i<vecsize; i++) {
            SysInt j=i+vecsize;
            SysInt jmax=vars[j].getMax();
            SysInt imin=vars[i].getMin();
            
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
                //SysInt mid=imin + (jmax-imin)/2;
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
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        D_ASSERT(array_size%2==0);
        for(SysInt i=0; i<array_size/2; i++)
        {
            if(v[i]<v[i+array_size/2]) return true;
            if(v[i]>v[i+array_size/2]) return false;
        }
        return true;
    }


#endif 
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Lexleq with full-length supports
    //

#if UseLexLeqLong
    
    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        D_ASSERT(vars[var].inDomain(val));
        D_ASSERT(vars.size()%2==0);
        // First part of vars is vector 1.
        SysInt vecsize=vars.size()/2;
        
        for(SysInt i=0; i<vecsize; i++) {
            SysInt j=i+vecsize;
            SysInt jmax=vars[j].getMax();
            SysInt imin=vars[i].getMin();
            
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
                        for(SysInt k=i+1; k<vecsize; k++) {
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
                        for(SysInt k=i+1; k<vecsize; k++) {
                            PADOUT(k);
                            PADOUT(k+vecsize);
                        }
                        
                        return true;
                    }
                }
                
                ADDTOASSIGNMENTFL(i,imin);
                ADDTOASSIGNMENTFL(j,jmax);
                for(SysInt k=i+1; k<vecsize; k++) {
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
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        D_ASSERT(array_size%2==0);
        for(SysInt i=0; i<array_size/2; i++)
        {
            if(v[i]<v[i+array_size/2]) return true;
            if(v[i]>v[i+array_size/2]) return false;
        }
        return true;
    }
    
#endif
    
#if UseList && ! SupportsGacNoCopyList

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Table of short supports passed in.
    
    //bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, DomainInt val) {
        D_ASSERT(tuple_lists.size()==vars.size());
        
        const vector<vector<pair<SysInt, DomainInt> > >& tuplist=tuple_lists[var][val-vars[var].getInitialMin()]; 
        
        SysInt listsize=tuplist.size();
        for(SysInt i=tuple_list_pos[var][checked_cast<SysInt>(val-vars[var].getInitialMin())]; i<listsize; i++) {
            
            SysInt supsize=tuplist[i].size();
            bool valid=true;
            
            for(SysInt j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(SysInt j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][checked_cast<SysInt>(val-vars[var].getInitialMin())]=i;
                return true;
            }
        }
        
        
        for(SysInt i=0; i<tuple_list_pos[var][val-vars[var].getInitialMin()]; i++) {
            
            SysInt supsize=tuplist[i].size();
            bool valid=true;
            
            for(SysInt j=0; j<supsize; j++) {
                if(! vars[tuplist[i][j].first].inDomain(tuplist[i][j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(SysInt j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tuplist[i][j].first, tuplist[i][j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val-vars[var].getInitialMin()]=i;
                return true;
            }
        }
        return false;
    }
    
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        // argh, how to do this.
        // test with element first
        
        SysInt idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }

#endif

#if UseList && SupportsGacNoCopyList

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Table of short supports passed in.
    
    //bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, DomainInt val) {
        D_ASSERT(tuple_lists.size()==vars.size());
        const SysInt val_offset = checked_cast<SysInt>(val-vars[var].getInitialMin());
        const vector<vector<pair<SysInt, DomainInt> > * >& tuplist=tuple_lists[var][val_offset]; 
        
        SysInt listsize=tuplist.size();
        for(SysInt i=tuple_list_pos[var][val_offset]; i<listsize; i++) {
            vector<pair<SysInt,DomainInt> > & tup=*(tuplist[i]);
            
            SysInt supsize=tup.size();
            bool valid=true;
            
            for(SysInt j=0; j<supsize; j++) {
                if(! vars[tup[j].first].inDomain(tup[j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(SysInt j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tup[j].first, tup[j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val_offset]=i;
                return true;
            }
        }
        
        
        for(SysInt i=0; i<tuple_list_pos[var][val_offset]; i++) {
            vector<pair<SysInt,DomainInt> > & tup=*(tuplist[i]);
            
            SysInt supsize=tup.size();
            bool valid=true;
            
            for(SysInt j=0; j<supsize; j++) {
                if(! vars[tup[j].first].inDomain(tup[j].second)) {
                    valid=false;
                    break;
                }
            }
            
            if(valid) {
                for(SysInt j=0; j<supsize; j++) {
                    ADDTOASSIGNMENT(tup[j].first, tup[j].second);  //assignment.push_back(tuplist[i][j]);
                }
                tuple_list_pos[var][val_offset]=i;
                return true;
            }
        }
        return false;
    }
    
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        // argh, how to do this.
        // test with element first
        abort();
//        DomainInt idx=v[array_size-2];
//        if(idx<0 || idx>=array_size-2) return false;
//        return v[v[array_size-2]] == v[array_size-1];
    }

#endif

#if UseNDOneList

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Table of short supports passed in. Use ND-onelist from AAAI paper.
    
    // Add the forward pointers to the the tuple_nd_list
    void setup_tuple_list()
    {
        /*tuple_nd_list.reserve(shortsupports.size());
        // Copy them in.
        vector<tuple<SysInt,SysInt,SysInt> > temp;
        for(SysInt i=0; i<shortsupports.size(); i++) {
            temp.clear();
            for(SysInt j=0; j<shortsupports[i].size(); j++) {
                temp.push_back(make_tuple(shortsupports[i][j].first, shortsupports[i][j].second, 0));
            }
            tuple_nd_list.push_back(temp);
        }*/
        
        // Uses fact that no forward pointer can ever be 0.
        
        for(SysInt i=0; i<tuple_nd_list.size(); i++) {
            vector<tuple<SysInt,SysInt,SysInt> > & tup=tuple_nd_list[i];
            
            for(SysInt j=i+1; j<tuple_nd_list.size(); j++) {
                bool all_lits_ptr_set=true;
                
                // Iterate through tup and set some ptrs to j if possible.
                for(SysInt litnum=0; litnum<tup.size(); litnum++) {
                    if(tup[litnum].get<2>()==0) {
                        SysInt var=tup[litnum].get<0>();
                        SysInt val=tup[litnum].get<1>();
                        // Is j different for var 
                        bool jdiff=true;
                        for(SysInt litnumj=0; litnumj<tuple_nd_list[j].size(); litnumj++) {
                            if(tuple_nd_list[j][litnumj].get<0>()==var && tuple_nd_list[j][litnumj].get<1>()==val) {
                                jdiff=false;  // Found the exact same literal. So tuple j is same for var.
                                break;
                            }
                        }
                        if(jdiff) {
                            tup[litnum].get<2>()=j;
                        }
                        else {
                            all_lits_ptr_set=false;
                        }
                    }
                }
                
                if(all_lits_ptr_set) break;
            }
            
            // set any remaining 0's to infinity -- i.e. jump past the end.
            for(SysInt litnum=0; litnum<tup.size(); litnum++) {
                if(tup[litnum].get<2>()==0) {
                    tup[litnum].get<2>()=tuple_nd_list.size();
                }
            }
        }
    }
    
    //bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        SysInt pos=tuple_list_pos[var][val-vars[var].getInitialMin()];
        SysInt listsize=tuple_nd_list.size();
        
        while(pos<listsize) {
            vector<tuple<SysInt,SysInt,SysInt> > & tup=tuple_nd_list[pos];
            bool valid=true;
            
            SysInt tupsize=tup.size();
            for(SysInt j=0; j<tupsize; j++) {
                // If the literal is out of domain, OR includes var but not val, then jump.
                if((! vars[tup[j].get<0>()].inDomain(tup[j].get<1>()) ) || 
                    (tup[j].get<0>()==var && tup[j].get<1>()!=val) ) {
                    D_ASSERT(tup[j].get<2>() > pos);
                    pos=tup[j].get<2>();  
                    valid=false;
                    break;
                }
            }
            if(valid) {
                // Found a support
                for(SysInt j=0; j<tupsize; j++) {
                    ADDTOASSIGNMENT(tup[j].get<0>(), tup[j].get<1>());
                }
                tuple_list_pos[var][val-vars[var].getInitialMin()]=pos;
                return true;
            }
        }
        
        // Restart at position 0
        pos=0;
        
        SysInt oldpos=tuple_list_pos[var][val-vars[var].getInitialMin()];
        D_ASSERT(oldpos<listsize);
        
        while(pos<oldpos) {
            vector<tuple<SysInt,SysInt,SysInt> > & tup=tuple_nd_list[pos];
            bool valid=true;
            
            SysInt tupsize=tup.size();
            for(SysInt j=0; j<tupsize; j++) {
                // If the literal is out of domain, OR includes var but not val, then jump.
                if((! vars[tup[j].get<0>()].inDomain(tup[j].get<1>()) ) || 
                    (tup[j].get<0>()==var && tup[j].get<1>()!=val) ) {
                    D_ASSERT(tup[j].get<2>() > pos);
                    pos=tup[j].get<2>();  
                    valid=false;
                    break;
                }
            }
            if(valid) {
                // Found a support
                for(SysInt j=0; j<tupsize; j++) {
                    ADDTOASSIGNMENT(tup[j].get<0>(), tup[j].get<1>());
                }
                tuple_list_pos[var][val-vars[var].getInitialMin()]=pos;
                return true;
            }
        }
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
    {
        // argh, how to do this.
        // test with element first
        
        SysInt idx=v[array_size-2];
        if(idx<0 || idx>=array_size-2) return false;
        return v[v[array_size-2]] == v[array_size-1];
    }

#endif
   
#if UseSquarePackingShort

    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing.
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant)
    //   ADAPTED FOR THIS BRANCH -- REMOVED EMPTY SUPPORT WHEN CONSTRAINT ENTAILED.
    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        SysInt i=vars[4].getAssignedValue();
        SysInt j=vars[5].getAssignedValue();
        
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
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
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
#endif

#if UseSquarePackingLong
    
    ////////////////////////////////////////////////////////////////////////////
    //
    //  Square packing with full-length supports
    // Expects x1,y1, x2,y2, boxsize1, boxsize2 (constant).

    // bool findNewSupport(box<pair<SysInt, DomainInt> >& assignment, SysInt var, SysInt val) {
    bool findNewSupport(SysInt var, SysInt val) {
        D_ASSERT(vars[4].isAssigned());
        D_ASSERT(vars[5].isAssigned());
        
        SysInt i=vars[4].getAssignedValue();
        SysInt j=vars[5].getAssignedValue();
        
        // object i below object j.
        if(vars[1].getMin()+i <=vars[3].getMax()) {
            if(var==1) {
                if(val+i<=vars[3].getMax()) { 
                    literalsScratch.push_back(make_pair(1, val));
                    literalsScratch.push_back(make_pair(3, vars[3].getMax()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(vars[1].getMin()+i<=val) {
                    literalsScratch.push_back(make_pair(1, vars[1].getMin()));
                    literalsScratch.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(1, vars[1].getMin()));
                literalsScratch.push_back(make_pair(3, vars[3].getMax()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i above object j
        if(vars[3].getMin()+j <= vars[1].getMax()) {
            if(var==1) {
                if(vars[3].getMin()+j <= val) {
                    literalsScratch.push_back(make_pair(1, val));
                    literalsScratch.push_back(make_pair(3, vars[3].getMin()));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else if(var==3) {
                if(val+j <= vars[1].getMax())
                {
                    literalsScratch.push_back(make_pair(1, vars[1].getMax()));
                    literalsScratch.push_back(make_pair(3, val));
                    PADOUT(0)
                    PADOUT(2)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(1, vars[1].getMax()));
                literalsScratch.push_back(make_pair(3, vars[3].getMin()));
                PADOUT(0)
                PADOUT(2)
                return true;
            }
        }
        
        // object i left of object j.
        if(vars[0].getMin()+i <=vars[2].getMax()) {
            if(var==0) {
                if(val+i <=vars[2].getMax()) {
                    literalsScratch.push_back(make_pair(0, val));
                    literalsScratch.push_back(make_pair(2, vars[2].getMax()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(vars[0].getMin()+i <=val) {
                    literalsScratch.push_back(make_pair(0, vars[0].getMin()));
                    literalsScratch.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(0, vars[0].getMin()));
                literalsScratch.push_back(make_pair(2, vars[2].getMax()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        // object i right of object j
        if(vars[2].getMin()+j <= vars[0].getMax()) {
            if(var==0) {
                if(vars[2].getMin()+j <= val) {
                    literalsScratch.push_back(make_pair(0, val));
                    literalsScratch.push_back(make_pair(2, vars[2].getMin()));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else if(var==2) {
                if(val+j <= vars[0].getMax()) {
                    literalsScratch.push_back(make_pair(0, vars[0].getMax()));
                    literalsScratch.push_back(make_pair(2, val));
                    PADOUT(1)
                    PADOUT(3)
                    return true;
                }
            }
            else {
                literalsScratch.push_back(make_pair(0, vars[0].getMax()));
                literalsScratch.push_back(make_pair(2, vars[2].getMin()));
                PADOUT(1)
                PADOUT(3)
                return true;
            }
        }
        
        return false;
    }
    
    virtual BOOL check_assignment(DomainInt* v, SysInt array_size)
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
#endif
    
    ////////////////////////////////////////////////////////////////////////////
    // Memory management.
    
    Support* getFreeSupport() {
        // Either get a Support off the free list or make one.
        if(supportFreeList==0) {
            return new Support();
        }
        else {
            Support* temp=supportFreeList;
            supportFreeList=supportFreeList->nextFree;
            return temp;
        }
    }

    inline void addToSupportFreeList(Support* sup)
    { 
          sup->nextFree=supportFreeList; 
          supportFreeList=sup;
    }
    
    virtual void full_propagate()
    {
       litsWithLostExplicitSupport.resize(0);
       varsWithLostImplicitSupport.resize(0); 

       for(SysInt i=0; i<vars.size(); i++) { 
               varsWithLostImplicitSupport.push_back(i);
       }

       findSupportsInitial();
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

