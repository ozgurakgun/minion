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
  literal(bool _asgn, Var _var, DomainInt _val) : asgn(_asgn), var(_var), val(_val) {}
  BOOL asgn; //true iff it's an assignment, false iff it's a pruning
  Var var;
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
//it's really a vector<literal> but I couldn't make that code compile
typedef vector<literal> label;

struct depth {
  depth() : d(-1), seq(-1) {} //default constructor
  depth(unsigned _d, unsigned _s) : d(_d), seq(_s) {}
  
  unsigned d; //depth set
  unsigned seq; //sequence number of action at depth d
};

inline bool operator==(const depth& d1, const depth& d2) {
  return d1.d == d2.d && d1.seq == d2.seq;
}
inline bool operator<(const depth& d1, const depth& d2) {
  return d1.d < d2.d || (d1.d == d2.d && d1.seq < d2.seq);
}
inline bool operator>(const depth& d1, const depth& d2) {
  return d2 < d1;
}
inline bool operator>=(const depth& d1, const depth& d2) {
  return d1 > d2 || d1 == d2;
}
inline ostream& operator<<(ostream& output, const depth& l) {
  output << "(d=" << l.d << ",seq=" << l.seq << ")";
  return output;
}
template<typename T, typename U>
inline ostream& operator<<(ostream& output, const pair<T,U> p) {
  output << "<" << p.first << "," << p.second << ">";
  return output;
}
template<typename T, typename U>
inline bool operator==(const pair<T,U>& p1, const pair<T,U>& p2) {
  return p1.first == p2.first && p1.second == p2.second;
}

#endif
