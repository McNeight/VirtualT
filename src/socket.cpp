// Implementation of the Socket class.


#include "socket.h"
#include "string.h"
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>



Socket::Socket() :
  m_sock ( -1 )
{

  memset ( &m_addr,
	   0,
	   sizeof ( m_addr ) );

}

Socket::~Socket()
{
  if ( is_valid() )
#ifdef WIN32
	  ::closesocket ( m_sock );
#else
	  ::close ( m_sock );
#endif
}

bool Socket::create()
{
  m_sock = (int) socket ( AF_INET,
		    SOCK_STREAM,
		    0 );

  if ( ! is_valid() )
  {
//	int err = WSAGetLastError();
	return false;
  }


  // TIME_WAIT - argh
  int on = 1;
  if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
    return false;


  return true;

}



bool Socket::bind ( const int port )
{

  if ( ! is_valid() )
    {
      return false;
    }



  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );


  if ( bind_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::listen() const
{
  if ( ! is_valid() )
    {
      return false;
    }

  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );


  if ( listen_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
  new_socket.m_sock = (int) ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

  if ( new_socket.m_sock <= 0 )
    return false;
  else
    return true;
}


bool Socket::send ( const std::string s ) const
{
	int flags;
#if defined(WIN32)
		flags = 0;
#elif defined(__APPLE__)
		flags = SO_NOSIGPIPE;
#else
		flags = MSG_NOSIGNAL;
#endif
  int status = ::send ( m_sock, s.c_str(), (int) s.size(), flags );
  if ( status == -1 )
    {
      return false;
    }
  else
    {
      return true;
    }
}


int Socket::recv ( std::string& s ) const
{
  char buf [ MAXRECV + 1 ];

  s = "";

  memset ( buf, 0, MAXRECV + 1 );

  int status = ::recv ( m_sock, buf, MAXRECV, 0 );

  if ( status == -1 )
    {
	  printf("status == -1  errno == %d in Socket::recv\n", errno);
      return 0;
    }
  else if ( status == 0 )
    {
      return 0;
    }
  else
    {
      s = buf;
      return status;
    }
}

int Socket::recv ( char* rxBuf, int rxLen ) const
{
  memset ( rxBuf, 0, rxLen );

  int status = ::recv ( m_sock, rxBuf, rxLen, 0 );

  if ( status == -1 )
    {
	  printf("status == -1  errno == %d in Socket::recv\n", errno);
      return 0;
    }

  return status;
}


bool Socket::connect ( const std::string host, const int port )
{
  if ( ! is_valid() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );

  int status = inet_pton ( AF_INET, (char *) host.c_str(), &m_addr.sin_addr );

  if ( errno == EAFNOSUPPORT ) return false;

  status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

  if ( status == 0 )
    return true;
  else
    return false;
}

bool Socket::shutdown()
{
	if (!is_valid())
		return false;
#ifdef WIN32
	  ::closesocket ( m_sock );
#else
	  ::close ( m_sock );
#endif
	  return true;
}

/*void Socket::set_non_blocking ( const bool b )
{

  int opts;

  opts = fcntl ( m_sock,
		 F_GETFL );

  if ( opts < 0 )
    {
      return;
    }

  if ( b )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );

  fcntl ( m_sock,
	  F_SETFL,opts );

}
*/
