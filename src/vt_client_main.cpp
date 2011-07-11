#include "clientsocket.h"
#include "socketexception.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>

#include <stdlib.h>

using namespace std;

int main ( int argc, char* argv[] )
{
    fstream	filestr;
	char	*hostArg, *portArg;
	size_t	x, len;

	hostArg = portArg = NULL;
	if (argc < 2)
	{
		printf("usage: %s [hostname] port#\n", argv[0]);
		return 1;
	}

	/* Determine if argv[1] is the port number */
	len = strlen(argv[1]);
	for (x = 0; x < len; x++)
	{
		if ((argv[1][x] < '0') || (argv[1][x] > '9'))
		{
			hostArg = argv[1];
			break;
		}
	}
	/* If we reached the end of argv[1] and found all numbers, its port */
	if (x == len)
	{
		portArg = argv[1];
	}

	/* Determine if argv[2] is the port number */
	if (argc > 2)
	{
		len = strlen(argv[2]);
		for (x = 0; x < len; x++)
		{
			if ((argv[2][x] < '0') || (argv[2][x] > '9'))
			{
				hostArg = argv[2];
				break;
			}
		}
		/* If we reached the end of argv[1] and found all numbers, its port */
		if (x == len)
		{
			portArg = argv[2];
		}
	}

	if (portArg == NULL)
	{
		printf("%s: must specifiy valid port number\n", argv[0]);
		return 1;
	}

#ifdef WIN32
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int err; 

	wVersionRequested = MAKEWORD( 1, 1 ); 

	err = WSAStartup( wVersionRequested, &wsaData ); 
	if ( err != 0 ) 
	{ 
        /* Tell the user that we couldn't find a useable */ 
        /* winsock.dll.                                  */ 
		return 1; 
	} 

    /* Confirm that the Windows Sockets DLL supports 1.1.*/ 
    /* Note that if the DLL supports versions greater    */ 
    /* than 1.1 in addition to 1.1, it will still return */ 
    /* 1.1 in wVersion since that is the version we      */ 
    /* requested.                                        */ 

    if ( LOBYTE( wsaData.wVersion ) != 1 || 
            HIBYTE( wsaData.wVersion ) != 1 ) 
	{ 
        /* Tell the user that we couldn't find a useable */ 
        /* winsock.dll.                                  */ 
        WSACleanup( ); 
        return 1;    
    } 
#endif

	std::cout << "Use decimal, C hex (0x2a) or Asm hex (14h) for input\n";
	std::cout << "Return data reported according to specified radix\n";
	std::cout << "Type 'help' or 'quit' to exit\n";

  try
    {
	  if (hostArg == NULL)
		  hostArg = (char *) "localhost";

      ClientSocket client_socket ( hostArg , atoi(portArg) );

      std::string reply;
	  std::string cmd;
	  std::string outfile;
	  size_t			pos,c;
	  std::cout << "Ok> ";

	while (true)
	{
		getline(std::cin, cmd);

		outfile = "stdout";
		// Search for '>' in the command
		if ((pos = cmd.find('>')) != -1)
		{
			for (c = pos+1; c < cmd.length(); c++)
			{
				if (cmd[c] != ' ')
					break;
			}

			outfile = cmd.substr(c, cmd.length()-c);
			cmd = cmd.substr(0, pos);
			while (cmd[cmd.length()-1] == ' ')
				cmd = cmd.substr(0, cmd.length()-1);
		}

		if (cmd == "quit")
			break;
		if (cmd == "")
		{
			std::cout << "Ok> ";
			continue;
		}
	
      	try
		{
	  		client_socket << cmd;
		}
      	catch ( SocketException& ) 
			{}

		// Get response(s)
		if (outfile != "stdout")
			filestr.open(outfile.c_str(), fstream::out);
		reply = "";
		int retry = 0;				// Only wait so long for "Ok"
		while (reply != "Ok")
		{
			// Get response
      		try
			{
	  			client_socket >> reply;
			}
      		catch ( SocketException& e) 
			{
				std::cout << "Exception - " << e.description() << "\n";
			}

			// Check if response is "Ok" or contains "Ok" at the end
			size_t len = reply.length();
			if ((reply[len-1] == 'k') && (reply[len-2] == 'O'))
			{
				// Check if output goes to a file
				if (outfile == "stdout")
      				std::cout << reply << "> ";
				else
				{
					reply = reply.substr(0, reply.length()-2);
					filestr << reply;
					std::cout << "Ok> ";
				}
				reply = "Ok";
			}
			else
			{
				if (outfile == "stdout")	
					std::cout << reply;
				else
				{
					filestr << reply;
				}
			}

			// Check if response is "Syntax error"
			if (reply == "Syntax error")
			{
				reply = "Ok";
				std::cout << "\nOk> ";
			}
		}
		if (outfile != "stdout")
			filestr.close();

		if (cmd == "terminate")
			break;
	}

    }
  catch ( SocketException& e )
    {
      std::cout << "Exception was caught:" << e.description() << "\n";
    }

	std::cout << "\n";

  return 0;
}
