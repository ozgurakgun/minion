template<typename T>
set<T> gather_variables(const set<set<T> >& edges)
{
	set<T> vars;
	for(typename set<set<T> >::iterator it = edges.begin(); it != edges.end(); ++it)
	{
    vars.insert(it->begin(), it->end());
	}
	
	return vars;
}

template<typename T>
set<T> gather_variables(const set<pair<T, T> >& edges)
{
	set<T> vars;
	for(typename set<pair<T, T> >::iterator it = edges.begin(); it != edges.end(); ++it)
	{
    vars.insert(it->first);
    vars.insert(it->second);
	}
	
	return vars;
}


set<set<Var> > build_hypergraph(const CSPInstance& csp)
{
	set<set<Var> > mset;
	for(list<ConstraintBlob>::const_iterator it = csp.constraints.begin(); it != csp.constraints.end(); ++it)
	{
	  set<Var> edge;
		for(int i = 0; i < it->vars.size(); ++i)
		{
			for(int j = 0; j < it->vars[i].size(); ++j)
			{ 
				if(it->vars[i][j].type != VAR_CONSTANT)
			  	edge.insert(it->vars[i][j]);
			}
		}
		mset.insert(edge);
	}
	return mset;
}

template<typename T>
set<pair<T, T> > make_graph_from_hypergraph(const set<set<T> >& hg)
{
	set<pair<T,T> > graph;
	for(typename set<set<T> >::iterator it = hg.begin(); it != hg.end(); it++)
	{
		for(typename set<T>::iterator v1 = it->begin(); v1 != it->end(); v1++)
		{
			typename set<T>::iterator v2 = v1;
			v2++;
			for(; v2 != it->end(); v2++)
			{
				if(*v1 < *v2)
				  graph.insert(make_pair(*v1, *v2));
				else
					graph.insert(make_pair(*v2, *v1));
			}	
		}
	}
	return graph;
}