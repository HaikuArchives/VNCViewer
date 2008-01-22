#if !defined(WND_CONNECTION_H)
#define WND_CONNECTION_H
/*
**
**	$Id: WndConnection.h,v 1.5 1998/10/25 16:05:53 bobak Exp $
**	$Revision: 1.5 $
**	$Filename: WndConnection.h $
**	$Date: 1998/10/25 16:05:53 $
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
#include <Window.h>
#include <View.h>

#include <DirectWindow.h>
#include <InterfaceDefs.h>
#include <Screen.h>
#include <zlib.h>

#include "App.h"
#include "Connection.h"

//#define SCREEN_STUFF
//#define DEBUG_VERBOSE

/*
**	defined in this header file
*/
class ViewConnection;
class WndConnection;

/****
**
*/
class WndConnection
{
protected:

	// @constructors
	WndConnection( const char* title, class Connection* conn, uint32 space, BRect frame );

public:

	enum msg	// message types for this window
	{
		msg_command			= B_SPECIFIERS_END,		// command keys go here
		msg_command_last	= msg_command + 'Z',	// last valid command key
		msg_fullscreen,
		msg_save_cnx,
		msg_view_cnx_infos,
		msg_update_delay,
		msg_invalid									// no valid messages
	};

	// @destructor
	~WndConnection( void );

	// @methods{Public Methods}
	static WndConnection* Create( Connection* conn );
	void ScreenUpdate( void );
	void ServerCutText( void );
	bool IsFullScreen(void) const { return myIsFullScreen; };
	void SetFullScreen(bool);
	void Minimize(bool yes);

	bool MessageReceived( BMessage* msg );
	void QuitSafe( void );

	inline void Lock( void )
	{
#if defined(DEBUG_VERBOSE)
		printf( "$WndConnection: locking..." );
		fflush( stdout );
#endif
		myWindow->Lock();
#if defined(DEBUG_VERBOSE)
		printf( "OK\n" );
#endif
	}
	
	inline void Unlock( void )
	{
		myWindow->Unlock();
#if defined(DEBUG_VERBOSE)
		printf( "$WndConnection: unlocked\n" );
#endif
	}

protected:

	class MyDirectWindow : public BDirectWindow
	{
	public:
		MyDirectWindow( WndConnection* parent, BRect frame, const char *title, window_type type, uint32 flags )
			: BDirectWindow( frame, title, type, flags ),
			  myParent(parent)
		{
			assert( parent != NULL );
		}

		void MessageReceived( BMessage* msg )
		{
			if (!myParent->MessageReceived( msg ))
				BDirectWindow::MessageReceived( msg );
		}

		virtual bool QuitRequested( void )
		{
			be_app->PostMessage( App::msg_window_closed );
			return false;
		}

		virtual void Zoom(BPoint origin, float width, float height)
		{
			PostMessage(msg_fullscreen);
		}
		
		virtual void Minimize(bool yes)
		{
			myParent->Minimize(yes);
			BDirectWindow::Minimize(yes);
		}
/*
		virtual void Hide(void)
		{
			myParent->Minimize(true);
			BWindow::Hide();
		}

		virtual void Show(void)
		{
			myParent->Minimize(false);
			BWindow::Show();
		}
*/
	private:
		WndConnection*	myParent;
	};
	
	class MyWindow : public BWindow
	{
	public:
		MyWindow( WndConnection* parent, BRect frame, const char *title, window_type type, uint32 flags )
			: BWindow( frame, title, type, flags ),
			  myParent(parent)
		{
			assert( parent != NULL );
		}

		void MessageReceived( BMessage* msg )
		{
			if (!myParent->MessageReceived( msg ))
				BWindow::MessageReceived( msg );
		}

		virtual bool QuitRequested( void )
		{
			be_app->PostMessage( App::msg_window_closed );
			return false;
		}

		virtual void Zoom(BPoint origin, float width, float height)
		{
			PostMessage(msg_fullscreen);
		}

		virtual void Minimize(bool yes)
		{
			myParent->Minimize(yes);
			BWindow::Minimize(yes);
		}

/*		virtual void Hide(void)
		{
			myParent->Minimize(true);
			BWindow::Hide();
		}

		virtual void Show(void)
		{
			myParent->Minimize(false);
			BWindow::Show();
		}
*/
	private:
		WndConnection*	myParent;
	};

#if defined(SCREEN_STUFF)
	class MyScreen : public BWindowScreen
	{
	public:
		MyScreen( const char *title, uint32 space, status_t *error )
			: BWindowScreen(title, space, error) {}

		bool QuitRequested( void )
		{
			be_app->PostMessage( App::msg_window_closed );
			return false;
		}
	};
#endif


public:
	time_t			myUpdateDelay;

private:

	// @members
	Connection*		myConnection;
	BWindow*		myWindow;
	BMenuBar		*myMenuBar;
	ViewConnection*	myView;
	bool			myIsFullScreen;
	bool			myIsMinimized;
	bool			myStartFullScreen;
	bool			myFullScreenAlertShown;
	BRect			myDesktopRect;
};

/****
**
*/
class ViewConnection : public BView
{
public:

	// @constructors
	ViewConnection( BRect frame, const char* name, Connection* conn );
	virtual ~ViewConnection(void);

	// @methods{Handlers}
	virtual void KeyDown( const char *bytes, int32 numBytes );
	virtual void KeyUp( const char *bytes, int32 numBytes );
	virtual void MouseDown( BPoint point );
	virtual void MouseUp( BPoint point );
	virtual void MouseMoved( BPoint point, uint32 transit, const BMessage *message );
	virtual void Draw( BRect updateRect );

	void ScreenUpdate( void );
	void ServerCutText( void );

	void UpdateFormat( const rfbPixelFormat* format );

	// Original mode before a full screen switch	
	display_mode oldMode;
protected:

	// @methods{Helpers}
	void Init( BRect rect );
	void CheckBufferSize( int bufsize );
	CARD32 GetXKey( char *bytes, int len, CARD32 *dead_key );
	int SendModifier( char *bytes, int len );
	void SendMouse( BPoint point );

	// @methods{Encodings}
	void RawRect(rfbFramebufferUpdateRectHeader *pfburh);
	void CopyRect(rfbFramebufferUpdateRectHeader *pfburh);
	void RRERect(rfbFramebufferUpdateRectHeader *pfburh);
	void CoRRERect(rfbFramebufferUpdateRectHeader *pfburh);
	void ZlibRect(rfbFramebufferUpdateRectHeader *pfburh);
	
	void HextileRect(rfbFramebufferUpdateRectHeader *pfburh);
	void HandleHextileEncoding8(int x, int y, int w, int h);
	void HandleHextileEncoding16(int x, int y, int w, int h);
	void HandleHextileEncoding32(int x, int y, int w, int h);

private:

	// member objects
	char*			myNetBuf;
	int				myNetBufSize;

	Connection*		myConnection;
	BBitmap*		myBitmap;
	BView*			myViewOff;

	uint32			myOldModifiers;

	// Shortcuts for speed's sake
	// Quick references to the color info
	CARD16	rs,gs,bs,rm,gm,bm; 	

    // The number of bytes required to hold at least one pixel.
	uint	minPixelBytes;
	CARD8	myBitsPerPixel;

	// ZLIB compression and TIGHT compression Stuff
	
	// Buffer for zlib decompression.
	void CheckZlibBufferSize(int bufsize);
	unsigned char *m_zlibbuf;
	int m_zlibbufsize;

	// zlib decompression state
	bool m_decompStreamInited;
	z_stream m_decompStream;

	// Valiables used by tight encoding:

	// Separate buffer for tight-compressed data.
	// char m_tightbuf[TIGHT_ZLIB_BUFFER_SIZE];

	// Four independent compression streams for zlib library.
	z_stream m_tightZlibStream[4];
	bool m_tightZlibStreamActive[4];

#if 0
	// Tight filter stuff. Should be initialized by filter initialization code.
	tightFilterFunc m_tightCurrentFilter;
	bool m_tightCutZeros;
	int m_tightRectWidth, m_tightRectColors;
	COLORREF m_tightPalette[256];
	CARD8 m_tightPrevRow[2048*3*sizeof(CARD16)];
#endif

	bool	compare_to_utfmap(char *bytes, int len, int32 index);

	key_map 		*myKeymap;
	char			*myUTF_Map;

};

#endif
