#if !defined(SOCKET_H)
#define SOCKET_H
/*
**
**	$Id: Socket.h,v 1.4 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.4 $
**	$Filename: Socket.h $
**	$Date: 1998/10/25 16:05:52 $
**
**	Author: Andreas F. Bobak (bobak@abstrakt.ch)
**
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
#include <stdio.h>

#define MAX_QUEUE	256

class Socket
{
public:
	
	// @constructors
	Socket( void );
	
	// @destructor
	~Socket( void );

	// @method{Member Methods}
	int ConnectToTcpAddr( uint host, int port );
	int ListenAtTcpPort( int port );
	int AcceptTcpConnection( void );

	void Close( void );

	static int StringToIPAddr( const char *str, unsigned int *addr );
	bool SameMachine( void );

	ssize_t Read( char *buf, size_t size, int flags );
	bool ReadExact( char *buf, int n );
	bool WriteExact( char *buf, int n );

	// Read the number of bytes and return them zero terminated in the buffer 
	inline void ReadString( char *buf, int length )
	{
		if (length > 0)
			ReadExact( buf, length );
		buf[length] = '\0';
	}

	// Put back one character
	inline void PutBack( char c )
	{
		myQueue[myBytesInQueue++] = c;
		if (myBytesInQueue >= MAX_QUEUE)
			printf( "MAX_QUEUE exceeded!\n" );
	}

private:

	// member objects
	char	myQueue[MAX_QUEUE];
	int		myBytesInQueue;
	int		myListenSocket;
	int		mySocket;
};

#endif
