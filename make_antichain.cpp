#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

//This program produces minion instances which represent the 'antichain
//problem'. This problem can be described as follows. The <n,l,d> antichain
//problem involves finding n multisets consisting of values between 1 and l,
//each repeated up to d times, so that for any pair of multisets x and y,
//neither is a subset of the other.  

//Usage: antichain <num multisets> <num values> <max repeats>

int main(int argc, char** argv)
{
  int num_sets = atoi(argv[1]);
  int values = atoi(argv[2]);
  int reps = atoi(argv[3]);
  
  cout << "MINION 3\n" 
       << "#antichain<" << argv[1] << "," << argv[2] << "," << argv[3] << ">" << endl;
  
  cout << "**VARIABLES**\n";
  printf("DISCRETE ARRAY[%d,%d] {0..%d}\n", num_sets, values, reps-1);
  
  cout << "**SEARCH**" << endl;
  cout << "VARORDER [ARRAY]\n";
  
  cout << "**CONSTRAINTS**" << endl;
  for(int i = 0; i < num_sets; ++i) {
    for(int j = i + 1; j < num_sets; ++j) {
      printf("watched-or({watchless(ARRAY[%d,0],ARRAY[%d,0])", i, j);
      for(int k = 1; k < values; ++k)
	printf(",watchless(ARRAY[%d,%d],ARRAY[%d,%d])", i, k, j, k);
      printf("})\n");
      printf("watched-or({watchless(ARRAY[%d,0],ARRAY[%d,0])", j, i);
      for(int k = 1; k < values; ++k)
	printf(",watchless(ARRAY[%d,%d],ARRAY[%d,%d])", j, k, i, k);
      printf("})\n");
    }
  }
  
  cout << "**EOF**" << endl;
}
