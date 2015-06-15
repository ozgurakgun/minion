#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/check_gsa.h"


inline AbstractConstraint*
BuildCT_CHECK_GSA(ConstraintBlob& bl)
{
  D_ASSERT(bl.internal_constraints.size() == 1);
  return checkGSACon(build_constraint(bl.internal_constraints[0]));
}

BUILD_CT(CT_CHECK_GSA, 0)
