/*
 *  preprocess.cpp
 *  Minion
 *
 *  Created by Chris Jefferson on 28/10/2007.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#define NO_MAIN
#include "minion.h"

/// Apply a high level of consistency to a CSP.
/** This function is not particularly optimised, implementing only the most basic SAC and SSAC algorithms */
void preprocessCSP(StateObj* stateObj, MinionArguments::PreProcess preprocessLevel, vector<AnyVarRef>& vars)
{
  if(preprocessLevel != MinionArguments::None)
  {
    long long lits = lit_count(vars);
    bool bounds_check = (preprocessLevel == MinionArguments::SACBounds) ||
    (preprocessLevel == MinionArguments::SSACBounds);
    clock_t start_SAC_time = clock();
    PropagateSAC prop_SAC;
    prop_SAC(stateObj, vars, bounds_check);
    cout << "Preprocess Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
    cout << "Removed " << (lits - lit_count(vars)) << " literals" << endl;
    if(preprocessLevel == MinionArguments::SSAC || preprocessLevel == MinionArguments::SSACBounds)
    {
      lits = lit_count(vars);
      start_SAC_time = clock();
      PropagateSSAC prop_SSAC;
      prop_SSAC(stateObj, vars, bounds_check);
      cout << "Preprocess 2 Time: " << (clock() - start_SAC_time) / (1.0 * CLOCKS_PER_SEC) << endl;
      cout << "Removed " << (lits - lit_count(vars)) << " literals" << endl;
    }
  }
  
}
