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

  template<typename VarValOrder, typename Propogator>
  void solve(StateObj* stateObj, VarOrder order_in, VarValOrder& search_order, Propogator& prop)
  {
    typedef typename VarValOrder::first_type::value_type VarType;
	switch(order_in)
	{
	  case ORDER_STATIC:
	  {
		Controller::VariableOrder<VarType, Controller::SlowStaticBranch> 
		order(stateObj, search_order.first, search_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
		break;
	  case ORDER_SDF:
	  {
		   Controller::VariableOrder<VarType, Controller::SDFBranch> 
		   order(stateObj, search_order.first, search_order.second);
		   
		   try 
		   { Controller::solve_loop(stateObj, order, search_order.first, prop); }
		   catch(...)
		   { }
	  }
	  break;
	  case ORDER_LDF:
	  {
		Controller::VariableOrder<VarType, Controller::LDFBranch> 
		order(stateObj, search_order.first, search_order.second);
		
		try 
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
		break;
		
	  case ORDER_ORIGINAL:
	  {  
		Controller::VariableOrder<VarType, Controller::StaticBranch>
		order(stateObj, search_order.first, search_order.second);
		try
		{ Controller::solve_loop(stateObj, order, search_order.first, prop); }
		catch(...)
		{ }
	  }
		break;
	  default:
		FAIL_EXIT();
	} 
  }

