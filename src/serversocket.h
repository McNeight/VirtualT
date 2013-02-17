// Definition of the ServerSocket class

#ifndef ServerSocket_class
#define ServerSocket_class

#include "socket.h"


class ServerSocket : private Socket
{
 public:

  ServerSocket ( int port );
  ServerSocket (){};
  virtual ~ServerSocket();

  virtual const ServerSocket& operator << ( const std::string& ) const;
  virtual const ServerSocket& operator >> ( std::string& ) const;
  

  void accept ( ServerSocket& );
  bool shutdown (void) { return Socket::shutdown(); }; 
  int recv ( char* rxBuf, int rxLen ) const;

};

class ConsoleSocket : public ServerSocket
{
public:
	ConsoleSocket() {}

	virtual const ConsoleSocket& operator << ( const std::string& ) const;
	virtual const ConsoleSocket& operator >> ( std::string& ) const;
};

#endif
