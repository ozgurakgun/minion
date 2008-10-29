template<typename VarRef>
class Prun : public VirtCon {
  AbstractConstraint* con;
  VarRef var;
  DomainInt val;
  
  virtual vector<VirtConPtr> whyT() const
  {
    D_ASSERT(false);
    return vector<VirtConPtr>();
    //eventually: con->whyT(var, val)
  }

  virtual pair<unsigned, unsigned> getDepth() const
  {
    D_ASSERT(false);
    return make_pair(-1, -1);
    //eventually: var->getDepth(val)
  }

  virtual bool equals(const VirtCon& other) const
  {
    Prun* other_p = dynamic_cast<Prun*>(other);
    return other_p && var.getBaseVal(val) == other_p->var.getBaseVal(other_p->val) && 
      var.getBaseVar() == other_p->var.getBaseVar();
  }
};
