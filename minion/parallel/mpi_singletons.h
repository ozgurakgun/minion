#ifndef MPI_SINGLETONS_H_ALAL
#define MPI_SINGLETONS_H_ALAL

#include "protocol.h"

/// This must be called before any other MPI function
void initialise_mpi(int argc, char* argv[]);
void shutdown_mpi();

boost::mpi::environment* get_env();

#endif
