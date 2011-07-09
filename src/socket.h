// Definition of the Socket class

#ifndef Socket_class
#define Socket_class

#include <sys/types.h>

#ifdef WIN32
typedef int socklen_t;
#include <winsock.h>
extern "C"
{
#include "inet_pton.h"
}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#include <string>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
 public:
  Socket();
  virtual ~Socket();

  // Server initialization
  bool create();
  bool shutdown();
  bool bind ( const int port );
  bool listen() const;
  bool accept ( Socket& ) const;

  // Client initialization
  bool connect ( const std::string host, const int port );

  // Data Transimission
  bool send ( const std::string ) const;
  int recv ( std::string& ) const;
  int recv ( char* rxBuf, int rxLen ) const;


//  void set_non_blocking ( const bool );

  bool is_valid() const { return m_sock != -1; }

 private:

  int m_sock;
  sockaddr_in m_addr;


};


#endif
