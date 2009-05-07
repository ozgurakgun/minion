#ifndef _PROTOCOL_H_TYOP
#define _PROTOCOL_H_TYOP

/* The following file describes the minion MPI protocol version 1.
 * It is intended this protocol will be implemented using boost::MPI
 * and is designed to be quite fragile, but optimised.
 *
 * A basic outline of the protocol:
 *
 * One machine is the server. On MPI this is machine '0'. It does not do 
 * any solving, but only deals with giving out work. All other machines 
 * are clients.
 *
 * We assume we will only solve 1 minion instance per execution.
 *
 *
 *
 * An outline of the protocol is as follows. The details of how to message pass
 * are further down, or in the code!
 *
 * 1) All clients -> Server. 'O_HAI' + version number.
 * 2) If version numbers conflict, Server -> all clients 'NO_WAI', abort
 * 3) If version numbers OK, Server -> all clients 'WELCUM'
 * 4) Server -> all clients 'HAVE_FILEZ' message, which provides the instance.
 *
 * The clients then enter the following cycle.
 *
 * 1) Wait until a 'HAVE_WURK' message arrives, providing a search, then do it.
 * 2) While working, any of the following messages may arrive:
 *   'IZ_IN_UR_BASE_STEALIN_UR_WURK' may arrive - pass back to server either:
 *       'GIVEZ_TREEZ', some work another machine could do.
         'PLZ_SEND_WURK' if there is no work (because the client has finished).
         'OUT_PLZ' if search is finished. Exit gracefully.
         
 * 3) If a solution is found, send 'HAS_SOLUTIONZ' to the server.
 * 4) If the client runs out of work, send 'PLZ_SEND_WURK' to the server.
 *
 * Clients should never recieve 'HAVE_WURK' while already operating. If they do
 * they should send 'NO_WAI' and abort. 
 *
 * A server's task is more complicated, but basically it sends HAS_WORKZ messages
 * to get work for clients which currently have nothing to do.
 * 
 * In general messages take the following form:
 * A 'Message' object which has a type, and up to 3 integers for information.
 * Then another C++ object, which will change depending on the Message.
*/

  enum MessageType
  {
      // This just provides something to initialise a message object to.
      // It should never be sent or received.
      INVALID_MESSAGE,
      
      // Clients send this when search starts.
      // info_a, info_b and info_c denote protocol version.
      O_HAI,
      
      // Servers sends this to clients as a first message, if all clients
      // responding with the correct protocol version.
      WELCUM,
      
      // A general error state. Whenever the server or client receives this, it
      // should abort quickly. Something has gone badly wrong.
      NO_WAI,
      
      // The instance file will be transmitted. Only from server to client.
      // Straight afterwards a std::string with the instance is sent.
      HAVE_FILEZ,
      
      // A more graceful error. Should only be sent by servers to clients at the
      // end of search.
      OUT_PLZ,
      
      // Passes work, either server->client or client->server. If a client receives 
      // this while it is already working it should return NO_WAI and abort. 
      // After this, a Branch object will be sent describing the branch to take
      HAVE_WURK,
      
      // Asks a client for work, to which it should (generally) return it's top-most
      // branch, which it will then not search. If it is already out of work, it should
      // return PLZ_SEND_WURK
      IZ_IN_UR_BASE_STEALIN_UR_WURK,
      
      // Denotes a client has no work. Can be sent at any time, in particular in reply
      // to IZ_IN_UR_BASE_STEALIN_UR_WORK. Sending this multiple times is not a mistake,
      // but not encourage.
      PLZ_SEND_WURK,
            
      // Denotes a client has found a solution. After this a vector<int> is sent
      // containing the solution
      HAS_SOLUTIONZ
  };

  struct MinionMessage
  {
      MessageType type;
      int info_a;
      int info_b;
      int info_c;
      
      MinionMessage(MessageType type_ = INVALID_MESSAGE, 
                    int info_a_ = 0, int info_b_ = 0, int info_c_ = 0) : 
                    type(type_), info_a(info_a_), info_b(info_b_), info_c(info_c_)
      { }
      
      template<class Archive>
      void serialize(Archive & ar, const unsigned int version)
      {
          ar & type;
          ar & info_a;
          ar & info_b;
          ar & info_c;
      }
      
      bool operator==(const MinionMessage& m) const
      { 
          return type == m.type && info_a == m.info_a && 
                 info_b == m.info_b && info_c == m.info_c; 
      }
      
      bool operator!=(const MinionMessage& m) const
          { return !(*this == m); }
  };
  
  static const MinionMessage protocol_version(INVALID_MESSAGE, 0, 0, 1);
  
#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#ifdef MINION_DEBUG
#define MINION_MPI_SEND(world, client, message) \
{ \
    std::cout << world.rank() << " sending to " << client << std::endl; \
    world.send(client, 0, message); \
}

#define MINION_MPI_RECV(world, client, message) \
{ \
    std::cout << world.rank() << " recieving from " << client << std::endl; \
    world.recv(client, 0, message); \
}

#endif



#endif
