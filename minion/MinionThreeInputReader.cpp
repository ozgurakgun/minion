/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: MinionThreeInputReader.cpp 615 2007-07-17 12:48:20Z azumanga $
*/

// MinionThreeInputReader.cpp
//
// Subversion Identity $Id: MinionThreeInputReader.cpp 615 2007-07-17 12:48:20Z azumanga $
//
// Plan here is to generate an instance of a problem (or whatever you have)
// and return that.

#define NO_MAIN

#include <string>
#include "system/system.h"

#include "CSPSpec.h"
using namespace ProbSpec;

#include "MinionInputReader.h"

#include "build_constraints/constraint_defs.h"
int num_of_constraints = sizeof(constraint_list) / sizeof(ConstraintDef);


ConstraintDef& get_constraint(ConstraintType t)
{
  for(int i = 0; i < num_of_constraints; ++i)
  {
    if(constraint_list[i].type == t)
	  return constraint_list[i];
  }
  
  D_FATAL_ERROR("Constraint not found");
}

template<typename T>
vector<T> make_vec(const T& t)
{
  vector<T> vec(1);
  vec[0] = t;
  return vec;
}

template<typename T>
typename T::value_type& index(T& container, int index_pos)
{
  if(index_pos < 0 || index_pos >= (int)container.size())
    throw parse_exception("Index position " + to_string(index_pos) + 
							  " out of range");
  return container[index_pos];
}

void MinionThreeInputReader::parser_info(string s)
{
  if(parser_verbose)
    cout << s << endl;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// read
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionThreeInputReader::read(InputFileReader* infile) {  

    
    string s = infile->get_asciistring();
    parser_info("Read: '" + s + "'");
    while(s != "**EOF**")
    {
      if(s == "**VARIABLES**")
        readVars(infile);
      else if(s == "**SEARCH**")
        readSearch(infile);
      else if(s == "**TUPLELIST**")
        readTuples(infile);
      else if(s =="**CONSTRAINTS**")
      {
        while(infile->peek_char() != '*')
          readConstraint(infile, false);
      }
      else if(s == "**GADGET**")
      { readGadget(infile); }
      else
        throw parse_exception("Don't understand '" + s + "'");
      s = infile->get_asciistring();
      parser_info("Read: '" + s + "'");
    }


  
  // Fill in any missing defaults
  if(instance.var_order.empty())
  {
    parser_info("No order generated, auto-generating complete order");
    int var_count = 0;
    var_count += instance.vars.BOOLs;
    for(unsigned i = 0; i < instance.vars.bound.size(); ++i)
      var_count += instance.vars.bound[i].first;
    for(unsigned i = 0; i < instance.vars.sparse_bound.size(); ++i)
      var_count += instance.vars.sparse_bound[i].first;
    for(unsigned i = 0; i < instance.vars.discrete.size(); ++i)
      var_count += instance.vars.discrete[i].first;
    for(unsigned i = 0; i < instance.vars.sparse_discrete.size(); ++i)
      var_count += instance.vars.sparse_discrete[i].first;
    
    instance.var_order.reserve(var_count);
    for(int i = 0; i < var_count; ++i)
      instance.var_order.push_back(instance.vars.get_var('x',i));
  }
  
  if(instance.val_order.empty())
    instance.val_order = vector<char>(instance.var_order.size(), 'a');
  
  // This has to be delayed unless not all variables are defined yet.
  if(print_all_vars)
    instance.print_matrix = make_vec(instance.vars.get_all_vars());
}

struct Gadget
{
  string name;
  int varCount;
  vector<int> construction;
};


void MinionThreeInputReader::readGadget(InputFileReader* infile)
{
  Gadget g;
  string s;
  
  s = infile->get_string();
  if(s != "NAME")
    throw parse_exception("Expected 'NAME', recieved :"+s);
  g.name = infile->get_string();
  
  s = infile->get_string();
  if(s != "VARCOUNT")
    throw parse_exception("Expected 'VARCOUNT', recieved :"+s);
  g.varCount = infile->read_num();
  
  s = infile->get_string();
  if(s != "CONSTRUCTION_SITE")
    throw parse_exception("Expected 'CONSTRUCTION_SITE', recieved :"+s);
  g.construction = getConstantVector();
  
  while(infile->peek_char() == '*')
  {
    
    
  }
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraint
// Recognise constraint by its name, read past name and leading '('
// Return false if eof or unknown ct. Else true.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
BOOL MinionThreeInputReader::readConstraint(InputFileReader* infile, BOOL reified) {
  string id = infile->getline('(');
  
  int constraint_num = -1;
  for(int i = 0; i < num_of_constraints; ++i)
  {
	if(constraint_list[i].name == id)
	{
	  constraint_num = i;
	  break;
	}
  }
  
  if(constraint_num == -1) 
  {
	if (infile->eof()) 
	{
	  throw parse_exception(string("Bad Constraint Name or reached end of file: '") + id + "'");
	}
	else
	{ throw parse_exception(string("Unknown Constraint:") + id); }
  }
  ConstraintDef constraint = constraint_list[constraint_num];
 
  if( constraint.trig_type == DYNAMIC_CT )
  {
#ifndef WATCHEDLITERALS
	cerr << "This version of Minion was not complied with -WATCHEDLITERALS" << endl;
	cerr << "So there is not support for the " << constraint.name << "." << endl;
	exit(1);
#else
	if(reified)
	{
	  cerr << "Cannot reify a watched constraint!" << endl;
	  exit(1);
	}
#endif
  }

  switch(constraint.type)
  {
	case CT_ELEMENT:
	case CT_WATCHED_ELEMENT:
	case CT_GACELEMENT:
	  readConstraintElement(infile, constraint) ;
	  break;
	case CT_REIFY:
	case CT_REIFYIMPLY:
	  { 
	  if(reified)
		throw parse_exception("Can't reify a reified constraint!");
	  readConstraint(infile, true);
	  
	  infile->check_sym(',');
	  Var reifyVar = readIdentifier(infile);
	  infile->check_sym(')');
	  if(constraint.type == CT_REIFY)
	    instance.last_constraint_reify(reifyVar);
	  else
	    instance.last_constraint_reifyimply(reifyVar);
	  }
	  break;

	case CT_WATCHED_TABLE:
	  readConstraintTable(infile, get_constraint(CT_WATCHED_TABLE));
	  break;

	default:
	  readGeneralConstraint(infile, constraint);
  }
  
  instance.bounds_check_last_constraint();
  return true ;
}


void MinionThreeInputReader::readGeneralConstraint(InputFileReader* infile, const ConstraintDef& def)
{
  // This slightly strange code is to save copying the ConstraintBlob as much as possible.
  instance.add_constraint(ConstraintBlob(def));
  vector<vector<Var> >& varsblob = instance.constraints.back().vars;
  varsblob.reserve(def.number_of_params);
  
  for(int i = 0; i < def.number_of_params; ++i)
  {
    switch(def.read_types[i])
	{
	  case read_list:
	    varsblob.push_back(readVectorExpression(infile));
		break;
	  case read_var:
	    varsblob.push_back(make_vec(readIdentifier(infile)));
		break;
	  case read_2_vars:
	  {
	    vector<Var> vars(2);
	    vars[0] = readIdentifier(infile);
	    infile->check_sym(',');
	    vars[1] = readIdentifier(infile);
            varsblob.push_back(vars);
	  }
		break;
	  case read_constant:
	    varsblob.push_back(make_vec(readIdentifier(infile)));
		if(varsblob.back().back().type != VAR_CONSTANT)
		  throw parse_exception("Expected constant but got variable.");
		break;
	  case read_constant_list:
	  {
		vector<Var> vectorOfConst ;
		vectorOfConst = readVectorExpression(infile) ;
		for(unsigned int loop = 0; loop < vectorOfConst.size(); ++loop)
		{
		  if(vectorOfConst[loop].type != VAR_CONSTANT)
			throw parse_exception("Vector must only contain constants.");
		}
		varsblob.push_back(vectorOfConst);
	  }
		break;  
	  default:
	    D_FATAL_ERROR("Internal Error!");
	}
	if(i != def.number_of_params - 1)
	  infile->check_sym(',');
  }
  infile->check_sym(')');
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintElement
// element(vectorofvars, indexvar, var)
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionThreeInputReader::readConstraintElement(InputFileReader* infile, const ConstraintDef& ctype) {
  parser_info("reading an element ct. " ) ;
  vector<vector<Var> > vars;
  // vectorofvars
  vars.push_back(readVectorExpression(infile));
  infile->check_sym(',');
  // indexvar
  vars.push_back(make_vec(readIdentifier(infile)));
  infile->check_sym(',');
  // The final var is shoved on the end of the vector of vars as it should
  // be of a similar type.
  // final var
  vars[0].push_back(readIdentifier(infile));
  infile->check_sym(')');
  instance.add_constraint(ConstraintBlob(ctype, vars));
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readConstraintTable
// table(<vectorOfVars>, {<tuple> [, <tuple>]})
// Tuples represented as a vector of int arrays.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionThreeInputReader::readConstraintTable(InputFileReader* infile, const ConstraintDef& def) 
{
  parser_info( "reading a table ct (unreifiable)" ) ;
  
  char delim = ' ';
  int count, elem ;
  vector<Var> vectorOfVars = readVectorExpression(infile) ;
  int tupleSize = vectorOfVars.size() ;
  
  infile->check_sym(',');
  
  char next_char = infile->peek_char();
  
  
  TupleList* tuplelist;
  
  if(next_char != '{')
  {
    string name = infile->get_string();
    tuplelist = instance.getTableSymbol(name);
  }
  else
  {
	vector<vector<int> > tuples ;
	infile->check_sym('{');
	while (delim != '}') 
	{
	  infile->check_sym('<');
	  vector<int> tuple(tupleSize);
	  elem = infile->read_num() ;
	  tuple[0] = elem ;
	  for (count = 1; count < tupleSize; count++) 
	  {
		infile->check_sym(',');
		elem = infile->read_num() ;
		tuple[count] = elem ;
	  }
	  infile->check_sym('>');
	  tuples.push_back(tuple) ;
	  delim = infile->get_char();                          // ',' or '}'
	  if(delim != ',' && delim!= '}')
		throw parse_exception("Expected ',' or '}'");
	}
	tuplelist = tupleListContainer->getNewTupleList(tuples);
  }
	
	infile->check_sym(')');
	ConstraintBlob tableCon(def, vectorOfVars);
	tableCon.tuples = tuplelist;
	instance.add_constraint(tableCon);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readIdentifier
// Expects "<idChar><index>", where <idChar> is 'x', 'v', 'm', 't'.
// Assumes caller knows what idChar should be.
// Returns an object of type Var.
// NB peek() does not ignore whitespace, >> does. Hence use of putBack()
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
Var MinionThreeInputReader::readIdentifier(InputFileReader* infile) {
  char idChar = infile->peek_char();
  
  if ((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    int i = infile->read_num();
	return Var(VAR_CONSTANT, i);
  }
  
  string name = infile->get_string();
  
  Var var = instance.vars.getSymbol(name);
  if(var.type == VAR_MATRIX)
  {
    vector<int> params = readConstantVector(infile,'[',']');
    vector<int> max_index = instance.vars.getMatrixSymbol(name);
    if(params.size() != max_index.size())
      throw parse_exception("Can't index a " + to_string(max_index.size()) + 
                            "-d matrix with " + to_string(params.size()) +
                            " indices.");
    for(int i = 0; i < params.size(); ++i)
    {
      if(params[i] < 0 || params[i] >= max_index[i])
        throw parse_exception(to_string(i) + string("th index is invalid"));
    }
    name += to_string(params);
    var = instance.vars.getSymbol(name);
  }
  parser_info("Read variable '" + name + "', internally: " + to_string(var));
  return var;
}



// This function reads an identifier which might be a single variable,
// which includes a fully derefenced matrix, or might be a partially or
// not-at-all dereferenced matrix. It could also just be a number!
// The code shares a lot with readIndentifier, and at some point the two
// should probably merge
vector<Var> MinionThreeInputReader::readPossibleMatrixIdentifier(InputFileReader* infile) {
  char idChar = infile->peek_char();
  
  vector<Var> returnVec;
  if ((('0' <= idChar) && ('9' >= idChar)) || idChar == '-') {
    int i = infile->read_num();
	returnVec.push_back(Var(VAR_CONSTANT, i));
    return returnVec;
  }
  
  string name = infile->get_string();
  
  Var var = instance.vars.getSymbol(name);
  if(var.type == VAR_MATRIX)
  {
    vector<int> params;
    if(infile->peek_char() == '[')
      params = readConstantVector(infile,'[',']',true);
    else
    { // build a vector of all 'nulls'
      vector<int> maxterms = instance.vars.getMatrixSymbol(name);
      params = vector<int>(maxterms.size(), -999);
    }
    returnVec = instance.vars.buildVarList(name, params);
    parser_info("Got matrix:" + to_string(returnVec));
  }
  else
  { 
    returnVec.push_back(var);
  }
  parser_info("Read variable '" + name + "', internally: " + to_string(var));
  return returnVec;  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readLiteralVector
// of vars or consts. Checks 1st elem of vect (empty vects not expected)
//  to see which.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionThreeInputReader::readLiteralVector(InputFileReader* infile) {
  vector<Var> newVector;
  infile->check_sym('[');
 
  // Delim here might end up being "x" or something similar. The reason
  // that we peek it is in case whis is an empty vector.
  
  char delim = infile->peek_char();
    	
  if(delim == ']')
  {
    // Eat the ']'
    infile->get_char();
    parser_info("Read empty vector.");
  }
  else
  {
    while (delim != ']') {
      vector<Var> v = readPossibleMatrixIdentifier(infile);
	  newVector.insert(newVector.end(), v.begin(), v.end());
      //newVector.push_back(readIdentifier(infile)) ;
	  delim = infile->get_char();
	     if(delim != ',' && delim != ']')
	     {
		   // replace X with the character we got.
		   string s = "Expected ',' or ']'. Got 'X'.";
		   s[s.size() - 3] = delim;
		   throw parse_exception(s);
	     }
      }
  }
  return newVector;
}

// Note: allowNulls maps '_' to -999 (a horrible hack I know).
// That last parameter defaults to false.
vector<int> MinionThreeInputReader::readConstantVector
          (InputFileReader* infile, char start, char end, bool allowNulls) 
{
  vector<int> newVector;
  infile->check_sym(start);
  
  // The reason we peek here is in case this is an empty vector
  char delim = infile->peek_char();
  
  if(delim == end)
  {
    // Eat the ']'
    infile->get_char();
    parser_info("Read empty vector.");
  }
  else
  {
    while (delim != end) 
    {
      if(allowNulls && infile->peek_char() == '_')
      {
        infile->get_char();
        newVector.push_back(-999);
      }
      else
	    newVector.push_back(infile->read_num()) ;
	  delim = infile->get_char();
      if(delim != ',' && delim != end)
       throw parse_exception(string("Expect ',' or ") + end + string("'. Got '") +
                             delim + string("'"));
    }
  }
  return newVector;
}

/// Read an expression of the type ' {<num>..<num>} '
vector<int> MinionThreeInputReader::readRange(InputFileReader* infile) 
{
  vector<int> newVector;
  infile->check_sym('{');
  
  newVector.push_back(infile->read_num());
  infile->check_sym('.');
  infile->check_sym('.');
  
  newVector.push_back(infile->read_num());
  
  infile->check_sym('}');
  return newVector;
}



void MinionThreeInputReader::readTuples(InputFileReader* infile)
{
  while(infile->peek_char() != '*')
  {
    string name = infile->get_string();
    int num_of_tuples = infile->read_num();
	int tuple_length = infile->read_num();
    parser_info("Reading tuplelist '" + name + "', length " + to_string(num_of_tuples) +
                ", arity " + to_string(tuple_length) );
	TupleList* tuplelist = tupleListContainer->getNewTupleList(num_of_tuples, tuple_length);
    int* tuple_ptr = tuplelist->getPointer();
    for(int i = 0; i < num_of_tuples; ++i)
      for(int j = 0; j < tuple_length; ++j)
	  {
	    tuple_ptr[i * tuple_length + j] = infile->read_num();
	  }
    tuplelist->finalise_tuples();
    instance.addTableSymbol(name, tuplelist);
  }
  
}

void MinionThreeInputReader::readSearch(InputFileReader* infile) {  
  while(infile->peek_char() != '*')
  {
    string var_type = infile->get_string();
   
    if(var_type == "VARORDER")
    {
      if(!instance.var_order.empty())
        throw parse_exception("Can't have two VARORDERs!");
      instance.var_order = readLiteralVector(infile);
      parser_info("Read var order, length " +
                  to_string(instance.var_order.size()));
    }
    else if(var_type == "VALORDER")
    {
      if(!instance.val_order.empty())
        throw parse_exception("Can't have two VALORDERs!");
      vector<char> valOrder ;
      
      infile->check_sym('[');
      
      char delim = infile->peek_char();
       
      while (delim != ']') {
        char valOrderIdentifier = infile->get_char();
        if(valOrderIdentifier != 'a' && valOrderIdentifier != 'd')
          throw parse_exception("Expected 'a' or 'd'");
        valOrder.push_back(valOrderIdentifier == 'a');
        delim = infile->get_char();                                 // , or ]
      }
      instance.val_order = valOrder;
      
      parser_info("Read val order, length " +
                  to_string(instance.val_order.size()));
    }
    else if(var_type == "MAXIMISING" || var_type == "MAXIMIZING")
    {
      if(instance.is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      parser_info("Maximising " + to_string(var));
      instance.set_optimise(true, var);
    }
    else if(var_type == "MINIMISING" || var_type == "MINIMIZING")
    {
      if(instance.is_optimisation_problem == true)
        throw parse_exception("Can only have one min / max per problem!");

      Var var = readIdentifier(infile);
      parser_info("Minimising " + to_string(var));
      instance.set_optimise(false, var);
    }
    else if(var_type == "PRINT")
    {
      if(!instance.print_matrix.empty())
        throw parse_exception("Can only have one PRINT statement!");
      if(infile->peek_char() == 'A')
      {
        string in = infile->get_string();
        if(in != "ALL")
          throw parse_exception("Don't understand '"+in+"'. Do you mean 'ALL'?");
        print_all_vars = true;
      }
      else if(infile->peek_char() == 'N')
      {
        string in = infile->get_string();
        if(in != "NONE")
          throw parse_exception("Don't understand '"+in+"'. Do you mean 'NONE'?");
        print_all_vars = false;
      }
      else
      {
        print_all_vars = false;
        instance.print_matrix = make_vec(readLiteralVector(infile));
      }
    }
    else
    {  throw parse_exception("Don't understand '" + var_type + "'"); }
  }
  

  
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionThreeInputReader::readVars(InputFileReader* infile) {
  while(infile->peek_char() != '*')
  {
    string var_type = infile->get_string();

    if(var_type != "BOOL" && var_type != "BOUND" && var_type != "SPARSEBOUND"
       && var_type != "DISCRETE")
      throw parse_exception(string("Unknown variable type: '") + var_type + "'");

    string varname = infile->get_string();
    parser_info("Name:" + varname);
      
    bool isArray = false;
    vector<int> indices;
      
    if(infile->peek_char() == '[')
    {
      parser_info("Is array!");
      isArray = true;
      indices = readConstantVector(infile,'[',']');
      parser_info("Found " + to_string(indices.size()) + " indices");
    }
    
    VariableType variable_type;
    vector<int> domain;
    
    if(var_type == "BOOL")
    {
      variable_type = VAR_BOOL;
    }
    else if(var_type == "BOUND")
    {
      variable_type = VAR_BOUND;
      domain = readRange(infile);
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "DISCRETE")
    {
      variable_type = VAR_DISCRETE_BASE;
      domain = readRange(infile);
      if(domain.size() != 2)
        throw parse_exception("Ranges contain 2 numbers!");
    }
    else if(var_type == "SPARSEBOUND")
    {
      variable_type = VAR_SPARSEBOUND;
      domain = readConstantVector(infile, '{', '}');
      if(domain.size() < 1)
        throw parse_exception("Don't accept empty domains!");
    }
    else
      throw parse_exception("I don't know about var_type '" + var_type + "'");
      
    if(isArray)
    {
      instance.vars.addMatrixSymbol(varname, indices);
      vector<int> current_index(indices.size(), 0);
      parser_info("New Var: " + varname + to_string(current_index));
      instance.vars.addSymbol(varname + to_string(current_index),
                              instance.vars.getNewVar(variable_type, domain));
      while(increment_vector(current_index, indices))
      {
        parser_info("New Var: " + varname + to_string(current_index));
        instance.vars.addSymbol(varname + to_string(current_index),
                                instance.vars.getNewVar(variable_type, domain));
      }
    }
    else
    {
      instance.vars.addSymbol(varname,
                              instance.vars.getNewVar(variable_type, domain));
    }
  }
  
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// readVectorExpression
// literal vector (of vars or consts), vi, mi(flattened), ti(flattened),
// row(mi, r), col(mi, c), col(ti, p, c), rowx(ti, p, r), rowz(ti, r, c)
// NB Expects caller knows whether vars or consts expected for lit vect.
// NB peek does not ignore wspace, >> does. Hence use of putback
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
vector<Var> MinionThreeInputReader::readVectorExpression(InputFileReader* infile) {
      parser_info( "Reading Literal Vector of vars or consts" ) ;
      return readLiteralVector(infile) ;      
}



