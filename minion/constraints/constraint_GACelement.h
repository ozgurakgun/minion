/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_GACelement.h 745 2007-11-02 13:37:26Z azumanga $
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


template<typename VarArray, typename IndexRef>
struct GACElementConstraint : public Constraint
{
  virtual string constraint_name()
  { return "GACElement"; }
  
  typedef typename VarArray::value_type VarRef;
  VarArray var_array;
  IndexRef indexvar;
  VarRef resultvar;
  DomainInt var_array_min_val;
  DomainInt var_array_max_val;
  GACElementConstraint(StateObj* _stateObj, const VarArray& _var_array, const IndexRef& _indexvar, const VarRef& _resultvar) :
    Constraint(_stateObj), var_array(_var_array), indexvar(_indexvar), resultvar(_resultvar)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_GACELEMENT,"Setting up Constraint");
    triggerCollection t;
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
	
	t.push_back(make_trigger(indexvar,
							 Trigger(this, array_size), DomainChanged));
	
	t.push_back(make_trigger(resultvar,
							 Trigger(this, array_size + 1), DomainChanged));
    return t;
  }
  
  void index_assigned()
  {
    int index = checked_cast<int>(indexvar.getAssignedValue());
	int array_size = var_array.size();
	
	if(index < 0 || index >= array_size)
	{
	  getState(stateObj).setFailed(true);
	  return;
	}
	
	var_array[index].setMin(resultvar.getMin());
	var_array[index].setMax(resultvar.getMax());
	
	DomainInt min_val = max(var_array[index].getMin(), resultvar.getMin());
	DomainInt max_val = min(var_array[index].getMax(), resultvar.getMax());
	
	for(DomainInt i = min_val; i <= max_val; ++i)
	{
	  if(!resultvar.inDomain(i))
	    var_array[index].removeFromDomain(i);
	}
  }
  
  BOOL support_for_val_in_result(DomainInt val)
  {
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	{
	  if(indexvar.inDomain(i) && var_array[i].inDomain(val))
	    return true;
	}
    return false;
  }
  
  BOOL support_for_val_in_index(DomainInt dom_index)
  {
	int index = checked_cast<int>(dom_index);
    DomainInt min_val = max(var_array[index].getMin(), resultvar.getMin());
	DomainInt max_val = min(var_array[index].getMax(), resultvar.getMax());
	for(DomainInt i = min_val; i <= max_val; ++i)
	{
	  if(var_array[index].inDomain(i) && resultvar.inDomain(i))
	    return true;
	}
    return false;
  }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(GACElement);
    int array_size = var_array.size();
	// DomainInt domain_size = (var_array_max_val - var_array_min_val + 1);
	
	if(indexvar.isAssigned())
	{ index_assigned(); }
	
    if(prop_val < array_size)
	{
	  if(indexvar.inDomain(prop_val) && !support_for_val_in_index(prop_val))
	  {
	    D_INFO(2, DI_GACELEMENT, "No support for var in index");
		indexvar.removeFromDomain(prop_val);
	  }
	  
	  VarRef& var = var_array[prop_val];
	  
	  DomainInt min_val = var.getInitialMin();
	  DomainInt max_val = var.getInitialMax();
	  for(DomainInt val = min_val; val <= max_val; ++val)
	  {
	    if(!var.inDomain(val) && resultvar.inDomain(val) &&
		   !support_for_val_in_result(val))
	    {
	      D_INFO(2, DI_GACELEMENT, "No support for val in result var");
	      resultvar.removeFromDomain(val);
	    }
	  }
	  return;
	}
	
	if(prop_val == array_size)
	{ // Value got removed from index. Basically have to check everything.
	  
	  for(DomainInt i = var_array_min_val; i <= var_array_max_val; ++i)
	  {
		if(resultvar.inDomain(i) && !support_for_val_in_result(i))
		  resultvar.removeFromDomain(i);
	  }
	  return;
	}
	
	D_ASSERT(prop_val == array_size + 1);
	
	for(int var = 0; var < array_size; ++var)
	{
	  if(indexvar.inDomain(var) && !support_for_val_in_index(var))
	  {
		D_INFO(2, DI_GACELEMENT, "No support for var in index");
		indexvar.removeFromDomain(var);
	  }
	}
  }
  
  virtual void full_propagate()
  {
    for(int i=0; i<var_array.size(); i++) 
        if(var_array[i].isBound()) 
            cerr << "Warning: GACElement is not designed to be used on bound variables and may cause crashes." << endl;
    if(indexvar.isBound() || resultvar.isBound())
        cerr << "Warning: GACElement is not designed to be used on bound variables and may cause crashes." << endl;
    indexvar.setMin(0);
	indexvar.setMax(var_array.size() - 1);
	resultvar.setMin(var_array_min_val);
	resultvar.setMax(var_array_max_val);
	for(unsigned i = 0; i < var_array.size() + 2; ++i)
	  propagate(i,0);
  }
  
  virtual BOOL check_assignment(vector<DomainInt> v)
  {
	int length = v.size();
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
	array.push_back(indexvar);
	array.push_back(resultvar);
	return array;
  }
};


// Note: we pass into the first vector into this function by value rather
// than by const reference because we want to change it.
template<typename Var1, typename Var2>
Constraint*
GACElementCon(StateObj* stateObj, Var1 vararray, const Var2& v1)
{ 
  // Because we can only have two things which are parsed at the moment, we do
  // a dodgy hack and store the last variable on the end of the vararray
  // during parsing. Now we must pop it back off.
  typedef typename Var1::value_type VarRef1;
  typedef typename Var2::value_type VarRef2;
  VarRef1 assignval = vararray.back();
  vararray.pop_back();
  return new GACElementConstraint<Var1, VarRef2>(stateObj, vararray, v1[0], assignval);  
}

BUILD_CONSTRAINT2(CT_GACELEMENT, GACElementCon);


