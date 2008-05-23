	namespace boost
	{
	  struct edge_component_t
	  {
	    enum
	    { num = 555 };
	    typedef edge_property_tag kind;
	  }
	  edge_component;
	}
	
	vector<int>
	find_articulation_points(const set<pair<int, int> >& edgelist)
	{
	  using namespace boost;
	  typedef adjacency_list < vecS, vecS, undirectedS,
	    no_property, property < edge_component_t, std::size_t > >graph_t;
	  typedef graph_traits < graph_t >::vertex_descriptor vertex_t;
    graph_t g;
	
		for(set<pair<int, int> >::iterator it = edgelist.begin(); it != edgelist.end(); ++it)
	  {
			add_edge(it->first, it->second, g);
	  }

	  property_map < graph_t, edge_component_t >::type
	    component = get(edge_component, g);

//	  std::size_t num_comps = biconnected_components(g, component);
//	  std::cerr << "Found " << num_comps << " biconnected components.\n";

	  std::vector<int> art_points;
	  articulation_points(g, std::back_inserter(art_points));
	//  std::cerr << "Found " << art_points.size() << " articulation points.\n";
		return art_points;
	/*  std::cout << "graph A {\n" << "  node[shape=\"circle\"]\n";

	  for (std::size_t i = 0; i < art_points.size(); ++i) {
	    std::cout << (char)(art_points[i] + 'A') 
	              << " [ style=\"filled\", fillcolor=\"red\" ];" 
	              << std::endl;
	  }

	  graph_traits < graph_t >::edge_iterator ei, ei_end;
	  for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
	    std::cout << (char)(source(*ei, g) + 'A') << " -- " 
	              << (char)(target(*ei, g) + 'A')
	              << "[label=\"" << component[*ei] << "\"]\n";
	  std::cout << "}\n";*/

	}
	
		vector<set<pair<int, int> > >
  	find_biconnected_components(const set<pair<int, int> >& edgelist)
  	{
  	  using namespace boost;
  	  typedef adjacency_list < vecS, vecS, undirectedS,
  	    no_property, property < edge_component_t, std::size_t > >graph_t;
  	  typedef graph_traits < graph_t >::vertex_descriptor vertex_t;
      graph_t g;

  		for(set<pair<int, int> >::iterator it = edgelist.begin(); it != edgelist.end(); ++it)
  	  {
  			add_edge(it->first, it->second, g);
  	  }

  	  property_map < graph_t, edge_component_t >::type
  	    component = get(edge_component, g);

  	  std::size_t num_comps = biconnected_components(g, component);
  	  std::cout << "Found " << num_comps << " biconnected components.\n";

  	  std::vector<int> art_points;
  	  articulation_points(g, std::back_inserter(art_points));
  	//  std::cerr << "Found " << art_points.size() << " articulation points.\n";
//  		return art_points;
 // 	  std::cout << "graph A {\n" << "  node[shape=\"circle\"]\n";

 // 	  for (std::size_t i = 0; i < art_points.size(); ++i) {
 // 	    std::cout << (char)(art_points[i] + 'A') 
 // 	              << " [ style=\"filled\", fillcolor=\"red\" ];" 
 // 	              << std::endl;
  //	  }

  	  graph_traits < graph_t >::edge_iterator ei, ei_end;
      vector<set<pair<int, int> > > sections(edgelist.size());
  	  for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
  	  {
        int i = component[*ei];
        sections[i].insert(make_pair(source(*ei, g), target(*ei, g)));
  	  }
      return sections;
  	  
 /* 	    std::cout << (char)(source(*ei, g) + 'A') << " -- " 
  	              << (char)(target(*ei, g) + 'A')
  	              << "[label=\"" << component[*ei] << "\"]\n";
  	  std::cout << "}\n";*/

  	}
