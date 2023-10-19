/*
**
**	$Id: Connection.cpp,v 1.5 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.5 $
**	$Filename: Connection.cpp $
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <Beep.h>
#include <Screen.h>
#include <View.h>

#include "Connection.h"
#include "vncauth.h"

#include "Utility.h"
#include "AuthDialog.h"

//#define	TIME_SCREENUPDATE

#define MAXPWLEN			8					// maximal password length
#define CHALLENGESIZE		16					// authentication challenge size

/*
**	our pixel formats if forced
*/
static const rfbPixelFormat vnc8bitFormat	= {8, 8, 1, 1, 7,7,3, 0,3,6,0,0};
static const rfbPixelFormat vnc16bitFormat	= {16, 16, 1, 1, 63, 31, 31, 0,6,11,0,0};

/*****************************************************************************
**	Connection
*/

/****
**	@purpose	Constructs a new Connection instance.
*/
Connection::Connection( void )
	: myDesktopName(NULL),
	  myIsConnected(false),
	  myPendingFormatChange(false),
	  myInsideHandler(false),
	  myAutoUpdate(true)
{
}

/****
**	@purpose	Destroys an existing Connection instance.
*/
Connection::~Connection( void )
{
	// free desktop name
	if (myDesktopName != NULL)
		delete [] myDesktopName;
}

/****
**	@purpose	Establish a connection to a VNC server.
**	@param		hostname	Name of the host to connect to.
**	@param		port		Number of the port to connect.
**	@param		passwd		The password used to authenticate the connection.
**							The string will be cleared after usage.
**	result		<true> if connection successful established, <false> otherwise.
*/
bool
Connection::Connect( const char* hostname, int port, char* passwd )
{
	bool result = false;
	uint host;

	// retrieve IP address
	if (!Socket::StringToIPAddr( hostname, &host ))
	{
		App::Alert( "Couldn't convert '%s' to host address.\n", hostname );
		return false;
	}

	// open connection
	if (mySocket.ConnectToTcpAddr( host, port ) < 0)
	{
		App::Alert( "Unable to connect to VNC server." );
		return false;
	}

	// initialize RFB connection
	if (Init( passwd ))
	{
		// we're now connected
		myIsConnected = true;

		// open the screen
		myWindow = WndConnection::Create( this );
		if (myWindow != NULL)
		{
			// start thread
			Run();
			
			// trigger message retriever
			PostMessage( msg_getnextrfbmessage );

			// update full screen for the first time
			SendFullFramebufferUpdateRequest();
			result = true;
		}
		else
		{
			App::Alert( "Failed to create window!" );
			mySocket.Close();
		}
	}
	else {
		mySocket.Close();
	}
	return result;
}

/****
**	@purpose	Establish a connection to a VNC server, without pass.
**	@param		hostname	Name of the host to connect to.
**	@param		port		Number of the port to connect.
**	result		<true> if connection successful established, <false> otherwise.
*/
bool
Connection::Connect( const char* hostname, int port )
{
	return Connect(hostname, port, NULL);
}

/****
**	@purpose	Initialize and authenticate RFB connection
**	@param		passwd	The password the authentication will be done with.
**						The string is cleared after usage.
**	@result		<true> if connection successful authenticated and initialized, <false> otherwise.
*/
bool
Connection::Init( char* passwd )
{
	rfbProtocolVersionMsg	pv;
	rfbClientInitMsg		ci;
	char	challenge[CHALLENGESIZE];
	CARD32	authScheme, authResult;
	int		major, minor;
	int		i;
	bool	authWillWork = true;
	BMessage archive;
	char dpasswd[8];

	/*
	**	if the connection is immediately closed, don't report anything, 
	**	so that pmw's monitor can make test connections
	*/
	if (!mySocket.ReadExact( pv, sz_rfbProtocolVersionMsg ))
		return false;

	// retrieve protocol version
	pv[sz_rfbProtocolVersionMsg] = 0;
	if (sscanf( pv, rfbProtocolVersionFormat, &major, &minor) != 2)
	{
		App::Alert( "Not a valid VNC server\n" );
		return false;
	}

	fprintf( stderr,
		"%s: VNC server supports protocol version %d.%d (viewer %d.%d)\n",
		App::GetApp()->GetName(),
		major, minor,
		rfbProtocolMajorVersion, rfbProtocolMinorVersion
	);

	if ((major == 3) && (minor < 3))
	{
		// if server is before 3.3 authentication won't work
		authWillWork = false;
	}
	else
	{
		// any other server version, just tell it what we want
		major = rfbProtocolMajorVersion;
		minor = rfbProtocolMinorVersion;
	}

	// acknowledge protocol version
	sprintf( pv, rfbProtocolVersionFormat, major, minor );
	if (!mySocket.WriteExact( pv, sz_rfbProtocolVersionMsg ))
		return false;

	// retrieve authentication scheme
	if (!mySocket.ReadExact( (char*)&authScheme, 4 ))
		return false;

	authScheme = Swap32IfLE( authScheme );
	switch (authScheme)
	{
	case rfbConnFailed:
	  {
	  	CARD32	reasonLen;
		char*	reason = "reason unknown";

		if (mySocket.ReadExact( (char *)&reasonLen, 4 ))
		{
			reasonLen	= Swap32IfLE( reasonLen );
			reason		= new char[reasonLen];

			if (mySocket.ReadExact( reason, reasonLen ))
				App::Alert( "VNC connection failed: %s", reason );
			else
				App::Alert( "VNC connection failed: reason unkown" );
			
			delete [] reason;
		}
		else
			App::Alert( "VNC connection failed: reason unkown" );

		return false;
	  }

	case rfbNoAuth:
		fprintf( stderr, "%s: No authentication needed\n", App::GetApp()->GetName() );
		break;

	case rfbVncAuth:
		if (!authWillWork)
		{
			App::Alert(
				"VNC server uses the old authentication scheme.\n"
				"You should kill your old desktop(s) and restart.\n"
				"If you really need to connect to this desktop use "
				"vncviewer3.2"
			);
			return false;
		}

		if (!mySocket.ReadExact( (char*)challenge, CHALLENGESIZE ))
			return false;

		if (passwd) {
			if (strlen( passwd ) == 0)
			{
				App::Alert( "No password given" );
				return false;
			}
		} else if( RehydrateWindow( "PasswordDialog" , &archive ) ) {
			AuthenticationDialog* ad = new AuthenticationDialog(&archive);
			{
				BRect rect;
				BScreen mainScreen;
				BRect rectScreen = mainScreen.Frame();
				
				int width = (int)ad->Frame().Width();
				int height = (int)ad->Frame().Height();
		
				// center window
 				rect.SetLeftTop( BPoint( (rectScreen.Width() - width) / 2, (rectScreen.Height() - height) / 2 ) );
 				rect.SetRightBottom( BPoint( rect.left + width, rect.top + height ) );
				ad->MoveTo( rect.left , rect.top );
			}
			
				if( !ad->DoModal() )
					return false; // need to return false.
			
			passwd = dpasswd; // use the local buffer since the arg is NULL;
			strncpy(passwd, ad->GetPassText(), 8); // don't want to get crashed by long passwords
	
			if (strlen(passwd) == 0) 
			{
				App::Alert( "Password had zero length\n");
				return false;//throw WarningException("Empty password");
			}
						
			ad->Lock();
			ad->Quit();
		}


		if (strlen( passwd ) > 8)
			passwd[8] = '\0';

		vncEncryptBytes( (u8*)challenge, passwd );

		// lose the password from memory // XXX: do we want this ?
		for (i = 0 ; i < strlen( passwd ) ; i++)
			passwd[i] = '\0';

		/*
		**	authenticate connection
		*/
		if (!mySocket.WriteExact( challenge, CHALLENGESIZE ))
			return false;

		if (!mySocket.ReadExact( (char*)&authResult, 4 ))
			return false;

		authResult = Swap32IfLE( authResult );
		switch (authResult)
		{
		case rfbVncAuthOK:
			fprintf( stderr, "%s: VNC authentication succeeded\n", App::GetApp()->GetName() );
			break;

		case rfbVncAuthFailed:
			App::Alert( "VNC authentication failed" );
			return false;

		case rfbVncAuthTooMany:
			App::Alert( "VNC authentication failed - too many tries" );
			return false;

		default:
			App::Alert( "Unknown VNC authentication result" );
			fprintf( stderr,
				"%s: Unknown VNC authentication result: %d\n",
				App::GetApp()->GetName(),
				(int)authResult
			);
			return false;
		}
		break;

	default:
		App::Alert( "Unknown authentication scheme from VNC server" );
		fprintf( stderr,
			"%s: Unknown authentication scheme from VNC server: %d\n",
			App::GetApp()->GetName(),
			(int)authScheme
		);
		return false;
	}

	/*
	**	get server init message
	*/
	ci.shared = (App::GetApp()->IsShareDesktop() ? 1 : 0);

	if (!mySocket.WriteExact( (char*)&ci, sz_rfbClientInitMsg ))
		return false;

	if (!mySocket.ReadExact( (char*)&myServerInit, sz_rfbServerInitMsg ))
		return false;

	myServerInit.framebufferWidth	= Swap16IfLE( myServerInit.framebufferWidth );
	myServerInit.framebufferHeight	= Swap16IfLE( myServerInit.framebufferHeight );
	myServerInit.format.redMax		= Swap16IfLE( myServerInit.format.redMax );
	myServerInit.format.greenMax	= Swap16IfLE( myServerInit.format.greenMax );
	myServerInit.format.blueMax		= Swap16IfLE( myServerInit.format.blueMax );
	myServerInit.nameLength			= Swap32IfLE( myServerInit.nameLength );

	/*
	**	get name of the desktop
	*/
	myDesktopName = new char[myServerInit.nameLength + 1];

	if (!mySocket.ReadExact( myDesktopName, myServerInit.nameLength ))
		return false;

	myDesktopName[myServerInit.nameLength] = 0;

	fprintf( stderr, "%s: Desktop name \"%s\"\n", App::GetApp()->GetName(), myDesktopName );

	fprintf( stderr,
		"%s: Connected to VNC server, using protocol version %d.%d\n",
		App::GetApp()->GetName(),
		rfbProtocolMajorVersion,
		rfbProtocolMinorVersion
	);

	fprintf( stderr, "%s: VNC server default format:\n", App::GetApp()->GetName() );
	PrintPixelFormat( &myServerInit.format );

	return true;
}


/****
**	@purpose	Prints pixel format in a human readable form.
**	@param		format	Pointer to the filled rfbPixelFormat structure.
*/
void
Connection::PrintPixelFormat( rfbPixelFormat* format )
{
	if (format->bitsPerPixel == 1)
	{
		fprintf( stderr, "Single bit per pixel.\n" );
		fprintf( stderr,
			"%s significant bit in each byte is leftmost on the screen.\n",
			(format->bigEndian ? "Most" : "Least")
		);
	}
	else
	{
		fprintf( stderr, "%d bits per pixel.\n", format->bitsPerPixel );
		if (format->bitsPerPixel != 8)
		{
			fprintf( stderr,
				"%s significant byte first in each pixel.\n",
				(format->bigEndian ? "Most" : "Least")
			);
		}
		if (format->trueColour)
		{
			fprintf( stderr, 
				"true colour: max red %d green %d blue %d\n",
				format->redMax, format->greenMax, format->blueMax
			);
			fprintf( stderr,
				"            shift red %d green %d blue %d\n",
				format->redShift, format->greenShift, format->blueShift
			);
		}
		else
		{
			fprintf( stderr, "Uses a colour map (not true colour).\n" );
		}
	}
}

/****
**	@purpose	Send pixel format structure to the RFB server.
**	@param		format	Pointer to the filled rfbPixelFormat structure.
*/
void
Connection::SetPixelFormat( const rfbPixelFormat* format )
{
	rfbSetPixelFormatMsg	spf;

	spf.type				= rfbSetPixelFormat;
	spf.format				= *format;
	spf.format.redMax		= Swap16IfLE( spf.format.redMax );
	spf.format.greenMax		= Swap16IfLE( spf.format.greenMax );
	spf.format.blueMax		= Swap16IfLE( spf.format.blueMax );
#if defined(__POWERPC__)
	spf.format.bigEndian	= 1;
#elif defined(__INTEL__) || defined(__amd64__)
	spf.format.bigEndian	= 0;
#else
#error	Endianess not defined!
#endif

	mySocket.WriteExact( (char*)&spf, sz_rfbSetPixelFormatMsg );
}

/****
**	@purpose	Setup internal pixel format structure according to the settings.
**	@result		A color_space value which can be passed to a BBitmap constructor.
*/
color_space
Connection::SetupPixelFormat( void )
{
	color_space result;

	if (App::GetApp()->IsUse8bit())
	{
		myFormat	= vnc8bitFormat;
		result		= B_CMAP8;
	}
	else if (!GetServerInit()->format.trueColour)
	{
		myFormat	= vnc16bitFormat;
#if defined(__POWERPC__)
		result		= B_RGB16_BIG;
#elif defined(__INTEL__) || defined(__amd64__)
		result		= B_RGB16;
#else
#error	Endianess not defined!
#endif
	}
	else
	{
		myFormat	= GetServerInit()->format;
		switch (myFormat.bitsPerPixel)
		{
		case 8:		result = B_CMAP8; break;
		case 16:	result = myFormat.bigEndian ? B_RGB16_BIG : B_RGB16; break;
		case 32:	result = myFormat.bigEndian ? B_RGB32_BIG : B_RGB32; break;

		default:
			App::Alert( "Unsupported # of bits per pixel!" );
			break;
		}
	}

#if 0
// for debugging purposes
	switch (result)
	{
	case B_CMAP8:		App::Alert("B_CMAP8");break;
	case B_RGB16:		App::Alert("B_RGB16");break;
	case B_RGB16_BIG:	App::Alert("B_RGB16_BIG");break;
	case B_RGB32:		App::Alert("B_RGB32");break;
	case B_RGB32_BIG:	App::Alert("B_RGB32_BIG");break;
	default:			App::Alert("unknown!");break;
	}
#endif

	return result;
}

/****
**	@purpose	Setup rectangle encodings with the RFB server.
**	@result		<true> if successful, <false> otherwise.  
*/
bool
Connection::SetFormatAndEncodings( void )
{
	char				buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];
	rfbSetEncodingsMsg*	se		= (rfbSetEncodingsMsg*)buf;
	CARD32*				encs	= (CARD32*)(&buf[sz_rfbSetEncodingsMsg]);
	int 				len		= 0;
	int	i;

	SetPixelFormat( &myFormat );

#if 0
	// set encodings
	se->type		= rfbSetEncodings;
	se->nEncodings	= 0;

	for (i = 0 ; i < App::GetApp()->GetExplicitEnc() ; i++)
		encs[se->nEncodings++] = Swap32IfLE( App::GetApp()->GetEncodings()[i] );

	if (mySocket.SameMachine())
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingRaw );

	if (App::GetApp()->AddCopyRect())
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingCopyRect );

	if (App::GetApp()->AddHextile())
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingHextile );

	if (App::GetApp()->AddCoRRE())
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingCoRRE );

	if (App::GetApp()->AddRRE())
		encs[se->nEncodings++] = Swap32IfLE( rfbEncodingRRE );

	len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

	se->nEncodings = Swap16IfLE( se->nEncodings );

	if (!mySocket.WriteExact( buf, len ))
		return false;

	return true;
#endif
    	// Put the preferred encoding first, and change it if the
	// preferred encoding is not actually usable.

	VNCOptions* m_opts = ((App *)be_app)->GetOptions();
	
 	se->type = rfbSetEncodings;
    se->nEncodings = 0;
   
   	for ( i = LASTENCODING; i >= rfbEncodingRaw; i--)
	{
		if (m_opts->m_PreferredEncoding == i) {
			if (m_opts->m_UseEnc[i]) {
				encs[se->nEncodings++] = Swap32IfLE(i);
			} else {
				m_opts->m_PreferredEncoding--;
			}
		}
	}

	// Now we go through and put in all the other encodings in order.
	// We do rather assume that the most recent encoding is the most
	// desirable!
	for (i = LASTENCODING; i >= rfbEncodingRaw; i--)
	{
		if ( (m_opts->m_PreferredEncoding != i) &&
			 (m_opts->m_UseEnc[i]))
		{
			encs[se->nEncodings++] = Swap32IfLE(i);
		}
	}

    len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;
	
    se->nEncodings = Swap16IfLE(se->nEncodings);
	
    mySocket.WriteExact((char *) buf, len);
    	
	return true;
}

// mmu
bool Connection::SetAutoUpdate(bool yes)
{
	if (myAutoUpdate == false && yes == true) {
		myAutoUpdate = yes;
		SendIncrementalFramebufferUpdateRequest();
	}
	myAutoUpdate = yes;
}


/****
**	@purpose	Send pointer event to the RFB server.
**	@param		x		Current x coordinate of the mouse pointer.
**	@param		y		Current y coordinate of the mouse pointer.
**	@param		buttons	Mask of the pressed buttons. Valid mask values
**						are any combinations of: rfbButton1Mask, rfbButton2Mask, 
**						rfbButton3Mask.
*/
void
Connection::SendPointerEvent( int x, int y, int buttons )
{
	rfbPointerEventMsg pe;

	pe.type			= rfbPointerEvent;
	pe.buttonMask	= buttons;

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	pe.x = Swap16IfLE( x );
	pe.y = Swap16IfLE( y );

	mySocket.WriteExact( (char*)&pe, sz_rfbPointerEventMsg );
}


/****
**	@purpose	Send keyboard event to the RFB server.
**	@param		key		Key value according to the X keyboard scheme.
**	@param		down	<true> = key down, <false> = key up
*/
void
Connection::SendKeyEvent( CARD32 key, bool down )
{
	rfbKeyEventMsg ke;

	ke.type	= rfbKeyEvent;
	ke.down	= down ? 1 : 0;
	ke.key	= Swap32IfLE( key );

	mySocket.WriteExact( (char*)&ke, sz_rfbKeyEventMsg );
}

/*****
**	@purpose	Send clipboard text to the RFB server.
**	@param		str		The text to send.
**	@param		len		Length of the text.
*/
void
Connection::SendClientCutText( char* str, int len )
{
	rfbClientCutTextMsg cct;

	cct.type	= rfbClientCutText;
	cct.length	= Swap32IfLE( len );

	mySocket.WriteExact( (char*)&cct, sz_rfbClientCutTextMsg );
	mySocket.WriteExact( str, len );
}

/****
**	@purpose	Send desktop update request to the RFB server.
**	@param		x			X coordinate of the desktop area to update.
**	@param		y			Y coordinate of the desktop area to update.
**	@param		w			Width of the desktop area to update.
**	@param		h			Height of the desktop area to update.
**	@param		incremental	<true> = request incemental updates, <false> = request full update.
*/
void
Connection::SendFramebufferUpdateRequest( int x, int y, int w, int h, bool incremental )
{
	rfbFramebufferUpdateRequestMsg fur;

	fur.type		= rfbFramebufferUpdateRequest;
	fur.incremental	= incremental ? 1 : 0;
	fur.x			= Swap16IfLE( x );
	fur.y			= Swap16IfLE( y );
	fur.w			= Swap16IfLE( w );
	fur.h			= Swap16IfLE( h );

	mySocket.WriteExact( (char *)&fur, sz_rfbFramebufferUpdateRequestMsg );
}

/****
**	@purpose	See BeBook: BLooper->QuitRequested()
*/
bool
Connection::QuitRequested( void )
{
	myWindow->QuitSafe();
	return true;
}

/****
**	@purpose	See BeBook: BLooper->MessageReceived()
*/
void
Connection::MessageReceived( BMessage* msg )
{
	switch (msg->what)
	{
	case msg_getnextrfbmessage:
#if defined(DEBUG_VERBOSE)
		printf( "$Enter MessageReceived: " );
#endif
		myWindow->Lock();
		if (DoRFBMessage())
		{
			PostMessage( msg_getnextrfbmessage );
			myWindow->Unlock();
//			snooze( 20 * 1000 );
			snooze(myWindow->myUpdateDelay);
#if defined(DEBUG_VERBOSE)
			printf( "$Exit MessageReceived true\n" );
#endif
			break;
		}
		myWindow->Unlock();
#if defined(DEBUG_VERBOSE)
		printf( "$Exit MessageReceived false\n" );
#endif
		break;

	default:
		BLooper::MessageReceived( msg );
		break;
	}
}

/****
**	@purpose	Gets a RFB message requests from the socket and handles it.
**	@result		<true> = go on with more requests, <false> = stop message feed.
*/
bool
Connection::DoRFBMessage( void )
{
#if defined(DEBUG_VERBOSE)
	printf( "$Enter DoRFBMessage\n" );
#endif

	if (myInsideHandler)
	{
#if defined(DEBUG_VERBOSE)
		printf( "$Exit DoRFBMessage myInsideHandler true\n" );
#endif
		return true;
	}
	myInsideHandler = true;

	// look at the type of the message, but leave it in the buffer 
	CARD8 msgType;
#if defined(DEBUG_VERBOSE)
	printf( "$DoRFBMessage: reading socket..." );
	fflush( stdout );
#endif
	int bytes = mySocket.Read( (char*)&msgType, 1, 0/*MSG_PEEK*/ );
#if defined(DEBUG_VERBOSE)
	printf( "%d byte received.\n", bytes );
#endif
	if (bytes == 0)
	{
		// do nothing
	}
	else if (bytes < 0)
	{
		//fprintf( stderr, "%s: Error while waiting for server message\n", App::GetApp()->GetName() );
	}
	else
	{
		assert( bytes == 1 );
		mySocket.PutBack( msgType );

		switch (msgType)
		{
		case rfbFramebufferUpdate:
#if defined(TIME_SCREENUPDATE)
			BStopWatch*	watch = new BStopWatch("ScreenUpdate");
			myWindow->ScreenUpdate();
			delete watch;
			SendFullFramebufferUpdateRequest();
#else
			myWindow->ScreenUpdate();
			if (myPendingFormatChange)
			{
				rfbPixelFormat oldFormat = myFormat;
				SetupPixelFormat();
				SetFormatAndEncodings();
				myPendingFormatChange = false;
				// if the pixel format has changed, request whole screen
				if (memcmp( &myFormat, &oldFormat, sizeof(rfbPixelFormat) ) != 0)
					SendFullFramebufferUpdateRequest();
				else
					SendIncrementalFramebufferUpdateRequest();
			}
			else if (myAutoUpdate) // XXX: mmu
				SendIncrementalFramebufferUpdateRequest();
#endif
			break;

		case rfbSetColourMapEntries:
			fprintf( stderr, "%s: Unhandled SetColormap message type received!\n", App::GetApp()->GetName() );
			App::GetApp()->ShowCursor();
			//App::Alert( "Unhandled SetColormap message type received!\n" );
			break;

		case rfbBell:
		  {
			rfbBellMsg bm;
			mySocket.ReadExact( (char*)&bm, sz_rfbBellMsg );
			if( App::GetApp()->GetOptions()->m_DeiconifyOnBell )
			{
				if( App::GetApp()->WindowAt(0)->IsMinimized() )//myWindow->IsMinimized() )
					 App::GetApp()->WindowAt(0)->Show();
			}
			beep();
			break;
		  }

		//$$$ ??? never received one of these messages, anyone knows why?
		case rfbServerCutText:
			myWindow->ServerCutText();
			break;

		default:
			fprintf( stderr, "%s: Unhandled message type %d received!\n", App::GetApp()->GetName(), msgType );
			App::GetApp()->ShowCursor();
			//App::Alert( "Unhandled message type received!" );
			myInsideHandler = false;
#if defined(DEBUG_VERBOSE)
			printf( "$Exit DoRFBMessage false\n" );
#endif
			return false;
		}
	}

	myInsideHandler = false;
#if defined(DEBUG_VERBOSE)
	printf( "$Exit DoRFBMessage true\n" );
#endif
	return true;
}
