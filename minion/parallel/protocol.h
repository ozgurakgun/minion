#ifndef _PROTOCOL_H_TYOP
#define _PROTOCOL_H_TYOP

/* The following file describes the minion MPI protocol version 1.
 * It is intended this protocol will be implemented using boost::MPI
 * and is designed to be quite fragile, but optimised.
 *
 * A basic outline of the protocol:
 * MPI machine '0' is the 'server'. It does not do any solving, but only
 * deals with giving out work. All other machines are clients.
 *
 * 1) Every client sends a 'O_HAI' message to the server with the
 * version of the protocol it supports.
 * 2) If the protocols disagree, the server will send 'NO_WAI' to all clients
 * and abort. Else, it will send 'WELCUM' and then enter the setup phase.
 *
 * 3) The server sends a 'HAVE_FILEZ' message, which provides the instance.
 *
 * The clients then enter the following cycle.
 *
 * Wait until a 'HAVE_WURK' message arrives, and begin the search.
 * While working, a 'IZ_IN_UR_BASE_STEALIN_UR_WURK' may arrive. This should be 
 * replied to with a 'GIVEZ_TREEZ', providing a partial search, or 'NO_WURK' if
 * this instance has no work.
 *
 * When a solution is found, a 'HAS_SOLUTIONZ' message should be sent to the server.
 * When the client has no more work, it sends 'NO_WURK' to the server (note that
 * this might get sent multiple times, if a GIVEZ_TREEZ was sent before. This is not
 * a problem).
 *
 * If a client ever recieved 'HAVE_WURK' message which it is already operating,
 * it should reply 'NO_WAI' and abort. This should never happen.
 *
 * Finally, 'OUT_PLZ' denotes the end of search. At this point clients should exit
 * gracefully.
 *
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
      // It should never be sent or recieved.
      INVALID_MESSAGE,
      
      // Clients send this when search starts.
      // info_a, info_b and info_c denote protocol version.
      O_HAI,
      
      // Servers sends this to clients as a first message, if all clients
      // responding with the correct protocol version.
      WELCUM,
      
      // A general error state. Whenever the server or client recieves this, it
      // should abort quickly. Something has gone badly wrong.
      NO_WAI,
      
      // The instance file will be transmitted. Only from server to client.
      // Straight afterwards a std::string with the instance is sent.
      HAVE_FILEZ,
      
      // A more graceful error. Should only be sent by servers to clients at the
      // end of search.
      OUT_PLZ,
      
      // Passes work, either server->client or client->server. If a client recieves 
      // this while it is already working it should return NO_WAI and abort. 
      // After this, a Branch object will be sent describing the branch to take
      HAVE_WURK,
      
      // Asks a client for work, to which it should (generally) return it's top-most
      // branch, which it will then not search. If it is already out of work, it should
      // return NO_WURK
      IZ_IN_UR_BASE_STEALIN_UR_WURK,
      
      // Denotes a client has no work. Can be sent at any time, in particular in reply
      // to IZ_IN_UR_BASE_STEALIN_UR_WORK. Sending this multiple times is not a mistake,
      // but not encourage.
      NO_WURK,
            
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
