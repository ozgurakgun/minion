#ifndef _LITERAL
#define _LITERAL

struct literal {
  AnyVarRef var; //variable
  int id; //variable ID
  int neg; //negation, 0 iff negated
  unsigned depth; //depth set
  DynamicConstraint* antecedent; //cause of instantiation (0 for search var)
  unsigned ant_seq_no; //sequence number of antecedent propagation (> is later)
};

#endif
