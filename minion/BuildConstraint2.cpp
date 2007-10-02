/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: BuildConstraint2.cpp 680 2007-10-01 08:19:53Z azumanga $
*/

/*
 *  BuildConstraint2.cpp
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/03/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

#define NO_MAIN

#include "minion.h"
#include "CSPSpec.h"

using namespace ProbSpec;

namespace BuildCon
{  

/// Helper function used in a few places.
AnyVarRef
get_AnyVarRef_from_Var(Var v)
{
  switch(v.type)
		{
		  case VAR_BOOL:
			return AnyVarRef(varContainer.getBooleanContainer().get_var_num(v.pos));
		  case VAR_NOTBOOL:
		    return AnyVarRef(VarNotRef(varContainer.getBooleanContainer().get_var_num(v.pos)));
		  case VAR_BOUND:
			return AnyVarRef(varContainer.getBoundvarContainer().get_var_num(v.pos));
		  case VAR_SPARSEBOUND:
			return AnyVarRef(varContainer.getSparseBoundvarContainer().get_var_num(v.pos));
		  case VAR_DISCRETE_SHORT:
			return AnyVarRef(varContainer.getRangevarContainer().get_var_num(v.pos));
		  case VAR_DISCRETE_LONG:
			return AnyVarRef(varContainer.getBigRangevarContainer().get_var_num(v.pos));
		  case VAR_SPARSEDISCRETE:	
			D_FATAL_ERROR("Sparse Discrete not supported at present");
		  case VAR_CONSTANT:
			return AnyVarRef(ConstantVar(v.pos));
		  default:
		    D_FATAL_ERROR("Unknown Error.");
		}
}

    /// Build the variable and value ordering used.
	/// The var order is placed, the val order is returned.
	pair<vector<AnyVarRef>, vector<int> > build_val_and_var_order(ProbSpec::CSPInstance& instance)
  {
	  vector<int> final_val_order;
	  vector<AnyVarRef> final_var_order;
	  if(instance.var_order.size() != instance.val_order.size())
	    D_FATAL_ERROR("Variable order and value order must be same size.");
	
	  for(unsigned int i = 0 ;i < instance.var_order.size(); ++i)
	  {
		final_val_order.push_back(instance.val_order[i]);
		final_var_order.push_back(get_AnyVarRef_from_Var(instance.var_order[i]));
	  }
	  return make_pair(final_var_order, final_val_order);
  }	

  /// Create all the variables used in the CSP.
  void build_variables(const ProbSpec::VarContainer& vars)
  {
	for(int i = 0; i < vars.BOOLs; ++i)
	{
	  BoolVarRef b = varContainer.getBooleanContainer().get_new_var();
	}
	for(unsigned int i = 0; i < vars.bound.size(); ++i)
	{
	  for(int j = 0; j < vars.bound[i].first; ++j)
	  {
		BoundVarRef b = varContainer.getBoundvarContainer().get_new_var(vars.bound[i].second.lower_bound,
													   vars.bound[i].second.upper_bound);
	  }
	}
	for(unsigned int i = 0; i < vars.sparse_bound.size(); ++i)
	{
	  for(int j = 0; j < vars.sparse_bound[i].first; ++j)
	  {
		SparseBoundVarRef b = 
		varContainer.getSparseBoundvarContainer().get_new_var(vars.sparse_bound[i].second);
	  }
	}
	
	for(unsigned int i = 0; i < vars.discrete.size(); ++i)
	{
	  for(int j = 0; j < vars.discrete[i].first; ++j)
	  {
	    if(varContainer.getRangevarContainer().valid_range(vars.discrete[i].second.lower_bound, vars.discrete[i].second.upper_bound))
	    LRangeVarRef r = varContainer.getRangevarContainer().get_new_var(vars.discrete[i].second.lower_bound,
														vars.discrete[i].second.upper_bound);
		else
		BigRangeVarRef r = varContainer.getBigRangevarContainer().get_new_var(vars.discrete[i].second.lower_bound,
															  vars.discrete[i].second.upper_bound);
	  }
    }
	
	for(unsigned int i = 0; i < vars.sparse_discrete.size(); ++i)
	{ D_FATAL_ERROR("Sparse discrete disabled at present due to bugs. Sorry."); }
  }
	
}



