#include <algorithm>
#include "literal.h"

float decay(float f) { return 0.95 * f; }

struct VSIDSBranch
{
  bool already_setup;
  vector<float> scores;
  
  VSIDSBranch() : already_setup(false) {}

  template<typename VarType>
  int operator()(vector<VarType>& var_order, int)
  {
    if(!already_setup) {
      scores = vector<float>(var_order.size(), 0.0);
      already_setup = true;
    }
    cout << scores << endl;
    vector<float>::iterator curr = scores.begin();
    vector<float>::iterator end = scores.end();
    size_t it_pos = 0;
    float max = -1.0;
    size_t max_pos = var_order.size(); //return last_var+1 when all assigned
    while(curr != end) {
      if(max < *curr && !(var_order[it_pos].isAssigned())) {
	max = *curr;
	max_pos = it_pos;
      } 
      it_pos++;
      curr++;
    }
    std::transform(scores.begin(), scores.end(), scores.begin(), decay);
    return max_pos; //highest priority unassigned variable
  }

  //this function is used to update the heuristic whenever ANY clause is used in
  //conflict analysis
  void update(list<literal>& clause)
  {
    list<literal>::iterator curr = clause.begin();
    list<literal>::iterator end = clause.end();
    while(curr != end) {
      scores[(*curr).id] += 1.0;
      curr++;
    }
  }
};

struct StaticBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	unsigned v_size = var_order.size();
	while(pos < v_size && var_order[pos].isAssigned())
	  ++pos;
	return pos;
  }

  void update(list<literal>& clause) {}
};

struct SlowStaticBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	unsigned v_size = var_order.size();
	pos = 0;
	while(pos < v_size && var_order[pos].isAssigned())
	  ++pos;
	return pos;
  }

  void update(list<literal>& clause) {}
};

struct SDFBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	int length = var_order.size();
	int smallest_dom = length;
	DomainInt dom_size = DomainInt_Max;
	
	
	for(int i = 0; i < length; ++i)
	{
	  DomainInt maxval = var_order[i].getMax();
	  DomainInt minval = var_order[i].getMin();
	  
	  if((maxval != minval) && ((maxval - minval) < dom_size) )
	  {
		dom_size = maxval - minval;
		smallest_dom = i;
		if( maxval - minval == 1)
		{ // Binary domain, must be smallest
		  return i;
		}
	  }
	}
	return smallest_dom;
  }

  void update(list<literal>& clause) {}
};

struct LDFBranch
{
  template<typename VarType>
  int operator()(vector<VarType>& var_order, int pos)
  {
	int length = var_order.size();
	
	pos = 0;
	while(pos < length && var_order[pos].isAssigned())
	  ++pos;
	if(pos == length)
	  return length;
	
	int largest_dom = pos;
	DomainInt dom_size = var_order[pos].getMax() - var_order[pos].getMin();
	
	++pos;
	
	for(; pos < length; ++pos)
	{
	  DomainInt maxval = var_order[pos].getMax();
	  DomainInt minval = var_order[pos].getMin();
	  
	  if(maxval - minval > dom_size)
	  {
		dom_size = maxval - minval;
		largest_dom = pos;
	  }
	}
	return largest_dom;
  }

  void update(list<literal>& clause) {}
};
