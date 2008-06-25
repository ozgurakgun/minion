#ifndef PARANOID_DECS
#define PARANOID_DECS

#ifdef PARANOID

static StateObj* debugStateObj = new StateObj();
static pair<vector<AnyVarRef>, vector<int> > debug_var_val_order;
static bool doing_checks; //this is T when a check is in progress, so that
			  //checkExpln() will not do any recursive checking

//takes a pruning and the g-explanation computed for it, and makes sure that by
//posting the literals in the explanation, the pruning is repeated BY PROPAGATION
inline bool checkExpln(literal pruning, vector<literal> expln) {
  if(!doing_checks) { //don't check explanations produced in the debug solver
    cout << "checking" << endl;
    doing_checks = true;
    getMemory(debugStateObj).backTrack().world_push();
    const size_t expln_s = expln.size();
    for(int i = 0; i < expln_s; i++) {
      literal lit = expln[i];
      if(lit.asgn)
	get_AnyVarRef_from_Var(debugStateObj, lit.var).propagateAssign(lit.val, label());
      else
	get_AnyVarRef_from_Var(debugStateObj, lit.var).removeFromDomain(lit.val, label());
    }
    PropagateGAC prop = PropagateGAC();
    prop(debugStateObj, debug_var_val_order);
    bool notpruned = get_AnyVarRef_from_Var(debugStateObj, pruning.var).inDomain(pruning.val);
    getMemory(debugStateObj).backTrack().world_pop();
    doing_checks = false;
    cout << "notpruned=" << notpruned << endl;
    return !notpruned;
  }
}

#else

inline bool checkExpln(literal, vector<literal>) { return true; } //do nothing when paranoid is off

#endif //PARANOID

#endif //PARANOID_DECS

#ifndef LEARNING_HPP
#define LEARNING_HPP

inline bool depth_sort(const pair<literal,depth>& p1, const pair<literal,depth>& p2) {
  return p1.second < p2.second;
}

inline label computeFirstUip(StateObj* stateObj, Var v)
{
  //cout << "failed var=" << v << endl;
  vector<pair<literal,depth> > at_curr_depth; //literals with depths from current decision level
  vector<literal> final; //literals in from earlier decision levels, unsorted, maybe with duplicates
  AnyVarRef v_avr = get_AnyVarRef_from_Var(stateObj, v);
  const depth decision_depth(getMemory(stateObj).backTrack().current_depth(), 0);
  const DomainInt max = v_avr.getInitialMax();
  for(DomainInt curr = v_avr.getInitialMin(); curr <= max; curr++) { //build must have a value failure
    const depth curr_d =  v_avr.getDepth(curr);
    //TODO make this work with sparse variables
    if(curr_d >= decision_depth)
      at_curr_depth.push_back(make_pair(literal(false, v, curr), curr_d));
    else
      final.push_back(literal(false, v, curr));
  }
  //cout << "at_curr_depth=" << at_curr_depth << endl;
  //cout << "final=" << final << endl;
  make_heap(at_curr_depth.begin(), at_curr_depth.end(), depth_sort); //heap with largest at head
  //cout << "at_curr_depth=" << at_curr_depth << endl;
  while(at_curr_depth.size() > 1) { //until 1st uip reached
    pair<literal,depth> deepest = at_curr_depth.front();
    pop_heap(at_curr_depth.begin(), at_curr_depth.end(), depth_sort); 
    at_curr_depth.pop_back();
    const label& expl = get_AnyVarRef_from_Var(stateObj,
					       deepest.first.var).getLabel(deepest.first.val);
    //cout << "loop label=" << expl << endl;
    for(size_t i = 0; i < expl.size(); i++) {
      const literal& lit_i = expl[i];
      const depth& lit_i_d = get_AnyVarRef_from_Var(stateObj, lit_i.var).getDepth(lit_i.val);
      if(lit_i_d >= decision_depth) { //if at current depth put it into at_curr_depth
	//TODO replace heap with better data structure (find max & fast search too), probably
	if(find(at_curr_depth.begin(), at_curr_depth.end(), make_pair(lit_i, lit_i_d)) == at_curr_depth.end()) {
	  at_curr_depth.push_back(make_pair(lit_i, lit_i_d));
	  push_heap(at_curr_depth.begin(), at_curr_depth.end(), depth_sort);
	}
      } else
	final.push_back(lit_i);
    }
    //cout << "loop at_curr_depth=" << at_curr_depth << endl;
  }
  //cout << "at_curr_depth=" << at_curr_depth << endl;
  final.push_back(at_curr_depth.front().first); //add unique literal at current decision level
  sort(final.begin(), final.end()); //sort literals in preparation for removing duplicates
  vector<literal>::iterator unique_end = unique(final.begin(), final.end());
  final.erase(unique_end, final.end()); //kill dupes 
  cout << "final=" << final << endl;
  return final;
}

#endif
