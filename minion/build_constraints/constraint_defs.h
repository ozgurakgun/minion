
ConstraintDef constraint_list[] = {
{ "element" , CT_ELEMENT, 2 ,{ read_list , read_var }, STATIC_CT },
{ "watchelement" , CT_WATCHED_ELEMENT, 2 ,{ read_list , read_var }, DYNAMIC_CT },
{ "gacelement" , CT_GACELEMENT, 2 ,{ read_list , read_var }, STATIC_CT },
{ "alldiff" , CT_ALLDIFF, 1 ,{ read_list }, STATIC_CT },
{ "alldiffgacslow" , CT_ALLDIFF_GACSLOW, 1 ,{ read_list }, STATIC_CT },
{ "diseq" , CT_DISEQ, 2 ,{ read_var , read_var }, STATIC_CT },
{ "eq" , CT_EQ, 2 ,{ read_var , read_var }, STATIC_CT },
{ "ineq" , CT_INEQ, 3 ,{ read_var , read_var , read_constant }, STATIC_CT },
{ "lexleq" , CT_LEXLEQ, 2 ,{ read_list , read_list }, STATIC_CT },
{ "lexless" , CT_LEXLESS, 2 ,{ read_list , read_list }, STATIC_CT },
{ "max" , CT_MAX, 2 ,{ read_list , read_var }, STATIC_CT },
{ "min" , CT_MIN, 2 ,{ read_list , read_var }, STATIC_CT },
{ "occurrence" , CT_OCCURRENCE, 3 ,{ read_list , read_constant , read_var }, STATIC_CT },
{ "occurrenceleq" , CT_LEQ_OCCURRENCE, 3 ,{ read_list , read_constant , read_var }, STATIC_CT },
{ "occurrencegeq" , CT_GEQ_OCCURRENCE, 3 ,{ read_list , read_constant , read_var }, STATIC_CT },
{ "product" , CT_PRODUCT2, 2 ,{ read_2_vars , read_var }, STATIC_CT },
{ "weightedsumleq" , CT_WEIGHTLEQSUM, 3 ,{ read_constant_list , read_list , read_var }, STATIC_CT },
{ "weightedsumgeq" , CT_WEIGHTGEQSUM, 3 ,{ read_constant_list , read_list , read_var }, STATIC_CT },
{ "sumgeq" , CT_GEQSUM, 2 ,{ read_list , read_var }, STATIC_CT },
{ "sumleq" , CT_LEQSUM, 2 ,{ read_list , read_var }, STATIC_CT },
{ "watchsumgeq" , CT_WATCHED_GEQSUM, 2 ,{ read_list , read_constant }, DYNAMIC_CT },
{ "watchsumleq" , CT_WATCHED_LEQSUM, 2 ,{ read_list , read_constant }, DYNAMIC_CT },
{ "table" , CT_WATCHED_TABLE, 2 ,{ read_list , read_tuples }, DYNAMIC_CT },
{ "watchvecneq" , CT_WATCHED_VECNEQ, 2 ,{ read_list , read_list }, DYNAMIC_CT },
{ "minuseq" , CT_MINUSEQ, 2 ,{ read_var , read_var }, STATIC_CT },
{ "litsumgeq" , CT_WATCHED_LITSUM, 3 ,{ read_list , read_constant_list , read_constant }, DYNAMIC_CT },
{ "pow" , CT_POW, 2 ,{ read_2_vars , read_var }, STATIC_CT },
{ "div" , CT_DIV, 2 ,{ read_2_vars , read_var }, STATIC_CT },
{ "modulo" , CT_MODULO, 2 ,{ read_2_vars , read_var }, STATIC_CT },
{ "reify", CT_REIFY, 0, {}, STATIC_CT },
{ "reifyimply", CT_REIFYIMPLY, 0, {}, STATIC_CT },
};
