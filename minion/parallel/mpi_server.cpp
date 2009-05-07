#include "protocol.h"
#include "../minion.h"

#include <iostream>
#include "../inputfile_parse/ParsingObject.hpp"

#include <boost/iostreams/copy.hpp>
using namespace std;

struct MinionMPIServer
{
    boost::mpi::communicator world;
    
    int world_size;
    
    MinionMPIServer() : world_size(world.size())
    { ProtocolAssert(world.rank() == 0); }
    
    void init_world(string instance)
    {
        MinionMessage m;
        bool versions_consistent = true;
        for(int i = 1; i < world_size; ++i)
        {
            MINION_MPI_RECV(world, i, m);
            if (m != protocol_version)
            {
                cerr << "Instance " << i << " has invalid protocol version!" << endl;
                versions_consistent = false;
            }
        }
        
        if(!versions_consistent)
        {
            MinionMessage failm(NO_WAI);
            for(int i = 1; i < world_size; ++i)
                MINION_MPI_SEND(world, i, failm);
            cerr << "Bailing" << endl;
            exit(1);
        }
        
        for(int i = 1; i < world_size; ++i)
        {
            MINION_MPI_SEND(world, i, MinionMessage(WELCUM));
            MINION_MPI_SEND(world, i, MinionMessage(HAVE_FILEZ));
            MINION_MPI_SEND(world, i, instance);
        }
        
    }
    
    
    void ProtocolAssert(bool b)
    { 
        if (!b)
        {
            cerr << "NOWAI!" << endl; 
            abort();
        }
    }
};


void minion_mpi_server_start(StateObj* stateObj)
{
    ParsingObject po(getOptions(stateObj).instance_name,
                     getOptions(stateObj).parser_verbose);
                     
    ostringstream oss;
    boost::iostreams::copy(po.in, oss);
    
    cout << "Starting Server" << endl;
    MinionMPIServer server;
    server.init_world(oss.str());
    cout << "Server setup!" << endl;
    server.world.barrier();
    

    
    
    
}
