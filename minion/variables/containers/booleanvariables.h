/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: booleanvariables.h 787 2007-11-13 11:31:37Z neilmoore67 $
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

/** @help variables;01 Description 
01 variables are used very commonly for logical expressions, and for
encoding the characteristic functions of sets and relations. Note that
wherever a 01 variable can appear, the negation of that variable can
also appear. A boolean variable x's negation is identified by !x.
*/

/** @help variables;01 Example 
Declaration of a 01 variable called bool in input file:

BOOL bool

Use of this variable in a constraint:

eq(bool, 0) #variable bool equals 0
*/

/// Standard data type used for storing compressed booleans 
typedef unsigned long data_type;
static const data_type one = 1;
static const data_type max_data = one << ( sizeof(data_type) - 1 );

struct BooleanContainer;

/// A reference to a boolean variable
struct BoolVarRef_internal
{
  static const BOOL isBool = true;
  static const BoundType isBoundConst = Bound_No;
  static string name() { return "Bool"; }
  BOOL isBound()
  { return false;}
  
  data_type shift_offset;
  unsigned data_offset;
  unsigned var_num;
  MoveablePointer data_position;
  MemOffset value_position;

#ifdef MANY_VAR_CONTAINERS
  BooleanContainer* boolCon;
  BooleanContainer& getCon() const { return *boolCon; }

  BoolVarRef_internal(const BoolVarRef_internal& b) :
  shift_offset(b.shift_offset), data_offset(b.data_offset), var_num(b.var_num), data_position(b.data_position),
  value_position(b.value_position) ,boolCon(b.boolCon)
  { }
  
  BoolVarRef_internal() : shift_offset(~1), data_offset(~1), var_num(~1), boolCon(NULL)
  { }  
#else
  static BooleanContainer& getCon_Static();
  BoolVarRef_internal(const BoolVarRef_internal& b) :
  shift_offset(b.shift_offset), data_offset(b.data_offset), var_num(b.var_num), data_position(b.data_position),
  value_position(b.value_position)
  { }
  
  BoolVarRef_internal() : shift_offset(~1), data_offset(~1), var_num(~1)
  { }
#endif
  
  BoolVarRef_internal(int value, BooleanContainer* b_con);
  
  data_type& assign_ptr() const
  { return *static_cast<data_type*>(data_position.get_ptr()); }
  
  data_type& value_ptr() const
  { return *static_cast<data_type*>(value_position.get_ptr()); }
  
  BOOL isAssigned() const
  { return assign_ptr() & shift_offset; }
  
  DomainInt getAssignedValue() const
  {
    D_ASSERT(isAssigned());
    return (bool)(value_ptr() & shift_offset);
  }
  
  BOOL inDomain(DomainInt b) const
  {
    if(b < 0 || b > 1) 
	  return false;
    return (!isAssigned()) || (b == getAssignedValue());
  }
  
  BOOL inDomain_noBoundCheck(DomainInt b) const
  {
    D_ASSERT(b == 0 || b == 1);
	return (!isAssigned()) || (b == getAssignedValue());
  }
  
  DomainInt getMin() const
  {
    if(!isAssigned()) return 0;
    return getAssignedValue();
  }
  
  DomainInt getMax() const
  {
    if(!isAssigned()) return 1;
    return getAssignedValue();
  }

  unsigned getDepth() const;

  unsigned getSeqNo() const;

  DynamicConstraint* getAntecedent() const;

  int getId() const
  { return var_num; }
 
  DomainInt getInitialMin() const
  { return 0; }
  
  DomainInt getInitialMax() const
  { return 1; }
 
  friend std::ostream& operator<<(std::ostream& o, const BoolVarRef_internal& b)
  { return o << "Bool:" << b.var_num; }
  
};

struct GetBooleanContainer;

#ifdef MORE_SEARCH_INFO
typedef InfoRefType<QuickVarRefType<GetBooleanContainer, BoolVarRef_internal>, VAR_INFO_BOOL> BoolVarRef;
#else
typedef QuickVarRefType<GetBooleanContainer, BoolVarRef_internal> BoolVarRef;
#endif

/// Container for boolean variables
struct BooleanContainer
{
  StateObj* stateObj;
  BooleanContainer(StateObj* _stateObj) : stateObj(_stateObj), var_count_m(0), lock_m(false), 
       trigger_list(stateObj, false), curr_seq_no(0)
  {}
  
  static const int width = 7;
  MoveablePointer assign_offset;
  MemOffset values_mem;
  unsigned var_count_m;
  TriggerList trigger_list;
  /// When false, no variable can be altered. When true, no variables can be created.
  BOOL lock_m;

  AnyVarRef conflict_var; //last conflicting variable
  DynamicConstraint* last_clause; //last clause to propagate

  vector<DynamicConstraint*> antecedents; //index by var no
  vector<unsigned> depths; //index by var no
  vector<unsigned> seq_nos; //lower numbers for earlier propagations
  unsigned curr_seq_no; //current number of propagations that have happened
  
  data_type* value_ptr()
  { return static_cast<data_type*>(values_mem.get_ptr()); }
  
  const data_type* value_ptr() const
  { return static_cast<const data_type*>(values_mem.get_ptr()); }
  
  data_type* assign_ptr()
  { return static_cast<data_type*>(assign_offset.get_ptr()); }
  
  const data_type* assign_ptr() const
  { return static_cast<const data_type*>(assign_offset.get_ptr()); }
  
  void lock()
  { 
   lock_m = true;
	// Min domain value = 0, max domain val = 1.
    trigger_list.lock(var_count_m, 0, 1);
  }
  
  /// Returns a new Boolean Variable.
  //BoolVarRef get_new_var();

  void setVarCount(int bool_count)
  {
    D_ASSERT(!lock_m);
    var_count_m = bool_count;
    
    antecedents = vector<DynamicConstraint*>(bool_count, (DynamicConstraint*)0);
    depths = vector<unsigned>(bool_count, 0);
    seq_nos = vector<unsigned>(bool_count, 0);

	int required_mem = var_count_m / 8 + 1;
	// Round up to nearest data_type block
	required_mem += sizeof(data_type) - (required_mem % sizeof(data_type));
    assign_offset = getMemory(stateObj).backTrack().request_bytes(required_mem);
    values_mem = getMemory(stateObj).nonBackTrack().request_bytes(required_mem);
  }
  
  /// Returns a reference to the ith Boolean variable which was previously created.
  BoolVarRef get_var_num(int i);
  
  
  void setMax(BoolVarRef_internal& d, DomainInt i) 
  {
    if(i < 0)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}

    D_ASSERT(i >= 0);
	if(i==0)
      propagateAssign(d,0);
  }
  
  void setMin(BoolVarRef_internal& d, DomainInt i) 
  {
    if(i > 1)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
    D_ASSERT(i <= 1);
    if(i==1)
      propagateAssign(d,1);
  }

  void setDepth(BoolVarRef_internal& bvr, unsigned d)
  { 
    depths[bvr.var_num] = d; 
  }
  
  void setAntecedent(BoolVarRef_internal& d, DynamicConstraint* a)
  { 
    antecedents[d.var_num] = a; 
    seq_nos[d.var_num] = curr_seq_no++; //remember sequence no
  }
  
  void removeFromDomain(BoolVarRef_internal& d, DomainInt b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m);
    if(b != 0 && b != 1)
      return;
      
    if(d.isAssigned())
    {
      if(b == d.getAssignedValue()) 
	    getState(stateObj).setFailed(true);
    }
    else
      uncheckedAssign(d,1-b);
  }
  
  void uncheckedAssign(BoolVarRef_internal& d, DomainInt b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m);
    D_ASSERT(!d.isAssigned());
    depths[d.var_num] = getMemory(stateObj).backTrack().current_depth();
    //assume this is a search decision, the propagator can set the true
    //antecedent
    antecedents[d.var_num] = 0; 
    if(b!=0 && b!=1)
    {
      getState(stateObj).setFailed(true);
      return;
    }
    assign_ptr()[d.data_offset] |= d.shift_offset;
    
    trigger_list.push_assign(d.var_num, b);
#ifndef FEW_BOOLEAN_TRIGGERS
    trigger_list.push_domain(d.var_num);
	trigger_list.push_domain_removal(d.var_num, 1 - b);
#endif
    
    if(b == 1)
    {
      trigger_list.push_lower(d.var_num, 1);
      value_ptr()[d.data_offset] |= d.shift_offset;
    }
    else
    {
      trigger_list.push_upper(d.var_num, 1);
      value_ptr()[d.data_offset] &= ~d.shift_offset;
    }
  }
  
  void propagateAssign(BoolVarRef_internal& d, DomainInt b)
  {
    if(!d.isAssigned()) {
      uncheckedAssign(d,b);
    }
    else
    {
      if(d.getAssignedValue() != b)
	getState(stateObj).setFailed(true);
    }
  }

  void addTrigger(BoolVarRef_internal& b, Trigger t, TrigType type)
  { 
    D_ASSERT(lock_m); 
#ifdef FEW_BOOLEAN_TRIGGERS
    if(type == DomainChanged)
      type = Assigned;
    // Static triggers should never be of this type!
    D_ASSERT(type != DomainRemoval);
#endif
    trigger_list.add_trigger(b.var_num, t, type); 
  }
    
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BoolVarRef_internal& b, DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    D_ASSERT(pos == -999 || ( type == DomainRemoval && pos != -999 ) );
    D_ASSERT(lock_m);
    
#ifdef FEW_BOOLEAN_TRIGGERS
    TrigType new_type = type;
    switch(type)
    {
      case DomainChanged:
        new_type = Assigned; break;
      case DomainRemoval:
        if(pos == 0)
          new_type = LowerBound;
        else
          new_type = UpperBound;
    }
    pos = -999;
#endif    
	trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
};

inline BoolVarRef BooleanContainer::get_var_num(int i)
{
  D_ASSERT(i < (int)var_count_m);
  return BoolVarRef(BoolVarRef_internal(i, this));
}

inline unsigned BoolVarRef_internal::getDepth() const
{ 
  return GET_LOCAL_CON().depths[var_num]; 
}

inline unsigned BoolVarRef_internal::getSeqNo() const
{ 
  return GET_LOCAL_CON().seq_nos[var_num]; 
}

inline DynamicConstraint* BoolVarRef_internal::getAntecedent() const
{ 
  return GET_LOCAL_CON().antecedents[var_num]; 
}



inline BoolVarRef_internal::BoolVarRef_internal(int value, BooleanContainer* b_con) : 
  data_offset(value / (sizeof(data_type)*8)), var_num(value),  
  data_position(b_con->assign_offset, data_offset*sizeof(data_type)),
  value_position(b_con->values_mem, data_offset*sizeof(data_type))
#ifdef MANY_VAR_CONTAINERS
, boolCon(b_con)
#endif
{ shift_offset = one << (value % (sizeof(data_type)*8)); }


