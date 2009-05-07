#include "mpi_singletons.h"

boost::mpi::environment* env = NULL;
boost::mpi::communicator* world = NULL;


void initialise_mpi(int argc, char* argv[])
{
    assert(env == NULL && world == NULL);
    env = new boost::mpi::environment(argc, argv);
}

void shutdown_mpi()
    { delete env; }
    
boost::mpi::environment* get_env()
    { return env; }
