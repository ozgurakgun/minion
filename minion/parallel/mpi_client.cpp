#include "protocol.h"
#include "../minion.h"
#include "../inputfile_parse/ParsingObject.hpp"

#include <iostream>
#include <sstream>
using namespace std;

struct MinionMPIClient
{
    boost::mpi::communicator world;
    StateObj* stateObj;
    
    
    MinionMPIClient(StateObj* _stateObj) : stateObj(_stateObj)
    { ProtocolAssert(world.rank() != 0); }
    
    void init_world(SearchMethod& args)
    {
        MINION_MPI_SEND(world, 0, protocol_version);

        MinionMessage m;
        
        MINION_MPI_RECV(world, 0, m);
        ProtocolAssert(m.type == WELCUM);
        
        MINION_MPI_RECV(world, 0, m);
        ProtocolAssert(m.type == HAVE_FILEZ);
        
        string s;        
        MINION_MPI_RECV(world, 0, s);
        cout << "Got instance: " << s << endl;
        
        istringstream iss(s);
        ParsingObject p_obj(iss);
        CSPInstance instance = readInputFromStream(p_obj, "--", false);
        
        BuildCSP(stateObj, instance);
        SolveCSP(stateObj, instance, args);
    }
    
    
    void ProtocolAssert(bool b)
    { 
        if (!b)
        {
            cerr << "Problem. Bailing!" << endl; 
            abort();
        }
    }
};


void minion_mpi_client_start(StateObj* stateObj, SearchMethod& args)
{
    cout << "Starting client" << endl;
    MinionMPIClient client(stateObj);
    client.init_world(args);
    cout << "client " << client.world.rank() << " setup!" << endl;
    
    client.world.barrier();
}
