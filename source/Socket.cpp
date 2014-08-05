/*
**
**	$Id: Socket.cpp,v 1.3 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.3 $
**	$Filename: Socket.cpp $
**	$Date: 1998/10/25 16:05:52 $
**
**	Author: Andreas F. Bobak (bobak@abstrakt.ch)
**
**	Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
**	Copyright (C) 1998 by Abstrakt Design, Andreas F. Bobak.
**
**	This is free software; you can redistribute it and / or modify it 
**	under the terms of the GNU General Public License as published by
**	the Free Software Foundation; either version 2 of the License, or
**	(at your option) any later version.
**
**	This software is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY;  without even the  implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
**	General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this software; if not, write to the Free Software
**	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
**	USA.
**
*/
#include <assert.h>
#include <unistd.h>

// check for BONE_VERSION
#include <sys/socket.h>
// actually for net_server
#include <netinet/in.h>

#if IPPROTO_TCP != 6
// net_server
# include <net/netdb.h>
//# include <net/socket.h>
#else
# include <netdb.h>
//# include <sys/socket.h>
# include <arpa/inet.h>
# define closesocket close
#endif

#ifndef __HAIKU__
// BeOS used signed int
// Haiku fixed it and used unsigned socklen_t type.
typedef int socklen_t;
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "App.h"
#include "Socket.h"

/****
**	@purpose	Construct a Socket instance.
*/
Socket::Socket( void )
	: myBytesInQueue(0),
	  mySocket(-1),
	  myListenSocket(-1)
{
}

/****
**	@purpose	Destroy an existing Socket instance
*/
Socket::~Socket( void )
{
	Close();
}

/****
**	@purpose	Close the socket connection.
*/
void
Socket::Close( void )
{
	if (mySocket != -1)
	{
		::closesocket( mySocket );
		mySocket = -1;
	}
	if (myListenSocket != -1)
	{
		::closesocket( myListenSocket );
		myListenSocket = -1;
	}
}

/****
**	@purpose	Try to read a number of bytes.
*/
ssize_t
Socket::Read( char *buf, size_t size, int flags )
{
	int i	= 0;

	assert( mySocket != -1 );

	if (myBytesInQueue > 0)
	{
		assert( myBytesInQueue <= MAX_QUEUE );
		memcpy( buf, myQueue, (myBytesInQueue <= size) ? myBytesInQueue : size );
		if (myBytesInQueue > size)
		{
			myBytesInQueue -= size;
			return size;
		}
		i = myBytesInQueue;
		myBytesInQueue = 0;
	}

	/*
	**	turn off blocking
	*/
	int one	= 1;
	if (::setsockopt( mySocket, SOL_SOCKET, SO_NONBLOCK, (char *)&one, sizeof(one) ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror(": DoRFBMessage: setsockopt");
	}

	// get data
	i = ::recv( mySocket, buf + i, (size - i), flags );

	/*
	**	turn on blocking
	*/
	one = 0;
	if (::setsockopt( mySocket, SOL_SOCKET, SO_NONBLOCK, (char *)&one, sizeof(one) ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror(": DoRFBMessage: setsockopt");
	}

	return i;
}


/****
**	@purpose	Read an exact number of bytes, and don't return until you've got them.
*/
bool
Socket::ReadExact( char *buf, int n )
{
	int i = 0;
	int j;

	if (myBytesInQueue > 0)
	{
		assert( myBytesInQueue <= MAX_QUEUE );
		memcpy( buf, myQueue, (myBytesInQueue <= n) ? myBytesInQueue : n );
		if (myBytesInQueue > n)
		{
			myBytesInQueue -= n;
			return n;
		}
		i = myBytesInQueue;
		myBytesInQueue = 0;
	}

	while (i < n)
	{
		j = ::recv( mySocket, buf + i, (n - i), 0 );
		if (j <= 0)
		{
			if (j < 0)
			{
				fprintf( stderr, App::GetApp()->GetName() );
				perror(": ReadExact::recv");
			}
			return false;
		}
		i += j;
	}

	return true;
}

/****
**	@purpose	Write an exact number of bytes, and don't return until you've sent them.
*/
bool
Socket::WriteExact( char *buf, int n )
{
    int i = 0;
    int j;

    while (i < n)
    {
		j = ::send( mySocket, buf + i, (n - i), 0 );
		if (j <= 0)
		{
			if (j < 0)
			{
				fprintf( stderr,App::GetApp()->GetName() );
				perror( ": send" );
			}
			else
				fprintf( stderr, "%s: send failed\n", App::GetApp()->GetName() );

		    return false;
		}
		i += j;
    }
    return true;
}


/*****
**	@purpose	Connects to the given TCP port.
*/
int
Socket::ConnectToTcpAddr( uint host, int port )
{
	struct sockaddr_in addr;
	int	one = 1;

	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( port );
	addr.sin_addr.s_addr	= host;

	mySocket = ::socket( AF_INET, SOCK_STREAM, 0 );
	if (mySocket < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": ConnectToTcpAddr: socket" );
		return -1;
	}

	if (::connect( mySocket, (struct sockaddr *)&addr, sizeof(addr) ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": ConnectToTcpAddr: connect" );
		::closesocket( mySocket );
		mySocket = -1;
		return -1;
	}

#if 0
	// no use on the mySocket
	if (::setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one) ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror(": ConnectToTcpAddr: setsockopt");
		::closesocket( mySocket );
		mySocket = -1;
		return -1;
	}
#endif

    return mySocket;
}

/****
**	@purpose	Starts listening at the given TCP port.
*/
int
Socket::ListenAtTcpPort( int port )
{
	struct sockaddr_in addr;
	int	one = 1;
	
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons( port );
	addr.sin_addr.s_addr	= INADDR_ANY;

	myListenSocket = ::socket( AF_INET, SOCK_STREAM, 0 );
	if (myListenSocket < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": ListenAtTcpPort: socket" );
		return -1;
	}
	
	if (::setsockopt( myListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one) ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": ListenAtTcpPort: setsockopt" );
		::closesocket( myListenSocket );
		myListenSocket = -1;
		return -1;
	}
	
	if (::bind( myListenSocket, (struct sockaddr *)&addr, sizeof(addr) ) < 0)
	{
		fprintf( stderr,App::GetApp()->GetName() );
		perror( ": ListenAtTcpPort: bind" );
		::closesocket( myListenSocket );
		myListenSocket = -1;
		return -1;
	}
	
	if (::listen( myListenSocket, 5 ) < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": ListenAtTcpPort: listen" );
		::closesocket( myListenSocket );
		myListenSocket = -1;
		return -1;
	}

	return myListenSocket;
}


/****
**	@purpose	Accept a TCP connection.
*/
int
Socket::AcceptTcpConnection( void )
{
    struct sockaddr_in addr;
    socklen_t	addrlen	= sizeof(addr);
    int	one		= 1;

	mySocket = ::accept( myListenSocket, (struct sockaddr *) &addr, &addrlen );
	if (mySocket < 0)
	{
		fprintf( stderr, App::GetApp()->GetName() );
		perror( ": AcceptTcpConnection: accept" );
		return -1;
	}

#if 0
	// no use on the BeOS
	if (::setsockopt( mySocket, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one) ) < 0)
    {
		fprintf( stderr,App::GetApp()->GetName() );
		perror( ": AcceptTcpConnection: setsockopt" );
		::closesocket( mySocket );
		mySocket = -1;
		return -1;
	}
#endif

    return mySocket;
}


/****
**	@purpose	Convert a host string to an IP address.
*/
int
Socket::StringToIPAddr( const char* str, uint *addr )
{
	struct hostent *hp;
	
	if ((*addr = ::inet_addr( str )) == -1)
	{
		if (!(hp = gethostbyname( str )))
			return 0;
		
		*addr = *(uint*)hp->h_addr;
	}

	return 1;
}


/****
**	@purpose	Test if the other end of a socket is on the same machine.
*/
bool
Socket::SameMachine( void )
{
	struct sockaddr_in peeraddr, myaddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	
	getpeername( mySocket, (struct sockaddr *)&peeraddr, &addrlen );
	getsockname( mySocket, (struct sockaddr *)&myaddr, &addrlen );
	
	return (peeraddr.sin_addr.s_addr == myaddr.sin_addr.s_addr);
}

