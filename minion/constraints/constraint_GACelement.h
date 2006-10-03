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
  int var_array_min_val;
  int var_array_max_val;
  GACElementConstraint(const VarArray& _var_array, const IndexRef& _indexvar, const VarRef& _resultvar) :
    var_array(_var_array), indexvar(_indexvar), resultvar(_resultvar)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_GACELEMENT,"Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
	int min_val = var_array[0].getInitialMin();
	int max_val = var_array[0].getInitialMax();
	for(int i = 1; i < array_size; ++i)
	{
	  min_val = min(min_val, var_array[i].getInitialMin());
	  max_val = max(max_val, var_array[i].getInitialMax());
	}
	
	var_array_min_val = min_val;
	var_array_max_val = max_val;
	
	int domain_size = var_array_max_val - var_array_min_val + 1;
	for(int i = 0; i < array_size; ++i)
	{
	  for(int j = var_array_min_val; j <= var_array_max_val; ++j)
	  {
	    t.push_back(make_trigger(var_array[i], 
								 Trigger(this, i * domain_size + j - var_array_min_val), DomainRemoval, j));
	  }
	}
	
	int base_trig = (array_size + 1) * domain_size;
	for(int i = indexvar.getInitialMin(); i <= indexvar.getInitialMax(); ++i)
	{
	  t.push_back(make_trigger(indexvar,
							   Trigger(this, base_trig + i - indexvar.getInitialMin()), DomainRemoval, i));
	}
	
	base_trig += indexvar.getInitialMax() - indexvar.getInitialMin() + 1;
	for(int i = resultvar.getInitialMin(); i <= resultvar.getInitialMax(); ++i)
	  t.push_back(make_trigger(resultvar,
							   Trigger(this, base_trig + i - resultvar.getInitialMin()), DomainRemoval, i));
    return t;
  }
  
  void index_assigned()
  {
    int index = indexvar.getAssignedValue();
	int array_size = var_array.size();
	
	if(index < 0 || index >= array_size)
	{
	  Controller::fail();
	  return;
	}
	
	var_array[index].setMin(resultvar.getMin());
	var_array[index].setMax(resultvar.getMax());
	
	int min_val = max(var_array[index].getMin(), resultvar.getMin());
	int max_val = min(var_array[index].getMax(), resultvar.getMax());
	
	for(int i = min_val; i <= max_val; ++i)
	{
	  if(!resultvar.inDomain(i))
	    var_array[index].removeFromDomain(i);
	}
  }
  
  bool support_for_val_in_result(int val)
  {
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	{
	  if(indexvar.inDomain(i) && var_array[i].inDomain(val))
	    return true;
	}
    return false;
  }
  
  bool support_for_val_in_index(int index)
  {
    int min_val = max(var_array[index].getMin(), resultvar.getMin());
	int max_val = min(var_array[index].getMax(), resultvar.getMax());
	for(int i = min_val; i <= max_val; ++i)
	{
	  if(var_array[index].inDomain(i) && resultvar.inDomain(i))
	    return true;
	}
    return false;
  }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
    int array_size = var_array.size();
	int domain_size = (var_array_max_val - var_array_min_val + 1);
	if(indexvar.isAssigned())
	{ index_assigned(); }
	
    if(prop_val < (array_size + 1) * domain_size)
	{
	  int raw_val = prop_val % domain_size;
	  int var = (prop_val - raw_val) / domain_size;
	  int val = raw_val + var_array_min_val;
	  
	  D_INFO(2, DI_GACELEMENT, "From array, var " + to_string(var) + ", val " + to_string(val));
	  
	  if(indexvar.inDomain(var) && !support_for_val_in_index(var))
	  {
	    D_INFO(2, DI_GACELEMENT, "No support for var in index");
		indexvar.removeFromDomain(var);
	  }
	  
	  if(resultvar.inDomain(val) && !support_for_val_in_result(val))
	  {
	    D_INFO(2, DI_GACELEMENT, "No support for val in result var");
	    resultvar.removeFromDomain(val);
	  }
	  return;
	}
	
	prop_val -= (array_size + 1) * domain_size ;
	
	if(prop_val < indexvar.getInitialMax() - indexvar.getInitialMin() + 1)
	{ // Value got removed from index. Basically have to check everything.
	  
	  for(int i = var_array_min_val; i <= var_array_max_val; ++i)
	  {
		if(resultvar.inDomain(i) && !support_for_val_in_result(i))
		  resultvar.removeFromDomain(i);
	  }
	  return;
	}
	prop_val -= indexvar.getInitialMax() - indexvar.getInitialMin() + 1;
	
	D_ASSERT(prop_val < resultvar.getInitialMax() - resultvar.getInitialMin() + 1);
	
	for(int var = 0; var < array_size; ++var)
	{
	  if(indexvar.inDomain(var) && !support_for_val_in_index(var))
	  {
		D_INFO(2, DI_GACELEMENT, "No support for var in index");
		indexvar.removeFromDomain(var);
	  }
	}
  }
  
  virtual void full_propogate()
  {
    indexvar.setMin(0);
	indexvar.setMax(var_array.size() - 1);
	int total_trigger_count = (var_array.size() + 1) * 
	(var_array_max_val - var_array_max_val + 1) +
	indexvar.getInitialMax() - indexvar.getInitialMin() + 1 +
	resultvar.getInitialMax() - resultvar.getInitialMin() + 1;
	for(int i = 0; i < total_trigger_count; ++i)
	  propogate(i,0);
  }
  
  virtual bool check_assignment(vector<int> v)
  {
	int length = v.size();
	if(v[length-2] < 0 || v[length-2] > length - 3)
	  return false;
	return v[v[length-2]] == v[length-1];
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

template<typename VarArray, typename VarRef1, typename VarRef2>
Constraint*
GACElementCon(const VarArray& vararray, const VarRef1& v1, const VarRef2& v2)
{ return new GACElementConstraint<VarArray, VarRef1>(vararray, v1, v2); }

