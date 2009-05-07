#include "protocol.h"
#include "mpi_singletons.h"

void minion_mpi_server_start();
void minion_mpi_client_start();

#include <iostream>

// This method just sets up basic MPI framework, then branches off to the server
// or client code as approriate
void go_parallel(int argc, char* argv[])
{
    initialise_mpi(argc, argv);
    boost::mpi::communicator world;
    if(world.rank() == 0)
    {
        for(int i = 0; i < argc; ++i)
            std::cout << argv[i] << ":";
        minion_mpi_server_start();
    }
    else
        minion_mpi_client_start();
    shutdown_mpi();
}