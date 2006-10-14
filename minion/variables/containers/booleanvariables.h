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


// Subversion Identity $Id:$

/// Standard data type used for storing compressed booleans 
typedef unsigned long data_type;
static const data_type one = 1;
static const data_type max_data = one << ( sizeof(data_type) - 1 );

struct BooleanContainer;

/// A reference to a boolean variable
struct BoolVarRef_internal
{
  static const bool isBool = true;
  static const BoundType isBoundConst = Bound_No;

  
  bool isBound()
  { return false;}
  
  unsigned var_num;
  unsigned twice_var_num;
  
  BoolVarRef_internal(int value, BooleanContainer* b_con);
  
  BoolVarRef_internal(const BoolVarRef_internal& b) :
    var_num(b.var_num), 
    twice_var_num(b.twice_var_num)
  {}
  
  BoolVarRef_internal()
  { 
    D_DATA(var_num = twice_var_num = ~1); 
  }
  

  bool inDomain_noBoundCheck(int b) const
  {
    D_ASSERT(b == 0 || b == 1);
    return hack_bms->isMember(twice_var_num+b);
  }
   
  bool isAssigned() const               
  { return (!inDomain_noBoundCheck(0) || !inDomain_noBoundCheck(1)) ; }


  
  int getAssignedValue() const
  {
    // possibly trying to be too clever here since this returns int
    // Idea is that (since we assume var is assigned) it's value is 1 iff 1 is in 
    //   the domain, else 0.
    D_ASSERT(isAssigned());
    //return (int)inDomain_noBoundCheck(1);
    if (inDomain_noBoundCheck(1)) {return 1;}
    return 0;
  }
  
  bool inDomain(int b) const
  {
    if(b < 0 || b > 1) return false;
    return inDomain_noBoundCheck(b);
  }
  
  
  int getMin() const
  {
    // possibly trying to be too clever again since this returns int
    // Idea is that min is 0 iff 0 is in the domain
    // Assumes it is ok to return 1 even if 1 is not in the domain in case of 
    // domain wipe out, since failure should occur for other reasons.

    if (inDomain_noBoundCheck(0)) {return 0;}
    return 1;
    // Could put ASSERT here that 1 is in the domain if we return 1, but don't want 
    // to since it could happen legally I think.
  }
  
  int getMax() const
    // see comments for getMin
  {
    //return (int)inDomain_noBoundCheck(1);
    if (inDomain_noBoundCheck(1)) {return 1;}
    return 0;
  }
 
  int getInitialMin() const
  { return 0; }
  
  int getInitialMax() const
  { return 1; }
 
};

struct GetBooleanContainer;
 typedef QuickVarRefType<GetBooleanContainer, BoolVarRef_internal> BoolVarRef;

/// Container for boolean variables
struct BooleanContainer
{
  static const int width = 7;
  unsigned var_count_m;
  TriggerList trigger_list;


  /// When false, no variable can be altered. When true, no variables can be created.
  bool lock_m;
  
  
  void lock()
  { 
    D_ASSERT(!lock_m);
    lock_m = true;
    
    hack_bms = new MonotonicSet(2*var_count_m);

	// Min domain value = 0, max domain val = 1.
    trigger_list.lock(var_count_m, 0, 1);
  }
  
  BooleanContainer() :  var_count_m(0), lock_m(false)
  {}
  
  /// Returns a new Boolean Variable.
  BoolVarRef get_new_var();
  
  /// Returns a reference to the ith Boolean variable which was previously created.
  BoolVarRef get_var_num(int i);

  
  
  void setMax(const BoolVarRef_internal& d, int i) 
  {
    if(i < 0)
	{
	  Controller::fail();
	  return;
	}

    D_ASSERT(i >= 0);
      if(i==0)
      propogateAssign(d,0);
  }
  
  void setMin(const BoolVarRef_internal& d, int i) 
  {
    if(i > 1)
	{
	  Controller::fail();
	  return;
	}
    D_ASSERT(i <= 1);
    if(i==1)
      propogateAssign(d,1);
  }
  
  void removeFromDomain(const BoolVarRef_internal& d, int b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m);
    if(d.isAssigned())
    {
      if(b == d.getAssignedValue()) 
	    Controller::fail();
    }
    else
      uncheckedAssign(d,1-b);
  }
  
  void uncheckedAssign(const BoolVarRef_internal& d, int b)
  {
    D_ASSERT(lock_m && d.var_num < var_count_m && d.twice_var_num == 2*d.var_num);
    D_ASSERT(!d.isAssigned());
    if(b!=0 && b!=1)
    {
	  Controller::fail();
	  return;
    }
    hack_bms->remove(d.twice_var_num+1-b);
    trigger_list.push_domain(d.var_num);
    trigger_list.push_assign(d.var_num, b);
    trigger_list.push_domain_removal(d.var_num, 1 - b);
    
    if(b)
    {
      trigger_list.push_lower(d.var_num, 1);
    }
    else
    {
      trigger_list.push_upper(d.var_num, 1);
    }
  }
  
  void propogateAssign(const BoolVarRef_internal& d, int b)
  {
    if(!d.isAssigned()) 
      uncheckedAssign(d,b);
    else
    {
      if(d.getAssignedValue() != b)
	Controller::fail();
    }
  }

  void addTrigger(BoolVarRef_internal& b, Trigger t, TrigType type, int val)
  { D_ASSERT(lock_m); trigger_list.add_trigger(b.var_num, t, type, val); }
    
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(BoolVarRef_internal& b, DynamicTrigger* t, TrigType type, int pos = -999)
  { 
    D_ASSERT(pos == -999 || ( type == DomainRemoval && pos != -999 ) );
    D_ASSERT(lock_m);
	trigger_list.addDynamicTrigger(b.var_num, t, type, pos); 
  }
#endif
  
  /*  operator std::string()
  {
    D_ASSERT(lock_m);
    stringstream s;
    int char_count = 0;
    for(unsigned int i=0;i<var_count_m;i++)
    {
      if(!isAssigned(BoolVarRef_internal(i,this)))
	s << "X";
      else
      {
	s << (getAssignedValue(BoolVarRef_internal(i,this))?1:0); 
      }
      char_count++;
      if(char_count%width==0) s << endl;
    }
    return s.str();
  }*/
};

VARDEF(BooleanContainer boolean_container);

struct GetBooleanContainer
{
  static BooleanContainer& con() { return boolean_container; }
};




inline BoolVarRef BooleanContainer::get_new_var()
{ 
  D_ASSERT(!lock_m);
  return BoolVarRef(BoolVarRef_internal(var_count_m++, this));
}

inline BoolVarRef BooleanContainer::get_var_num(int i)
{
  D_ASSERT(!lock_m);
  D_ASSERT(i < (int)var_count_m);
  return BoolVarRef(BoolVarRef_internal(i, this));
}

inline BoolVarRef_internal::BoolVarRef_internal(int value, BooleanContainer* b_con) : 
  var_num(value)  
{ 
  twice_var_num = 2*var_num;
}

