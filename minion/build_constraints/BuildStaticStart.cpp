#include "BuildStart.h"
AbstractConstraint* build_constraint(StateObj* stateObj, ConstraintBlob& b) {
switch(b.constraint->type) {
case CT_WATCHED_NEQ : return build_constraint_CT_WATCHED_NEQ(stateObj, b);
case CT_DISEQ : return build_constraint_CT_DISEQ(stateObj, b);
case CT_DISEQ_REIFY : return build_constraint_CT_DISEQ_REIFY(stateObj, b);
case CT_EQ : return build_constraint_CT_EQ(stateObj, b);
case CT_EQ_REIFY : return build_constraint_CT_EQ_REIFY(stateObj, b);
case CT_MINUSEQ : return build_constraint_CT_MINUSEQ(stateObj, b);
case CT_MINUSEQ_REIFY : return build_constraint_CT_MINUSEQ_REIFY(stateObj, b);
case CT_WATCHED_LESS : return build_constraint_CT_WATCHED_LESS(stateObj, b);
case CT_WATCHED_TABLE : return build_constraint_CT_WATCHED_TABLE(stateObj, b);
case CT_WATCHED_NEGATIVE_TABLE : return build_constraint_CT_WATCHED_NEGATIVE_TABLE(stateObj, b);
case CT_GADGET : return build_constraint_CT_GADGET(stateObj, b);
case CT_WATCHED_OR : return build_constraint_CT_WATCHED_OR(stateObj, b);
case CT_WATCHED_NEW_OR : return build_constraint_CT_WATCHED_NEW_OR(stateObj, b);
case CT_WATCHED_LIT : return build_constraint_CT_WATCHED_LIT(stateObj, b);
case CT_WATCHED_NOTLIT : return build_constraint_CT_WATCHED_NOTLIT(stateObj, b);
case CT_REIFY : return build_constraint_CT_REIFY(stateObj, b);
case CT_CHECKEXPLN : return build_constraint_CT_CHECKEXPLN(stateObj, b);
default: D_FATAL_ERROR("Fatal error building constraints");
}}
