// Implementation of the ServerSocket class

#include <iostream>
#include <stdio.h>
#include "serversocket.h"
#include "socketexception.h"

using namespace std;

ServerSocket::ServerSocket ( int port )
{
  if ( ! Socket::create() )
    {
      throw SocketException ( "Could not create server socket." );
    }

  if ( ! Socket::bind ( port ) )
    {
      throw SocketException ( "Could not bind to port." );
    }

  if ( ! Socket::listen() )
    {
      throw SocketException ( "Could not listen to socket." );
    }

}

ServerSocket::~ServerSocket()
{
}


const ServerSocket& ServerSocket::operator << ( const std::string& s ) const
{
  if ( ! Socket::send ( s ) )
    {
      throw SocketException ( "Could not write to socket." );
    }

  return *this;

}


const ServerSocket& ServerSocket::operator >> ( std::string& s ) const
{
  if ( ! Socket::recv ( s ) )
    {
      throw SocketException ( "Could not read from socket." );
    }

  return *this;
}

void ServerSocket::accept ( ServerSocket& sock )
{
  if ( ! Socket::accept ( sock ) )
    {
      throw SocketException ( "Could not accept socket." );
    }
}

int ServerSocket::recv ( char* rxBuf, int rxLen ) const
{
	int ret;

	ret = Socket::recv(rxBuf, rxLen);

	if (!ret)
	{
		throw SocketException ( "Could not read from socket.");
	}
	return ret;
}

/*
==============================================================================
ConsoleSocket print function
==============================================================================
*/
const ConsoleSocket& ConsoleSocket::operator << ( const std::string& s ) const
{
	// Print to stdout
	printf("%s", s.c_str());
	fflush(stdout);
	//cout << s;

	return *this;

}


const ConsoleSocket& ConsoleSocket::operator >> ( std::string& s ) const
{
	cin >> s;
	return *this;
}

