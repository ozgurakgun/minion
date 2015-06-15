#include "../minion.h"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/

#include "../constraints/constraint_mddc.h"

template <typename T>
AbstractConstraint*
BuildCT_NEGATIVEMDDC(const T& t1, ConstraintBlob& b)
{ return new MDDC<T, true>(t1, b.tuples); }

BUILD_CT(CT_NEGATIVEMDDC, 1)
