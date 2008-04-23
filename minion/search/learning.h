/* Minion Constraint Solver
 http://minion.sourceforge.net
 
 For Licence Information see file LICENSE.txt 
 
*/

#ifndef LEARNING
#define LEARNING

#include <vector>

//data structure to represent an assignment or non assignment
struct literal {
  literal(bool _asgn, VarIdent _var, DomainInt _val) : asgn(_asgn), var(_var), val(_val) {}
  bool asgn; //true iff it's an assignment. false iff it's a pruning
  VarIdent var;
  DomainInt val;
};

inline bool operator<(const literal& a, const literal& b) {
  return a.asgn < b.asgn ||
    (a.asgn == b.asgn && a.var < b.var) ||
    (a.asgn == b.asgn && a.var == b.var && a.val < b.val);
}

inline bool operator==(const literal& a, const literal& b) {
  return a.asgn == b.asgn && a.var == b.var && a.val == b.val;
}

inline ostream& operator<<(ostream& output, const literal& l) {
  output << "(asgn=" << l.asgn << ",var=" << l.var << ",val=" << l.val << ")";
  return output;
}

//data structure to represent a label for pruning/nogood
typedef vector<literal> label;

#endif
