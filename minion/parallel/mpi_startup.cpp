#include "protocol.h"
#include "mpi_singletons.h"
#include "../minion.h"
void minion_mpi_server_start(StateObj*);
void minion_mpi_client_start(StateObj*);

#include <iostream>

// This method just sets up basic MPI framework, then branches off to the server
// or client code as approriate
void go_parallel(int argc, char* argv[], StateObj* stateObj)
{
    initialise_mpi(argc, argv);
    boost::mpi::communicator world;
    if(world.rank() == 0)
    {
        for(int i = 0; i < argc; ++i)
            std::cout << argv[i] << ":";
        minion_mpi_server_start(stateObj);
    }
    else
        minion_mpi_client_start(stateObj);
    shutdown_mpi();
}