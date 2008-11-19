
ConstraintDef constraint_list[] = {
{ "diseq" , CT_DISEQ, 2,{ read_var , read_var }, STATIC_CT },
{ "__reify_diseq" , CT_DISEQ_REIFY, 3,{ read_var , read_var , read_bool_var }, STATIC_CT },
{ "eq" , CT_EQ, 2,{ read_var , read_var }, STATIC_CT },
{ "__reify_eq" , CT_EQ_REIFY, 3,{ read_var , read_var , read_bool_var }, STATIC_CT },
{ "minuseq" , CT_MINUSEQ, 2,{ read_var , read_var }, STATIC_CT },
{ "__reify_minuseq" , CT_MINUSEQ_REIFY, 3,{ read_var , read_var , read_bool_var }, STATIC_CT },
{ "table" , CT_WATCHED_TABLE, 2,{ read_list , read_tuples }, DYNAMIC_CT },
{ "negativetable" , CT_WATCHED_NEGATIVE_TABLE, 2,{ read_list , read_tuples }, DYNAMIC_CT },
{ "gadget" , CT_GADGET, 1,{ read_list }, STATIC_CT },
{ "disabled-or" , CT_WATCHED_OR, 1,{ read_list }, DYNAMIC_CT },
{ "watched-or" , CT_WATCHED_NEW_OR, 1,{ read_constraint_list }, DYNAMIC_CT },
{ "w-literal" , CT_WATCHED_LIT, 2,{ read_var , read_constant }, DYNAMIC_CT },
{ "w-notliteral" , CT_WATCHED_NOTLIT, 2,{ read_var , read_constant }, DYNAMIC_CT },
{ "reify" , CT_REIFY, 2,{ read_constraint , read_bool_var }, DYNAMIC_CT },
{ "checkexpln" , CT_CHECKEXPLN, 1,{ read_list }, DYNAMIC_CT },
};
