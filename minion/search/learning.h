/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

#include "../system/linked_ptr.h"

#ifndef EXPLANATIONS_HEADER
#define EXPLANATIONS_HEADER

class Explanation {

};

//explanations are stored as garbage collected pointers
typedef shared_ptr<Explanation> ExplPtr;

#endif
