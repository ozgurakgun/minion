/* Minion Constraint Solver
 http://minion.sourceforge.net
 
 For Licence Information see file LICENSE.txt 
 
*/

#ifndef LEARNING
#define LEARNING

#include <vector>
#include "../CSPSpec.h"

//data structure to represent an assignment or non assignment
struct literal {
  BOOL asgn; //true iff it's an assignment
  Var var;
  DomainInt val;
};

//data structure to represent a label for pruning/nogood
//it's really a vector<literal> but I couldn't make that code compile
typedef vector<literal> label;

#endif
