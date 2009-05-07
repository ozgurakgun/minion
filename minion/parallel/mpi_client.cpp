#include "protocol.h"

#include <iostream>
using namespace std;

struct MinionMPIClient
{
    boost::mpi::communicator world;
    
    MinionMPIClient()
    { ProtocolAssert(world.rank() != 0); }
    
    void init_world()
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


void minion_mpi_client_start()
{
    cout << "Starting client" << endl;
    MinionMPIClient client;
    client.init_world();
    cout << "client " << client.world.rank() << " setup!" << endl;
    
    client.world.barrier();
}
