/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

template<typename Var, typename Vars, typename Prop>
bool inline check_fail(StateObj* stateObj, Var& var, DomainInt val, Vars& vars, Prop prop, bool checkBounds)
{
  Controller::world_push(stateObj);
  var.propagateAssign(val);
  prop(stateObj, vars, checkBounds);
  
  bool check_failed = getState(stateObj).isFailed();
  getState(stateObj).setFailed(false);
  
  Controller::world_pop(stateObj);
  
  return check_failed;
}

template <typename Var, typename Prop>
void propagateSAC_internal(StateObj* stateObj, vector<Var>& vararray, Prop prop, bool checkBounds)
{
  getQueue(stateObj).propagateQueue();
  if(getState(stateObj).isFailed())
	return;
  
  bool reduced = true;
  while(reduced)
  {
    reduced = false;
    for(int i = 0; i < vararray.size(); ++i)
    {
      Var& var = vararray[i];
      if(var.isBound() || checkBounds)
      {
        while(check_fail(stateObj, var, var.getMax(), vararray, prop, checkBounds))
        {
          reduced = true;
          var.setMax(var.getMax() - 1);
          prop(stateObj, vararray, checkBounds);
          if(getState(stateObj).isFailed())
            return;
        }
        
        while(check_fail(stateObj, var, var.getMin(), vararray, prop, checkBounds))
        {
          reduced = true;
          var.setMin(var.getMin() + 1);
          prop(stateObj, vararray, checkBounds);
          if(getState(stateObj).isFailed())
            return;
        }
      }
      else
      {
        for(DomainInt val = var.getMin(); val <= var.getMax(); ++val)
        {
          if(var.inDomain(val) && check_fail(stateObj, var, val, vararray, prop, checkBounds))
          {
            reduced = true;
            var.removeFromDomain(val);
            prop(stateObj, vararray, checkBounds);
            if(getState(stateObj).isFailed())
              return;          
          }
        }
      }
    }
  }
}


struct PropagateSAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars, bool checkBounds)
  {propagateSAC_internal(stateObj, vars, propagate_queue_vars<Vars>, checkBounds);}
};


struct PropagateSSAC
{
  template<typename Vars>
  void operator()(StateObj* stateObj, Vars& vars, bool checkBounds)
  {
	PropagateSAC sac;
	propagateSAC_internal(stateObj, vars, sac, checkBounds);
  }
};
