
template<typename T>
pair<set<T>, set<T> > partition_vertices(const set<pair<T, T> >& edgeset, const set<T>& part)
{
  set<T> first;
  
  // prime split
  
  typename set<pair<T, T> >::iterator it = edgeset.begin();
  
  while(first.empty())
  {
    if(!part.count(it->first))
      first.insert(it->first);
    if(!part.count(it->second))
      first.insert(it->second);
    ++it;
  }
  
  int initial_first_size = -1;
  
  while(initial_first_size != first.size())
  {
    initial_first_size = first.size();
    
    for(typename set<pair<T, T> >::iterator it = edgeset.begin(); it != edgeset.end(); ++it)
    {
      if(first.count(it->first) || first.count(it->second))
      {
        if(!part.count(it->first) && !part.count(it->second))
        {
          first.insert(it->first);
          first.insert(it->second);
        } 
      }
    }
  }
  
  set<T> second;
  
  for(typename set<pair<T, T> >::iterator it = edgeset.begin(); it != edgeset.end(); ++it)
  {
    if(!first.count(it->first) && !part.count(it->first))
      second.insert(it->first);
    if(!first.count(it->second) && !part.count(it->second))
      second.insert(it->second);
  }
  
  return make_pair(first, second);
}


template<typename T>
set<pair<T, T> > project_edges(const set<pair<T, T> >& edges, set<T> vertices)
{
  set<pair<T, T> > ret;
  for(typename set<pair<T, T> >::iterator it = edges.begin(); it != edges.end(); ++it)
  {
    if(vertices.count(it->first) && vertices.count(it->second))
      ret.insert(*it);
  }
  return ret;
}
