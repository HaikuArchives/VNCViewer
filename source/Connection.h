#if !defined(CONNECTION_H)
#define CONNECTION_H
/*
**
**	$Id: Connection.h,v 1.5 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.5 $
**	$Filename: Connection.h $
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
#include "Types.h"

#include "App.h"
#include "rfbproto.h"
#include "Socket.h"
#include "WndConnection.h"

#if defined(__POWERPC__)
#define Swap16IfLE(s)	s
#define Swap32IfLE(l)	l
#elif defined(__INTEL__) || defined(__amd64__)
#define Swap16IfLE(s)	swap16( s )
#define Swap32IfLE(l)	swap32( l )
#else
#error	Endianess not defined!
#endif

/*
**	defined in this header file
*/
class Connection;

/****
**
*/
class Connection : public BLooper
{
public:

	enum msg
	{
		msg_getnextrfbmessage = 128		// retrieve another RFB message
	};


	// @constructors
	Connection( void );
	
	// @destructor
	~Connection( void );

	// @methods{Member Methods}
	bool Connect( const char* hostname, int port, char* passwd );
	bool Connect( const char* hostname, int port); // XXX:

	color_space SetupPixelFormat( void );
	bool SetFormatAndEncodings( void );

	void SendPointerEvent( int x, int y, int buttonMask );
	void SendKeyEvent( CARD32 key, bool down );
	void SendClientCutText( char* str, int len );

	bool DoRFBMessage( void );

	bool SetAutoUpdate(bool yes);


	// @methods{Selectors}
	const char* GetDesktopName( void ) const			{ return myDesktopName; }
	Socket* GetSocket( void )							{ return &mySocket; }
	bool IsConnected( void ) const						{ return myIsConnected; }
	const rfbPixelFormat* GetFormat( void ) const		{ return &myFormat; }
	const rfbServerInitMsg* GetServerInit( void ) const { return &myServerInit; }
	const bool IsAutoUpdate(void) const					{ return myAutoUpdate; }

protected:

	// @methods{Handlers}
	virtual bool QuitRequested( void );
	virtual void MessageReceived( BMessage* msg );

	// @methods{Helpers}
	bool Init( char* passwd );
	bool Init( ); // XXX:
	static void PrintPixelFormat( rfbPixelFormat* format );
	void SetPixelFormat( const rfbPixelFormat* format );
	void SendFramebufferUpdateRequest( int x, int y, int w, int h, bool incremental );

	void SendIncrementalFramebufferUpdateRequest( void )
	{
	    SendFramebufferUpdateRequest(
	    	0, 0,
	    	myServerInit.framebufferWidth, myServerInit.framebufferHeight,
	    	true
	    );
	}

	void SendFullFramebufferUpdateRequest( void )
	{
	    SendFramebufferUpdateRequest(
	    	0, 0,
	    	myServerInit.framebufferWidth, myServerInit.framebufferHeight,
	    	false
	    );
	}


private:

	// member objects
	Socket					mySocket;
	rfbServerInitMsg		myServerInit;	// server init message
	rfbPixelFormat			myFormat;		// used pixel format
	class WndConnection*	myWindow;		// VNC desktop

	char*		myDesktopName;				// desktop name
	bool		myIsConnected;				// <true> = connection succ. established
	bool		myPendingFormatChange;
	bool 		myInsideHandler;
	bool		myAutoUpdate;
};
#endif
