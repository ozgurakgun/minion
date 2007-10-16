/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: BuildConstraint2.cpp 701 2007-10-09 14:12:05Z azumanga $
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
get_AnyVarRef_from_Var(StateObj* stateObj, Var v)
{
  switch(v.type)
		{
		  case VAR_BOOL:
			return AnyVarRef(stateObj->varCon().getBooleanContainer().get_var_num(v.pos));
		  case VAR_NOTBOOL:
		    return AnyVarRef(VarNotRef(stateObj->varCon().getBooleanContainer().get_var_num(v.pos)));
		  case VAR_BOUND:
			return AnyVarRef(stateObj->varCon().getBoundvarContainer().get_var_num(v.pos));
		  case VAR_SPARSEBOUND:
			return AnyVarRef(stateObj->varCon().getSparseBoundvarContainer().get_var_num(v.pos));
		  case VAR_DISCRETE_SHORT:
			return AnyVarRef(stateObj->varCon().getRangevarContainer().get_var_num(v.pos));
		  case VAR_DISCRETE_LONG:
			return AnyVarRef(stateObj->varCon().getBigRangevarContainer().get_var_num(v.pos));
		  case VAR_SPARSEDISCRETE:	
			D_FATAL_ERROR("Sparse Discrete not supported at present");
		  case VAR_CONSTANT:
			return AnyVarRef(ConstantVar(stateObj, v.pos));
		  default:
		    D_FATAL_ERROR("Unknown Error.");
		}
}

    /// Build the variable and value ordering used.
	/// The var order is placed, the val order is returned.
	pair<vector<AnyVarRef>, vector<int> > build_val_and_var_order(StateObj* stateObj, ProbSpec::CSPInstance& instance)
  {
	  vector<int> final_val_order;
	  vector<AnyVarRef> final_var_order;
	  if(instance.var_order.size() != instance.val_order.size())
	    D_FATAL_ERROR("Variable order and value order must be same size.");
	
	  for(unsigned int i = 0 ;i < instance.var_order.size(); ++i)
	  {
		final_val_order.push_back(instance.val_order[i]);
		final_var_order.push_back(get_AnyVarRef_from_Var(stateObj, instance.var_order[i]));
	  }
	  return make_pair(final_var_order, final_val_order);
  }	

  /// Create all the variables used in the CSP.
  void build_variables(StateObj* stateObj, const ProbSpec::VarContainer& vars)
  {
    stateObj->varCon().getBooleanContainer().setVarCount(vars.BOOLs);
	stateObj->varCon().getBoundvarContainer().addVariables(vars.bound);
    stateObj->varCon().getSparseBoundvarContainer().addVariables(vars.sparse_bound);


    vector<pair<int, Bounds> > smallRange;
    vector<pair<int, Bounds> > largeRange;

	for(unsigned int i = 0; i < vars.discrete.size(); ++i)
	{
        if(stateObj->varCon().getRangevarContainer().valid_range(vars.discrete[i].second.lower_bound, vars.discrete[i].second.upper_bound))
          smallRange.push_back(vars.discrete[i]);
        else
          largeRange.push_back(vars.discrete[i]);
    }
	
    stateObj->varCon().getRangevarContainer().addVariables(smallRange);
    stateObj->varCon().getBigRangevarContainer().addVariables(largeRange);
	
	for(unsigned int i = 0; i < vars.sparse_discrete.size(); ++i)
	{ D_FATAL_ERROR("Sparse discrete disabled at present due to bugs. Sorry."); }
  }
	
}



