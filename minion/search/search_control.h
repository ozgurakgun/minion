/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/


enum VarOrder
{
  ORDER_STATIC,
  ORDER_SDF,
  ORDER_LDF,
  ORDER_ORIGINAL
};

  template<typename VarValOrder>
  void solve(StateObj* stateObj, VarOrder order, VarValOrder& var_val_order)
  {
    typedef typename VarValOrder::first_type::value_type VarType;
	switch(order)
	{
	  case ORDER_STATIC:
	  {
		Controller::VariableOrder<VarType, Controller::SlowStaticBranch> 
		order(stateObj, var_val_order.first, var_val_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
	  case ORDER_SDF:
	  {
		   Controller::VariableOrder<VarType, Controller::SDFBranch> 
		   order(stateObj, var_val_order.first, var_val_order.second);
		   
		   try 
		   { Controller::solve_loop(stateObj, order, var_val_order.first); }
		   catch(...)
		   { }
	  }
	  break;
	  case ORDER_LDF:
	  {
		Controller::VariableOrder<VarType, Controller::LDFBranch> 
		order(stateObj, var_val_order.first, var_val_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
		
	  case ORDER_ORIGINAL:
	  {  
		Controller::VariableOrder<VarType, Controller::StaticBranch>
		order(stateObj, var_val_order.first, var_val_order.second);
		try
		{ Controller::solve_loop(stateObj, order, var_val_order.first); }
		catch(...)
		{ }
	  }
		break;
	  default:
		FAIL_EXIT();
	} 
  }

