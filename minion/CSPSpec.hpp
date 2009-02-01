inline void inputPrint(std::ostream& o, StateObj* stateObj, const Var& v)
{ o << getState(stateObj).getVarContainer()->getName(v); }
