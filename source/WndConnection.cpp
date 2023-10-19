/*
**
**	$Id: WndConnection.cpp,v 1.6 1998/10/25 16:05:52 bobak Exp $
**	$Revision: 1.6 $
**	$Filename: WndConnection.cpp $
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
#include <Beep.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Region.h>
#include <Screen.h>
#include <String.h>
#include <UTF8.h>
#include <View.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>

#include "keysymdef.h"
#include "WndConnection.h"

#define INITIALNETBUFSIZE	4096

// if once I implement scrolling... 
#define hScrollPos	0
#define vScrollPos	0

// create rgb_color from separate colors
inline rgb_color get_color( uchar r, uchar g, uchar b )
{
	rgb_color a;
	a.red	= r;
	a.green	= g;
	a.blue	= b;
	a.alpha	= 0;
	return a;
}

// read a pixel from the given address, and set the highcolor in the off-screen view
#define SETHIGHCOLOR_FROM_PIXEL8_ADDRESS(p) (myViewOff->SetHighColor( \
				(uchar) (((*(CARD8 *)p >> rs) & rm) * 255 / rm), \
				(uchar) (((*(CARD8 *)p >> gs) & gm) * 255 / gm), \
				(uchar) (((*(CARD8 *)p >> bs) & bm) * 255 / bm), 0 ))

#define SETHIGHCOLOR_FROM_PIXEL16_ADDRESS(p) (myViewOff->SetHighColor( \
				(uchar) ((( *(CARD16 *)p >> rs) & rm) * 255 / rm), \
				(uchar) ((( *(CARD16 *)p >> gs) & gm) * 255 / gm), \
				(uchar) ((( *(CARD16 *)p >> bs) & bm) * 255 / bm), 0 ))

#define SETHIGHCOLOR_FROM_PIXEL32_ADDRESS(p) (myViewOff->SetHighColor( \
				(uchar) ((( *(CARD32 *)p >> rs) & rm) * 255 / rm), \
				(uchar) ((( *(CARD32 *)p >> gs) & gm) * 255 / gm), \
				(uchar) ((( *(CARD32 *)p >> bs) & bm) * 255 / bm), 0 ))

// read a pixel from the given address, and return a color value
#define COLOR_FROM_PIXEL8_ADDRESS(p) (get_color( \
				(uchar) (((*(CARD8 *)p >> rs) & rm) * 255 / rm), \
				(uchar) (((*(CARD8 *)p >> gs) & gm) * 255 / gm), \
				(uchar) (((*(CARD8 *)p >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL16_ADDRESS(p) (get_color( \
				(uchar) ((( *(CARD16 *)p >> rs) & rm) * 255 / rm), \
				(uchar) ((( *(CARD16 *)p >> gs) & gm) * 255 / gm), \
				(uchar) ((( *(CARD16 *)p >> bs) & bm) * 255 / bm) ))

#define COLOR_FROM_PIXEL32_ADDRESS(p) (get_color( \
                (uchar) ((( *(CARD32 *)p >> rs) & rm) * 255 / rm), \
                (uchar) ((( *(CARD32 *)p >> gs) & gm) * 255 / gm), \
                (uchar) ((( *(CARD32 *)p >> bs) & bm) * 255 / bm) ))

// shortcut to set one pixel...
#define SetPixel(x,y)	myViewOff->FillRect( BRect(x,y,x,y) )

/*****************************************************************************
**	WndConnection
*/

/****
**	@purpose	Constructs a new WndConnection instance.
**	@param		title	Title of the connection window the remote desktop resists in.
**	@param		conn	Pointer to the connection to display.
**	@param		space	Passed to the BScreen constructor.
**	@param		frame	Rectangle defining the remote desktop/window dimensions.
*/
WndConnection::WndConnection( const char* title, Connection* conn, uint32 space, BRect frame )
	: myConnection(conn),
	  myWindow(NULL),
	  myView(NULL),
	  myIsFullScreen(false),
	  myIsMinimized(false),
	  myStartFullScreen(false),
	  myFullScreenAlertShown(false),
	  myDesktopRect(frame),
	  myUpdateDelay(25000)
{
	BRect	limits;

	/*
	**	open a screen if we have a valid <space> value
	*/
#if defined(SCREEN_STUFF)
	if (space != -1)
	{
		status_t error;
		myWindow = new MyScreen( title, space, &error );
		if (error != B_OK)
		{
			App::Alert( "Failed to create screen. Fallback to window" );
			space = -1;
		}
		else
			limits = myWindow->ConvertFromScreen( frame );
	}
#endif

	/*
	**	no screen, so fallback to window
	*/
	if ( !App::GetApp()->GetOptions()->m_FullScreen )//space == -1)
	{
//		myWindow = new MyWindow( this, frame, title, B_TITLED_WINDOW, 0 );
		myWindow = new MyDirectWindow(this, frame, title, B_TITLED_WINDOW, 0 );

		/*
		**	limit window size
		*/
/*		float	minw, minh, maxw, maxh;
	myWindow->Lock(); //XXX: useless ?
		limits = myWindow->ConvertFromScreen( frame );
		myWindow->GetSizeLimits( &minw, &maxw, &minh, &maxh );
		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() );
	myWindow->Unlock();
*/	}
	else
	{
		myWindow = new MyDirectWindow(this, frame, title, B_TITLED_WINDOW, 0 );
			
		/*
		**	limit window size
		*/
/*		float	minw, minh, maxw, maxh;
	myWindow->Lock(); //XXX: useless ?
		limits = myWindow->ConvertFromScreen( frame );
		myWindow->GetSizeLimits( &minw, &maxw, &minh, &maxh );
		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() );
		((BDirectWindow *)myWindow)->SetFullScreen(true);
	myWindow->Unlock();
*/		
	}

	/*
	**	create menus
	*/
	BMenuBar*	bar	 = new BMenuBar( myWindow->Bounds(), "menu_bar" );
	BMenu*		menu;

	menu = new BMenu( "File" );
	menu->AddItem( new BMenuItem( "Send Ctrl-Alt-Del", new BMessage(App::msg_send_ctrlaltdel) ) );
	menu->AddItem( new BSeparatorItem() );
	menu->AddItem( new BMenuItem( "About VNCviewer", new BMessage(B_ABOUT_REQUESTED) ) );
	menu->AddItem( new BSeparatorItem() );
	menu->AddItem( new BMenuItem( "Quit", new BMessage(B_QUIT_REQUESTED), 'Q' ) );
	menu->SetTargetForItems( App::GetApp() );
	bar->AddItem( menu );

	menu = new BMenu( "View" );
	menu->AddItem( new BMenuItem( "Fullscreen", new BMessage(msg_fullscreen) ) );
	menu->AddItem( new BMenuItem( "Connection infos", new BMessage(msg_view_cnx_infos) ) );
	
//	menu->AddItem( new BSeparatorItem() );
//	menu->AddItem( new BMenuItem( "About VNCviewer", new BMessage(B_ABOUT_REQUESTED) ) );
//	menu->AddItem( new BSeparatorItem() );
//	menu->AddItem( new BMenuItem( "Quit", new BMessage(B_QUIT_REQUESTED), 'Q' ) );
	menu->SetTargetForItems( myWindow );
	bar->AddItem( menu );

	myMenuBar = bar; // XXX: so we can hide it later
	
	/*
	**	add menu bar
	*/
	myWindow->Lock();
	myWindow->AddChild( bar );
	myWindow->SetKeyMenuBar( bar );

	int barheight = (int)(bar->Frame().bottom - bar->Frame().top + 1);

	float	minw, minh, maxw, maxh;
	limits = myWindow->ConvertFromScreen( frame );
	myWindow->GetSizeLimits( &minw, &maxw, &minh, &maxh );
	

//XXX:
		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() + barheight);


	if (!App::GetApp()->GetOptions()->m_FullScreen) {// XXX: handle this dynamically
//		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() + barheight);

		myWindow->ResizeBy( 0, barheight );
		// shift to window coordinate system
		frame.OffsetTo( BPoint(0, barheight) /*B_ORIGIN*/ );
	} else {
		myStartFullScreen = myIsFullScreen = true;
		myFullScreenAlertShown = true;
		App::Alert("Hint: To escape from fullscreen mode, press\nShift-Control-Esc");
//		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() - barheight);

//		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() );
		myMenuBar->Hide();
		myWindow->ResizeBy(0, 0);
		frame.OffsetTo( BPoint(0, 0) /*B_ORIGIN*/ );
	}
	


	/*
	**	resize window to fit menu bar
	*/

	


//	if (App::GetApp()->GetOptions()->m_FullScreen) // XXX: handle this dynamically
	if (myStartFullScreen) // XXX: handle this dynamically
		((BDirectWindow *)myWindow)->SetFullScreen(true);

	myWindow->Unlock();

	/*
	**	attach view to window
	*/
	myView = new ViewConnection( frame/*limits*/, "view", conn );
	myWindow->Lock();
	myWindow->AddChild( myView );
	myView->MakeFocus();
	myWindow->Unlock();

	// add shortcut
	myWindow->AddShortcut( 'X', B_COMMAND_KEY, new BMessage(msg_command + 'X') );

	// 'open' window
	myWindow->Show();
}

/****
**	@purpose	Destroys an existing WndConnection instance.
*/
WndConnection::~WndConnection( void )
{
//if (myStartFullScreen) // XXX: handle this dynamically
//		((BDirectWindow *)myWindow)->SetFullScreen(false);
//	SetFullScreen(false);
}

/****
**	@purpose	Instantiates an initializes a new WndConnection object.
**	@param		conn	A pointer to the remote connection.
**	@result		A pointer to the newly created WndConnection object or NULL,
**				if the creation failed.
*/
WndConnection*
WndConnection::Create( Connection* conn )
{
	// we need an up and running connection
	if ((conn == NULL) || !conn->IsConnected())
		return NULL;

	// shortcuts
	const rfbServerInitMsg*	si = conn->GetServerInit();

	BRect	rectScreen = BScreen( B_MAIN_SCREEN_ID ).Frame();
 	BRect	rect;
 	float	width	= si->framebufferWidth;
 	float	height	= si->framebufferHeight;

	// center window
	rect.SetLeftTop( BPoint( (rectScreen.Width() - width) / 2, (rectScreen.Height() - height) / 2 ) );
	rect.SetRightBottom( BPoint( rect.left + width - 1, rect.top + height - 1) );

	// retrieve space value
	uint32 space = (uint32)-1;

#if defined(SCREEN_STUFF)
	if ((width == 640) && (height == 480))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_640x480; break;
		case 16:	space = B_16_BIT_640x480; break;
		case 32:	space = B_32_BIT_640x480; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
	else if ((width == 800) && (height == 600))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_800x600; break;
		case 16:	space = B_16_BIT_800x600; break;
		case 32:	space = B_32_BIT_800x600; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
	else if ((width == 1024) && (height == 768))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_1024x768; break;
		case 16:	space = B_16_BIT_1024x768; break;
		case 32:	space = B_32_BIT_1024x768; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
	else if ((width == 1152) && (height == 900))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_1152x900; break;
		case 16:	space = B_16_BIT_1152x900; break;
		case 32:	space = B_32_BIT_1152x900; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
	else if ((width == 1280) && (height == 1024))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_1280x1024; break;
		case 16:	space = B_16_BIT_1280x1024; break;
		case 32:	space = B_32_BIT_1280x1024; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
	else if ((width == 1600) && (height == 1200))
	{
		switch (si->format.depth)
		{
		case 8:		space = B_8_BIT_1600x1200; break;
		case 16:	space = B_16_BIT_1600x1200; break;
		case 32:	space = B_32_BIT_1600x1200; break;
		default:	App::Alert( "Color depth not supported" ); return NULL;
		}
	}
#endif

	// create and return object
	return new WndConnection( conn->GetDesktopName(), conn, space, rect );
}

void
WndConnection::SetFullScreen(bool full)
{
	BRect	limits;
	
	if (myIsFullScreen == full)
		return;

	myWindow->Lock();
//	myView->Lock();
	int barheight = (int)(myMenuBar->Frame().bottom - myMenuBar->Frame().top + 1);

	float	minw, minh, maxw, maxh;
	
//	limits = myWindow->ConvertFromScreen( myWindow->Frame() );
	myWindow->GetSizeLimits( &minw, &maxw, &minh, &maxh );
	
	if (full) {// XXX: handle this dynamically
		BScreen scr;
		if (!myFullScreenAlertShown) {
			myFullScreenAlertShown = true;
			App::Alert("Hint: To escape from fullscreen mode, press\nShift-Control-Esc");
		}
		limits = myWindow->ConvertFromScreen( myWindow->Frame() );
//		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() - barheight);

//		myWindow->ResizeBy( 0, -barheight );
		// shift to window coordinate system
		myView->MoveBy(0, - barheight);
//		myView->Frame().OffsetTo( BPoint(0, barheight) /*B_ORIGIN*/ );
//		if (myMenuShown)
			myMenuBar->Hide();
		((BDirectWindow *)myWindow)->SetFullScreen(true);
	} else {
		limits = myWindow->ConvertFromScreen( myDesktopRect );
		((BDirectWindow *)myWindow)->SetFullScreen(false);
//		if (!myMenuShown)
			myMenuBar->Show();
		myView->MoveBy(0, barheight);
		// = myView->Origin();
//		myView->SetFrame.OffsetTo( BPoint(0, 0) /*B_ORIGIN*/ );
//		myView->SetOrigin(o);
		myWindow->ResizeBy(0, barheight);
//		myWindow->ResizeTo(myDesktopRect);
//		myWindow->SetSizeLimits( minw, limits.Width(), minh, limits.Height() + barheight);
	}

	App::GetApp()->GetOptions()->m_FullScreen = full;
	myIsFullScreen = full;

//	myView->Unlock();
	myWindow->Unlock();
}

/****
**	@purpose	Handle window message. Called by myWindow.
**	@param		msg		Message to handle.
**	@result		<true> if message has been handled, <false> otherwise.
*/
bool
WndConnection::MessageReceived( BMessage* msg )
{
	if ((msg->what >= msg_command) && (msg->what <= msg_command_last))
	{
		myConnection->SendKeyEvent( XK_Alt_L, true );
		myConnection->SendKeyEvent( msg->what - msg_command, true );
		myConnection->SendKeyEvent( msg->what - msg_command, false );
		myConnection->SendKeyEvent( XK_Alt_L, false );
		return true;
	}
	switch (msg->what) {

	case msg_save_cnx:
		break;
	case msg_view_cnx_infos:
		{
			BString txt("Connection informations\n\n");
			const rfbServerInitMsg *smsg;
			if (myConnection->GetDesktopName())
				txt << "Desktop name: " << myConnection->GetDesktopName() << "\n";
			if (smsg = myConnection->GetServerInit()) {
				txt << "Size: " << smsg->framebufferWidth;
				txt << " x " << smsg->framebufferHeight << "\n";
				
			}
			App::Alert(txt.String());
		}
		break;
	case msg_update_delay:
		break;
	case msg_fullscreen:
		SetFullScreen(!IsFullScreen());
		return true;
	case B_ZOOM:
		SetFullScreen(!IsFullScreen());
		return true;
	case B_WINDOW_ACTIVATED:
	puts("B_WINDOW_ACTIVATED");
		Minimize(false);
		break;
//	case B_WORKSPACE_ACTIVATED:
//		break;
/*
	case B_KEY_UP:
		puts("B_KEY_UP");
		break;
	case B_KEY_DOWN:
		puts("B_KEY_DOWN");
		break;
	case B_UNMAPPED_KEY_UP:
		puts("B_UNMAPPED_KEY_UP");
		break;
	case B_UNMAPPED_KEY_DOWN:
		puts("B_UNMAPPED_KEY_DOWN");
		break;
*/		
	}
	return false;
}

//XXX: fixme
void WndConnection::Minimize(bool yes)
{
return; // XXX fixme !
puts("WndConnection::Minimize()");
	if ((yes == true) && (myIsMinimized == false)) {
//	if (yes) {
		puts("mini(true)");
		myConnection->SetAutoUpdate(false);
	} else if ((yes == false) && (myIsMinimized == true)) {
//	} else {
		puts("mini(false)");
		myConnection->SetAutoUpdate(true);
//				myConnection->SendIncrementalFramebufferUpdateRequest();
	}
	myIsMinimized = yes;
}


/****
**	@purpose	Safely close, quit and shutdown this window and thread.
*/
void
WndConnection::QuitSafe( void )
{
	// remove shortcut
	myWindow->RemoveShortcut( 'X', B_COMMAND_KEY );

	SetFullScreen(false);

	// XXX: ???
	if( myView->oldMode.virtual_width != 0 )
	{
		BScreen mainScreen;
		mainScreen.SetMode( &myView->oldMode , false );
	}
	
	// quit window
	myWindow->Lock();
	myWindow->Quit();
	//NO myWindow->Unlock();
}

/****
**	@purpose	Safely pass screen update request to the window's view.
*/
void
WndConnection::ScreenUpdate( void )
{
#if defined(DEBUG_VERBOSE)
	printf( "$WndConnection: myConnection locking..." );
	fflush( stdout );
#endif
	myConnection->Lock();
#if defined(DEBUG_VERBOSE)
	printf( "OK\n" );
#endif
	Lock();
	myView->ScreenUpdate();
	Unlock();
	myConnection->Unlock();
#if defined(DEBUG_VERBOSE)
	printf( "$WndConnection: myConnection unlocked\n" );
#endif
}

/****
**	@purpose	Safely pass server clipboard cut request to the window's view.
*/
void
WndConnection::ServerCutText( void )
{
	beep();
	beep();
	printf( "ServerCutText\n" );
	Lock();
	myView->ServerCutText();
	Unlock();
}


/*****************************************************************************
**	ViewConnection
*/

/****
**	@purpose	Creates a new ViewConnection instance.
**	@param		frame	BView/remote desktop dimensions. 
**	@param		name	Name of this view.
**	@param		conn	Pointer to a valid RFB connection.
*/
ViewConnection::ViewConnection( BRect frame, const char* name, Connection* conn )
	: BView(frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	  myBitmap(NULL),
	  myConnection(conn),
  	  myKeymap(NULL),
	  myUTF_Map(NULL)
{
	assert( conn != NULL );

	get_key_map(&myKeymap, &myUTF_Map);

	myOldModifiers = modifiers();

	// initialize view
	frame.OffsetTo( B_ORIGIN );
	Init( frame );
}

ViewConnection::~ViewConnection(void)
{
	if (myKeymap) free(myKeymap);
	if (myUTF_Map) free(myUTF_Map);
}

void dump_timing_info(display_timing *pTiming)
{
	printf("pixel_clock = %d\nh_display = %d\nh_sync_start = %d\nh_sync_end = %d\nh_total = %d\n" , pTiming->pixel_clock , pTiming->h_display, pTiming->h_sync_start, pTiming->h_sync_end , pTiming->h_total );
	printf("v_display = %d\nv_sync_start = %d\nv_sync_end = %d\nv_total = %d\nflags = %d\n" , pTiming->v_display , pTiming->v_sync_start , pTiming->v_sync_end , pTiming->v_total , pTiming->flags );
}

/****
**	@purpose	Initialize this instance.
**	@param		rect	BView/remote desktop dimensions
*/
void
ViewConnection::Init( BRect rect )
{
	/*
	**	init socket buffer
	*/
	oldMode.virtual_width = 0;
	
	// new zlib stuff
	m_decompStreamInited = false;
	for (int i = 0; i < 4; i++)
		m_tightZlibStreamActive[i] = false;
	m_zlibbuf = NULL;
	m_zlibbufsize = 0;

	myNetBuf		= NULL;
	myNetBufSize	= 0;
	CheckBufferSize( INITIALNETBUFSIZE );

	// init pixel format
	color_space space = myConnection->SetupPixelFormat();
	UpdateFormat( myConnection->GetFormat() );

if ( App::GetApp()->GetOptions()->m_FullScreen )
{
		BScreen mainScreen;
		
		display_mode curMode;
		mainScreen.GetMode(&curMode);
		oldMode = curMode;
		//dump_timing_info( &curMode.timing );
		int resolution = (int)(((float)curMode.timing.pixel_clock * 1000.0) / (curMode.timing.v_total * curMode.timing.h_total) + .5);
		
		display_mode* pList;
		uint32 modeCount = 0;
				
		mainScreen.GetModeList( &pList , &modeCount);
		int i;
		for(i = 0; i < modeCount; i ++)
		{
			int resolution2 = (int)((float)pList[i].timing.pixel_clock * 1000.0) / (pList[i].timing.v_total * pList[i].timing.h_total);
			if( pList[i].virtual_width == myConnection->GetServerInit()->framebufferWidth && pList[i].virtual_height == myConnection->GetServerInit()->framebufferHeight && pList[i].space == curMode.space )
			{
				if( abs( resolution - resolution2 ) < 2 )
				{
					mainScreen.SetMode( &pList[i] , false);
					break;
				}
			}
		}
		
		if( i == modeCount ) // Couldn't find the same resolution, find the nearest higher one
		{
			for(int i = 0; i < modeCount; i ++)
			{
				int resolution2 = (int)((float)pList[i].timing.pixel_clock * 1000.0) / (pList[i].timing.v_total * pList[i].timing.h_total);
				if( pList[i].virtual_width == myConnection->GetServerInit()->framebufferWidth && pList[i].virtual_height == myConnection->GetServerInit()->framebufferHeight && pList[i].space == curMode.space )
				{
					if( resolution2 > resolution )
					{
						mainScreen.SetMode( &pList[i] , false);
						break;
					}
				}
			}
		}
}
#if 1
	/*
	**	allocate off-screen bitmap and view
	*/
	myBitmap	= new BBitmap( rect, space, true );
	myViewOff	= new BView( rect, "", 0, B_WILL_DRAW );

	myViewOff->SetViewColor( B_TRANSPARENT_32_BIT );
	SetViewColor( B_TRANSPARENT_32_BIT );

	myBitmap->Lock();

	// add view to bitmap
	myBitmap->AddChild( myViewOff );

	/*
	**	put a "please wait" message up initially
	*/
	rgb_color	c = myViewOff->HighColor();

	myViewOff->SetPenSize( 1.0 );
	myViewOff->SetHighColor( 0, 0, 0 );
	myViewOff->MovePenTo( BPoint( 20, 20 ) );
	myViewOff->DrawString( "Please wait - initial screen loading" );

	myViewOff->SetHighColor( c );

	myViewOff->Sync();		// flush all drawing to the bitmap
	myBitmap->Unlock();
#else
	/*
	**	do not allocate off-screen bitmap and view
	*/

	/*
	**	put a "please wait" message up initially
	*/
	rgb_color	c = HighColor();

	SetPenSize( 1.0 );
	SetHighColor( 0, 0, 0 );
	MovePenTo( BPoint( 20, 20 ) );
	DrawString( "Please wait - initial screen loading" );

	SetHighColor( c );
#endif

	// init connection format and rectangle encodings
	myConnection->SetFormatAndEncodings();
}

/****
**	@purpose	See BeBook: BView->Draw()
*/
void
ViewConnection::Draw( BRect update )
{
	assert( myBitmap != NULL );
	// just flush bitmap to view
	DrawBitmap( myBitmap, update, update );
}

bool ViewConnection::compare_to_utfmap(char *bytes, int len, int32 index)
{
	int clen;
	clen = myUTF_Map[index++];
	if (len != clen)
		return false;
	while (clen--)
		if (myUTF_Map[index++] != *bytes++)
			return false;
	return true;
}

/****
**	@purpose	Convert Be key value to X key value.
**	@param		byte	Key value according to Be keycoding (UTF8)
**	@result		X key value.
*/
CARD32
ViewConnection::GetXKey( char *bytes, int len, CARD32 *dead_key)
{
	u16		key = 0;
	char byte;

	*dead_key = 0;

	if ((len > 1)) {
		char buff[4];
		int32 srclen=len;
		int32 dstlen=4;
		int32 cookie=0;

		for (int i=1; i<32; i+=2) {
			if (compare_to_utfmap(bytes, len, myKeymap->grave_dead_key[i])) {
				*dead_key = XK_dead_grave;
				key = myUTF_Map[myKeymap->grave_dead_key[i-1]+1];
				printf("deadkey: %c\n", key);
				goto gotone;
			}
		}

		for (int i=1; i<32; i+=2) {
			if (compare_to_utfmap(bytes, len, myKeymap->acute_dead_key[i])) {
				*dead_key = XK_dead_acute;
				key = myUTF_Map[myKeymap->acute_dead_key[i-1]+1];
				printf("deadkey: %c\n", key);
				goto gotone;
			}
		}

		for (int i=1; i<32; i+=2) {
			if (compare_to_utfmap(bytes, len, myKeymap->circumflex_dead_key[i])) {
				*dead_key = XK_dead_circumflex;
				key = myUTF_Map[myKeymap->circumflex_dead_key[i-1]+1];
				printf("deadkey: %c\n", key);
				goto gotone;
			}
		}

		for (int i=1; i<32; i+=2) {
			if (compare_to_utfmap(bytes, len, myKeymap->tilde_dead_key[i])) {
				*dead_key = XK_dead_tilde;
				key = myUTF_Map[myKeymap->tilde_dead_key[i-1]+1];
				printf("deadkey: %c\n", key);
				goto gotone;
			}
		}
gotone:
		if (*dead_key) {
//			myConnection->SendKeyEvent(dead_key, true);
			bytes[0] = key;
		} else {

//			int32 offset;
//			offset = (((unsigned char)bytes[0]) << 8) & 0x0FF00 + (((unsigned char)bytes[1]) & 0x0FF);


/*			if (myUTF_Map)
				printf("UTF8 -> '%c'\n", myUTF_Map[offset]);
			else puts("NO UTF MAP");
*/
			convert_from_utf8(B_ISO1_CONVERSION, bytes, &srclen, buff, &dstlen, &cookie, '\0');
			printf("UTF8-> '%c'\n", buff[0]);
			
			switch (buff[0]) {
			case '\xe7':
				*dead_key = XK_dead_cedila;
				bytes[0] = 'c';
				break;
			case '\xb5':
				bytes[0] = XK_mu;
				break;
			case '\xa7':
				bytes[0] = XK_paragraph;
				break;
			case '\xa3':
				bytes[0] = XK_sterling;
				break;
			default:
//				bytes[0] = buff[0];
				break;
			}
		}
	}
/*	if ((len == 2) && ((unsigned char)bytes[0] == (unsigned char)0xC2)) {
		if ((unsigned char)bytes[1] == (unsigned char)0x0b2) bytes[0] = '²';
		if ((unsigned char)bytes[1] == (unsigned char)0x0a7) bytes[0] = '§';
		if ((unsigned char)bytes[1] == (unsigned char)0x0a3) bytes[0] = '£';
		if ((unsigned char)bytes[1] == (unsigned char)0x0a4) bytes[0] = '¤';
		if ((unsigned char)bytes[1] == (unsigned char)0x0b5) bytes[0] = 'µ';

		len = 1;
	} else {
		
	}
*/
	byte = bytes[0];
	
#if 1
	switch (byte)
	{
	case B_BACKSPACE:		printf( "Backspace\n" ); break;
	case B_DELETE:			printf( "Delete\n" ); break;
	case B_INSERT:			printf( "Insert\n" ); break;

	case B_LEFT_ARROW:		printf( "Left\n" ); break;
	case B_RIGHT_ARROW:		printf( "Right\n" ); break;
	case B_UP_ARROW:		printf( "Up\n" ); break;
	case B_DOWN_ARROW:		printf( "Down\n" ); break;

	//case B_ENTER:			key = XK_Execute; break;
	case B_SPACE:			printf( "space\n" ); break;
	case B_RETURN:			printf( "Return\n" ); break;
	case B_TAB:				printf( "Tab\n" ); break;

	case B_ESCAPE:			printf( "Escape\n" ); break;
	case B_HOME:			printf( "Home\n" ); break;
	case B_END:				printf( "End\n" ); break;
	case B_PAGE_UP:			printf( "Page_Up\n" ); break;
	case B_PAGE_DOWN:		printf( "Page_Down\n" ); break;

	case B_FUNCTION_KEY:	
	{
		int32 k;
		Window()->CurrentMessage()->FindInt32( "key", (int32*)&k );
	
			switch (k)
		{
		case B_F1_KEY:		printf( "F1\n" ); break;
		case B_F2_KEY:		printf( "F2\n" ); break;
		case B_F3_KEY:		printf( "F3\n" ); break;
		case B_F4_KEY:		printf( "F4\n" ); break;
		case B_F5_KEY:		printf( "F5\n" ); break;
		case B_F6_KEY:		printf( "F6\n" ); break;
		case B_F7_KEY:		printf( "F7\n" ); break;
		case B_F8_KEY:		printf( "F8\n" ); break;
		case B_F9_KEY:		printf( "F9\n" ); break;
		case B_F10_KEY:		printf( "F10\n" ); break;
		case B_F11_KEY:		printf( "F11\n" ); break;
		case B_F12_KEY:		printf( "F12\n" ); break;
		//case B_PRINT_KEY:	key = XK_Print; break;
		case B_SCROLL_KEY:	printf( "ScrollLock\n" ); break;
		case B_PAUSE_KEY:	printf( "Pause\n" ); break;
		}
		break;
	  }
	}
#endif
	switch (byte)
	{
	case B_BACKSPACE:		key = XK_BackSpace; break;
	case B_DELETE:			key = XK_Delete; break;
	case B_INSERT:			key = XK_Insert; break;

	case B_LEFT_ARROW:		key = XK_Left; break;
	case B_RIGHT_ARROW:		key = XK_Right; break;
	case B_UP_ARROW:		key = XK_Up; break;
	case B_DOWN_ARROW:		key = XK_Down; break;

	//case B_ENTER:			key = XK_Execute; break;
	case B_SPACE:			key = XK_space; break;
	case B_RETURN:			key = XK_Return; break;
	case B_TAB:				key = XK_Tab; break;

	case B_ESCAPE:			key = XK_Escape; break;
	case B_HOME:			key = XK_Home; break;
	case B_END:				key = XK_End; break;
	case B_PAGE_UP:			key = XK_Page_Up; break;
	case B_PAGE_DOWN:		key = XK_Page_Down; break;

	case B_FUNCTION_KEY:
	  {
		int32 k;
		Window()->CurrentMessage()->FindInt32( "key", (int32*)&k );
		switch (k)
		{
		case B_F1_KEY:		key = XK_F1; break;
		case B_F2_KEY:		key = XK_F2; break;
		case B_F3_KEY:		key = XK_F3; break;
		case B_F4_KEY:		key = XK_F4; break;
		case B_F5_KEY:		key = XK_F5; break;
		case B_F6_KEY:		key = XK_F6; break;
		case B_F7_KEY:		key = XK_F7; break;
		case B_F8_KEY:		key = XK_F8; break;
		case B_F9_KEY:		key = XK_F9; break;
		case B_F10_KEY:		key = XK_F10; break;
		case B_F11_KEY:		key = XK_F11; break;
		case B_F12_KEY:		key = XK_F12; break;
		//case B_PRINT_KEY:	key = XK_Print; break;
		case B_SCROLL_KEY:	key = XK_Scroll_Lock; break;
		case B_PAUSE_KEY:	key = XK_Pause; break;
		default:
			return 0;
		}
		break;
	  }

	default:
		key = byte;
		break;
	}
	printf("FINAL: '%c'", (char)key);
	return key;
//	myConnection->SendKeyEvent(key, true);
	
/*	if (dead_key) {
		myConnection->SendKeyEvent(dead_key, false);
		bytes[0] = key;
	}
*/
}

/****
**	@purpose	Send modifier keychange.
**	@param		byte	Be styled key value.
**	@result		Demodified key value.
*/
int
ViewConnection::SendModifier( char *bytes, int len )
{
	uint32	mod		= modifiers();

	if ((mod & B_LEFT_CONTROL_KEY) && !(myOldModifiers & B_LEFT_CONTROL_KEY))
		myConnection->SendKeyEvent( XK_Control_L, true );
	else if (!(mod & B_LEFT_CONTROL_KEY) && (myOldModifiers & B_LEFT_CONTROL_KEY))
		myConnection->SendKeyEvent( XK_Control_L, false );

	if ((mod & B_RIGHT_CONTROL_KEY) && !(myOldModifiers & B_RIGHT_CONTROL_KEY))
		myConnection->SendKeyEvent( XK_Control_R, true );
	else if (!(mod & B_RIGHT_CONTROL_KEY) && (myOldModifiers & B_RIGHT_CONTROL_KEY))
		myConnection->SendKeyEvent( XK_Control_R, false );

	if ((mod & B_LEFT_OPTION_KEY) && !(myOldModifiers & B_LEFT_OPTION_KEY))
		myConnection->SendKeyEvent( XK_Alt_L, true );
	else if (!(mod & B_LEFT_OPTION_KEY) && (myOldModifiers & B_LEFT_OPTION_KEY))
		myConnection->SendKeyEvent( XK_Alt_L, false );

/*	if ((mod & B_RIGHT_OPTION_KEY) && !(myOldModifiers & B_RIGHT_OPTION_KEY))
		myConnection->SendKeyEvent( XK_Alt_R, true );
	else if (!(mod & B_RIGHT_OPTION_KEY) && (myOldModifiers & B_RIGHT_OPTION_KEY))
		myConnection->SendKeyEvent( XK_Alt_R, false );
*/

	if ((len == 1) && (mod & B_CONTROL_KEY))
		bytes[0] += 'a'-1;

	myOldModifiers = mod;

	return len;
}

void print_mods()
{
	uint32 mod;
	
	mod = modifiers();
	printf("[%s %s %s %s %s %s %s %s]\n", 
			(mod & B_LEFT_SHIFT_KEY)?"lS":"  ",
			(mod & B_RIGHT_SHIFT_KEY)?"rS":"  ",
			(mod & B_LEFT_CONTROL_KEY)?"lC":"  ",
			(mod & B_RIGHT_CONTROL_KEY)?"rC":"  ",
			(mod & B_LEFT_OPTION_KEY)?"lO":"  ",
			(mod & B_RIGHT_OPTION_KEY)?"rO":"  ",
			(mod & B_LEFT_COMMAND_KEY)?"lK":"  ",
			(mod & B_RIGHT_COMMAND_KEY)?"rK":"  ");
}

/****
**	@purpose	See BeBook: BView->KeyDown()
*/
void
ViewConnection::KeyDown( const char *bytes, int32 numBytes )
{
	char data[10];
printf("KeyDown(");
for (int i=0; i<numBytes; i++)
	printf("%02x '%c' ", (unsigned char)bytes[i], bytes[i]);
printf(", %li) ", numBytes);
print_mods();
	if( App::GetApp()->GetOptions()->m_ViewOnly )
		return;
//	if (numBytes == 1)
	{
		CARD32 dead_key, key;
//		Window()->CurrentMessage()->PrintToStream();

		memcpy(data, bytes, (numBytes>10)?(10):(numBytes));
//		char byte = SendModifier( bytes[0] );
//		myConnection->SendKeyEvent( GetXKey( data, SendModifier( data, (numBytes>10)?(10):(numBytes) ) ), true );
		key = GetXKey( data, SendModifier( data, (numBytes>10)?(10):(numBytes) ), &dead_key );
		if (dead_key)
			myConnection->SendKeyEvent( dead_key, true );
		myConnection->SendKeyEvent( key, true );
		//printf( "KeyDown: %08lX, %02lX, '%c'\n", GetXKey( byte ), byte, byte );
	}
}


/****
**	@purpose	See BeBook: BView->KeyUp()
*/
void
ViewConnection::KeyUp( const char *bytes, int32 numBytes )
{
	uint32 mod;
	char data[10];
printf("KeyUp(");
for (int i=0; i<numBytes; i++)
	printf("%02x '%c' ", (unsigned char)bytes[i], bytes[i]);
printf(", %li) ", numBytes);
print_mods();
	
	mod = modifiers();
	
	if (numBytes == 1 && bytes[0] == B_ESCAPE && mod & B_CONTROL_KEY && mod & B_SHIFT_KEY) {
		puts("FS");
		Window()->PostMessage(WndConnection::msg_fullscreen);
		return;
	}
	
	if( App::GetApp()->GetOptions()->m_ViewOnly )
		return;

//	if (numBytes == 1)
	{
		CARD32 dead_key, key;
		memcpy(data, bytes, (numBytes>10)?(10):(numBytes));
//		char byte = SendModifier( bytes, numBytes );
//		myConnection->SendKeyEvent( GetXKey( data, SendModifier( data, (numBytes>10)?(10):(numBytes) ) ), false );
		key = GetXKey( data, SendModifier( data, (numBytes>10)?(10):(numBytes) ), &dead_key );
		myConnection->SendKeyEvent( key, false );
		if (dead_key)
			myConnection->SendKeyEvent( dead_key, false );
	}
}

/****
**	@purpose	Send mouse event.
**	@param		point	Current mouse position.
*/
void
ViewConnection::SendMouse( BPoint point )
{
	uint32	buttons;
	int		mask;
	char dummy = 0;
	
	(void)SendModifier( &dummy, 1 );

	Window()->CurrentMessage()->FindInt32( "buttons", (int32*)&buttons ); 

	if (App::GetApp()->IsSwapMouse())
	{
		mask = (((buttons & B_PRIMARY_MOUSE_BUTTON) ? rfbButton1Mask : 0) |
				((buttons & B_SECONDARY_MOUSE_BUTTON) ? rfbButton2Mask : 0) |
				((buttons & B_TERTIARY_MOUSE_BUTTON) ? rfbButton3Mask : 0)  );
	}
	else
	{
		mask = (((buttons & B_PRIMARY_MOUSE_BUTTON) ? rfbButton1Mask : 0) |
				((buttons & B_SECONDARY_MOUSE_BUTTON) ? rfbButton3Mask : 0) |
				((buttons & B_TERTIARY_MOUSE_BUTTON) ? rfbButton2Mask : 0)  );
	}
	myConnection->SendPointerEvent( point.x + hScrollPos, point.y + vScrollPos, mask );
}

void ViewConnection::MouseUp( BPoint point )
{
	if( App::GetApp()->GetOptions()->m_ViewOnly )
		return;

	SendMouse( point );
}

/****
**	@purpose	See BeBook: BView->MouseDown()
*/
void
ViewConnection::MouseDown( BPoint point )
{
	if( App::GetApp()->GetOptions()->m_ViewOnly )
		return;

#if defined(DEBUG_VERBOSE)
	printf( "$Enter MouseDown\n" );
#endif
	myConnection->Lock();
#if defined(DEBUG_VERBOSE)
	printf( "$MouseDown: myConnection locked\n" );
#endif

	// not the elegant way for pasting, but it works
	uint32	buttons;
	Window()->CurrentMessage()->FindInt32( "buttons", (int32*)&buttons ); 
#if 0
//$$$ why the hell doesn't this work? it messes with the whole server clipboard...
	if (buttons & (App::GetApp()->IsSwapMouse() ? B_TERTIARY_MOUSE_BUTTON : B_SECONDARY_MOUSE_BUTTON))
	{
		if (be_clipboard->Lock())
		{
			BMessage* clipper = be_clipboard->Data();
			if (clipper->HasData( "text/plain", B_MIME_TYPE ))
			{
				char*	text;
				ssize_t	len;
				clipper->FindData( "text/plain", (type_code)B_MIME_TYPE, (const void**)&text, &len );
//printf( "clipboard data: '%s'\n", text );
printf( "clipboard data: '");
for (int i=0; i<len; i++)
	printf( "%c", text[i] );
printf( "'\n");
				myConnection->SendClientCutText( text, len );
			}
else
printf( "no text/plain in clipboard\n" );
			be_clipboard->Unlock();
		}
else
printf( "can't lock clipboard\n" );
	}
#endif

//XXX: what's this mess ?
    // Why is this needed?
//	SendMouse( point );
//	while (buttons)
//	{
		SendMouse( point );
#if defined(DEBUG_VERBOSE)
		printf( "$MouseDown: " );
#endif
//		myConnection->DoRFBMessage();
//		GetMouse( &point, &buttons, true );
//	}

//	myConnection->SendPointerEvent( point.x + hScrollPos, point.y + vScrollPos, 0 );
#if defined(DEBUG_VERBOSE)
	printf( "$Last MouseDown: \n" );
#endif
//	myConnection->DoRFBMessage();

	myConnection->Unlock();

#if defined(DEBUG_VERBOSE)
	printf( "$Exit MouseDown\n" );
#endif
}

/****
**	@purpose	See BeBook: BView->MouseMove()
*/
void
ViewConnection::MouseMoved( BPoint point, uint32 transit, const BMessage* /*msg*/ )
{
	if( App::GetApp()->GetOptions()->m_ViewOnly )
		return;

#if defined(DEBUG_VERBOSE)
	printf( "$Enter MouseMoved\n" );
#endif

	if (transit == B_ENTERED_VIEW)
		App::GetApp()->HideCursor();
	else if (transit == B_EXITED_VIEW)
		App::GetApp()->ShowCursor();

	SendMouse( point );

#if defined(DEBUG_VERBOSE)
	printf( "$Exit MouseMoved\n" );
#endif
}

/****
**	@purpose	Handle desktop updates.
*/
void
ViewConnection::ScreenUpdate( void )
{
	bool ok;
#if defined(DEBUG_VERBOSE)
	printf( "$Enter ScreenUpdate\n" );
#endif

	// update our format shortcuts
	UpdateFormat( myConnection->GetFormat() );

	rfbFramebufferUpdateMsg sut;
	ok = myConnection->GetSocket()->ReadExact( (char*)&sut, sz_rfbFramebufferUpdateMsg );
#if defined(DEBUG_VERBOSE)
	printf( "$ScreenUpdate: ReadExact %s\n", ok ? "OK" : "ERROR" );
#endif
	sut.nRects = Swap16IfLE( sut.nRects );

#if defined(DEBUG_VERBOSE)
	printf( "$ScreenUpdate: drawing %d rectangles\n", sut.nRects );
#endif

	// no rectangles available
	if (sut.nRects == 0)
	{
#if defined(DEBUG_VERBOSE)
		printf( "$Exit ScreenUpdate\n" );
#endif
		return;
	}

	// find the bounding region of this batch of updates
	BRegion* fullregion = NULL;

	myBitmap->Lock();
#if defined(DEBUG_VERBOSE)
	printf( "$ScreenUpdate: myBitmap locked\n" );
#endif
	rgb_color	c = myViewOff->HighColor();
	for (uint i = 0; i < sut.nRects; i++)
	{
		rfbFramebufferUpdateRectHeader surh;
		ok = myConnection->GetSocket()->ReadExact( (char*)&surh, sz_rfbFramebufferUpdateRectHeader );
#if defined(DEBUG_VERBOSE)
		printf( "$ScreenUpdate: ReadExact %s; rectangle %d\n", ok ? "OK" : "ERROR", i );
#endif

		surh.r.x		= Swap16IfLE( surh.r.x );
		surh.r.y		= Swap16IfLE( surh.r.y );
		surh.r.w		= Swap16IfLE( surh.r.w );
		surh.r.h		= Swap16IfLE( surh.r.h );
		surh.encoding	= Swap32IfLE( surh.encoding );

		switch (surh.encoding)
		{
		case rfbEncodingRaw:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingRaw\n" );
#endif
			RawRect( &surh );
			break;

		case rfbEncodingCopyRect:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingCopyRect\n" );
#endif
			CopyRect( &surh );
			break;

		case rfbEncodingRRE:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingRRE\n" );
#endif
			RRERect( &surh );
			break;

		case rfbEncodingCoRRE:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingCoRRE\n" );
#endif
			CoRRERect( &surh );
			break;

		case rfbEncodingHextile:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingHextile\n" );
#endif
			HextileRect( &surh );
			break;

		case rfbEncodingZlib:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingZlib\n" );
#endif
			ZlibRect( &surh );
			break;
			
		case rfbEncodingTight:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: rfbEncodingTight\n" );
#endif
			printf("Tight encoding\n"); // XXX: TODO
			break;
			
		default:
#if defined(DEBUG_VERBOSE)
			printf( "$ScreenUpdate: UNKNOWN ENCODING %08lX!\n", surh.encoding );
#endif
			fprintf( stderr, "Unknown encoding %08lX - not supported!\n", surh.encoding );
			goto bailout;
		}

		// either we update the window when all rectangles have been received
#if defined(DEBUG_VERBOSE)
		printf( "$ScreenUpdate: update region\n" );
#endif
		if (App::GetApp()->IsBatchMode())
		{
			if (fullregion == NULL)
			{
				fullregion = new BRegion();
				fullregion->Set( BRect(
					surh.r.x - hScrollPos, surh.r.y - vScrollPos,
					surh.r.x - hScrollPos + surh.r.w, 
					surh.r.y - vScrollPos + surh.r.h )
				);
			}
			else
			{
				fullregion->Include( BRect(
					surh.r.x - hScrollPos, surh.r.y - vScrollPos,
					surh.r.x - hScrollPos + surh.r.w, 
					surh.r.y - vScrollPos + surh.r.h )
				);
			}
		}
		// or we do it for each rectangle
		else
		{
			BRect rect;
			rect.left	= surh.r.x - hScrollPos;
			rect.top	= surh.r.y - vScrollPos;
			rect.right	= rect.left+surh.r.w;
			rect.bottom	= rect.top+surh.r.h;
			myViewOff->Invalidate( rect );
			Invalidate( rect );
		}
	}
bailout:;
	if (App::GetApp()->IsBatchMode())
	{
		Invalidate( fullregion->Frame() );
		delete fullregion;
	}

	myViewOff->SetHighColor( c );

	myBitmap->Unlock();
#if defined(DEBUG_VERBOSE)
	printf( "$ScreenUpdate: myBitmap unlocked\n" );
#endif

#if defined(DEBUG_VERBOSE)
	printf( "$Exit ScreenUpdate\n" );
#endif
}

/****
**	@purpose	Copy the server clipboard text into the local clipboard.
*/
void
ViewConnection::ServerCutText( void )
{
	rfbServerCutTextMsg	sctm;
	myConnection->GetSocket()->ReadExact( (char*)&sctm, sz_rfbServerCutTextMsg );

	int len = Swap32IfLE( sctm.length );

	CheckBufferSize( len );
	if (len == 0)
		myNetBuf[0] = '\0';
	else
	{
		myConnection->GetSocket()->ReadString( myNetBuf, len );
		if (be_clipboard->Lock())
		{
			be_clipboard->Clear(); 
			BMessage* clipper = be_clipboard->Data();
			clipper->AddData( "text/plain", B_MIME_TYPE, myNetBuf, len ); 
			be_clipboard->Commit();
			be_clipboard->Unlock();
		}
	}
}

/****
**	@purpose	Makes sure myNetBuf is at least as big as the specified size.
**	@param		bufsize		Size the buffer must be.
**	@description
**	Makes sure myNetBuf is at least as big as the specified size.
**	Note that myNetBuf itself may change as a result of this call.
**	Throws an exception on failure.
*/
void
ViewConnection::CheckBufferSize( int bufsize )
{
	if (myNetBufSize > bufsize)
		return;

	char* newbuf = new char[bufsize+256];;
	if (newbuf == NULL)
	{
		fprintf( stderr, "Insufficient memory to allocate network buffer.\n" );
		return;
	}

	// only if we're successful...

	if (myNetBuf != NULL)
		delete [] myNetBuf;
	myNetBuf		= newbuf;
	myNetBufSize	= bufsize + 256;
}

/****
**	@purpose	Updates our pixel format shortcuts.
**	@param		format	Pointer to a valid rfbPixelFormat structure.
*/
void
ViewConnection::UpdateFormat( const rfbPixelFormat* format )
{
	// shortcuts to speed things up
	rs = format->redShift;   rm = format->redMax;
	gs = format->greenShift; gm = format->greenMax;
	bs = format->blueShift;  bm = format->blueMax;

	myBitsPerPixel	= format->bitsPerPixel;

	// The number of bytes required to hold at least one pixel.
	minPixelBytes = (format->bitsPerPixel + 7) >> 3;
	assert( minPixelBytes > 0 && minPixelBytes < 7 );	// or something is very wrong!
}

/**************************************************************************
**	Here come the encodings
*/

/*
**	Raw encoding
*/

// This is fun for debugging purposes - it shows all the 'raw' pixels in
// inverse colour. This applies to bits drawn with the Raw encoding or with
// the raw subencoding of hextile.

// #define RAWCOLOURING

#ifdef RAWCOLOURING

#define SETPIXELS(bpp, x, y, w, h)												\
	{																			\
		CARD##bpp *p = (CARD##bpp *) myNetBuf;									\
		for (int k = y; k < y+h; k++) {										\
			for (int j = x; j < x+w; j++) {									\
					COLORREF c = SETHIGHCOLOR_FROM_PIXEL##bpp##_ADDRESS(p);		\
					COLORREF neg = RGB(										\
						255-GetRValue(c),										\
						255-GetGValue(c),										\
						255-GetBValue(c));										\
					SetPixelV(j,k, neg);							\
					p++;														\
			}																	\
		}																		\
	}

#else

#define SETPIXELSBUFFER( buf , bpp , x , y , w , h )				\
	{																\
		CARD##bpp *p = (CARD##bpp *) buf;							\
		for (int k = y; k < y+h; k++) {								\
			for (int j = x; j < x+w; j++) {							\
					SETHIGHCOLOR_FROM_PIXEL##bpp##_ADDRESS(p);		\
					SetPixel(j,k);									\
					p++;											\
			}														\
		}															\
	}
		
#define SETPIXELS(bpp, x, y, w, h)									\
	{																\
		CARD##bpp *p = (CARD##bpp *) myNetBuf;						\
		for (int k = y; k < y+h; k++) {								\
			for (int j = x; j < x+w; j++) {							\
					SETHIGHCOLOR_FROM_PIXEL##bpp##_ADDRESS(p);		\
					SetPixel(j,k);									\
					p++;											\
			}														\
		}															\
	}
 
#endif	 

void
ViewConnection::RawRect(rfbFramebufferUpdateRectHeader *pfburh )
{
	uint numpixels = pfburh->r.w * pfburh->r.h;
    // this assumes at least one byte per pixel. Naughty.
	uint numbytes = numpixels * minPixelBytes;
	// Read in the whole thing
    CheckBufferSize( numbytes );
	myConnection->GetSocket()->ReadExact( myNetBuf, numbytes );

	// This big switch is untidy but fast
	switch (myBitsPerPixel)
	{
	case 8:
		SETPIXELS(8, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)
		break;
	case 16:
		SETPIXELS(16, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)
		break;
	case 24:
	case 32:
		SETPIXELS(32, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)            
		break;
	default:
		fprintf( stderr, "%s: Invalid number of bits per pixel: %d\n", App::GetApp()->GetName(), myBitsPerPixel );
		return;
	}
	
}

/*
**	CopyRect Encoding
*/
void
ViewConnection::CopyRect( rfbFramebufferUpdateRectHeader* pfburh )
{
	rfbCopyRect cr;
	BRect		rect;

	myConnection->GetSocket()->ReadExact( (char*)&cr, sz_rfbCopyRect );

	cr.srcX = Swap16IfLE( cr.srcX );
	cr.srcY = Swap16IfLE( cr.srcY );

	rect.SetLeftTop( BPoint(pfburh->r.x, pfburh->r.y) );
	rect.SetRightBottom( BPoint(rect.left + pfburh->r.w-1, rect.top + pfburh->r.h-1) );
	myViewOff->CopyBits( BRect( cr.srcX, cr.srcY, cr.srcX+rect.Width(), cr.srcY+rect.Height() ), rect );
}

/*
**	RRE (Rising Rectangle Encoding)
*/
void
ViewConnection::RRERect( rfbFramebufferUpdateRectHeader* pfburh )
{
	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char tmpbuf[sz_rfbRREHeader+4];			// biggest pixel is 4 bytes long
	assert( myBitsPerPixel <= 32 );			// check that!
	rfbRREHeader*	prreh	= (rfbRREHeader*)tmpbuf;
	CARD8*			pcolor	= (CARD8*)tmpbuf + sz_rfbRREHeader;

	myConnection->GetSocket()->ReadExact( tmpbuf, sz_rfbRREHeader + minPixelBytes );

	prreh->nSubrects = Swap32IfLE( prreh->nSubrects );

    rgb_color color;
	switch (myBitsPerPixel)
	{
	case 8:
		color = COLOR_FROM_PIXEL8_ADDRESS(pcolor); break;
	case 16:
		color = COLOR_FROM_PIXEL16_ADDRESS(pcolor); break;
	case 24:
	case 32:
		color = COLOR_FROM_PIXEL32_ADDRESS(pcolor); break;
	}

	// Draw the background of the rectangle
	myViewOff->SetHighColor( color );
	myViewOff->FillRect( BRect(pfburh->r.x, pfburh->r.y, pfburh->r.x + pfburh->r.w -1, pfburh->r.y + pfburh->r.h -1) );

	if (prreh->nSubrects == 0)
		return;
	
	// Draw the sub-rectangles
	rfbRectangle rect, *pRect;
	// The size of an RRE subrect including color info
	int subRectSize = minPixelBytes + sz_rfbRectangle;
	
	// Read subrects into the buffer 
	CheckBufferSize( subRectSize * prreh->nSubrects );
	myConnection->GetSocket()->ReadExact( myNetBuf, subRectSize * prreh->nSubrects );
	u8* p = (u8*)myNetBuf;
	
	for (CARD32 i = 0; i < prreh->nSubrects; i++)
	{
		pRect = (rfbRectangle *)(p + minPixelBytes);
		
		switch (myBitsPerPixel)
		{
		case 8:
			color = COLOR_FROM_PIXEL8_ADDRESS(p); break;
		case 16:
			color = COLOR_FROM_PIXEL16_ADDRESS(p); break;
		case 32:
			color = COLOR_FROM_PIXEL32_ADDRESS(p); break;
		};

		// color = COLOR_FROM_PIXEL8_ADDRESS(myNetBuf);
		rect.x = (CARD16)(Swap16IfLE( pRect->x ) + pfburh->r.x);
		rect.y = (CARD16)(Swap16IfLE( pRect->y ) + pfburh->r.y);
		rect.w = Swap16IfLE( pRect->w );
		rect.h = Swap16IfLE( pRect->h );
		myViewOff->SetHighColor( color );
		myViewOff->FillRect( BRect(rect.x, rect.y, rect.x+rect.w-1, rect.y+rect.h-1) );
		p += subRectSize;
    }
}

/*
**	CoRRE Encoding
*/
void
ViewConnection::CoRRERect( rfbFramebufferUpdateRectHeader* pfburh )
{
	bool	ok;
	assert( myBitsPerPixel <= 32 );			// check that!

	// An RRE rect is always followed by a background color
	// For speed's sake we read them together into a buffer.
	char			tmpbuf[sz_rfbRREHeader+4];			// biggest pixel is 4 bytes long
	rfbRREHeader*	prreh	= (rfbRREHeader*)tmpbuf;
	CARD8*			pcolor	= (CARD8*)tmpbuf + sz_rfbRREHeader;

#if defined(DEBUG_VERBOSE)
	printf( "$CoRRERect: 1 reading socket (%d bytes)...", sz_rfbRREHeader + minPixelBytes );
	fflush( stdout );
#endif
	ok = myConnection->GetSocket()->ReadExact( tmpbuf, sz_rfbRREHeader + minPixelBytes );
#if defined(DEBUG_VERBOSE)
	printf( "%s.\n", ok ? "OK" : "ERROR" );
#endif

	prreh->nSubrects = Swap32IfLE( prreh->nSubrects );

	rgb_color color;
	switch (myBitsPerPixel)
	{
	case 8:
		color = COLOR_FROM_PIXEL8_ADDRESS(pcolor);
		break;

	case 16:
		color = COLOR_FROM_PIXEL16_ADDRESS(pcolor);
		break;

	case 32:
		color = COLOR_FROM_PIXEL32_ADDRESS(pcolor);
		break;
	}

	// Draw the background of the rectangle
	myViewOff->SetHighColor( color );
	myViewOff->FillRect( BRect(pfburh->r.x, pfburh->r.y, pfburh->r.x + pfburh->r.w -1, pfburh->r.y+pfburh->r.h-1) );

	if (prreh->nSubrects == 0)
		return;

	// Draw the sub-rectangles
	rfbCoRRERectangle*	pRect;
	rfbRectangle		rect;

	// The size of an CoRRE subrect including color info
	int subRectSize = minPixelBytes + sz_rfbCoRRERectangle;

	// Read subrects into the buffer 
	CheckBufferSize( subRectSize * prreh->nSubrects );
#if defined(DEBUG_VERBOSE)
	printf( "$CoRRERect: 2 reading socket (subRectSize=%d, prreh->nSubrects=%d)...", subRectSize, prreh->nSubrects );
	fflush( stdout );
#endif
	ok = myConnection->GetSocket()->ReadExact( myNetBuf, subRectSize * prreh->nSubrects );
#if defined(DEBUG_VERBOSE)
	printf( "%s.\n", ok ? "OK" : "ERROR" );
#endif

	u8* p = (u8*)myNetBuf;

	for (CARD32 i = 0; i < prreh->nSubrects; i++)
	{
		pRect = (rfbCoRRERectangle*) (p + minPixelBytes);
		
		switch (myBitsPerPixel)
		{
		case 8:
			color = COLOR_FROM_PIXEL8_ADDRESS(p);
			break;

		case 16:
			color = COLOR_FROM_PIXEL16_ADDRESS(p);
			break;

		case 32:
			color = COLOR_FROM_PIXEL32_ADDRESS(p);
			break;
		};
		
		// color = COLOR_FROM_PIXEL8_ADDRESS(myNetBuf);
		rect.x = pRect->x + pfburh->r.x;
		rect.y = pRect->y + pfburh->r.y;
		rect.w = pRect->w;
		rect.h = pRect->h;

		myViewOff->SetHighColor( color );
		myViewOff->FillRect( BRect(rect.x, rect.y, rect.x+rect.w-1, rect.y+rect.h-1 ) );

		p += subRectSize;
    }
}

/*
**	ZLib Encoding.
*/
#include <zlib.h>

void ViewConnection::ZlibRect(rfbFramebufferUpdateRectHeader *pfburh) 
{
	unsigned int numpixels = pfburh->r.w * pfburh->r.h;
    // this assumes at least one byte per pixel. Naughty.
	unsigned int numRawBytes = numpixels * minPixelBytes;
	unsigned int numCompBytes;
	unsigned int inflateResult;

	rfbZlibHeader hdr;

	// Read in the rfbZlibHeader
	myConnection->GetSocket()->ReadExact((char *)&hdr, sz_rfbZlibHeader);

	numCompBytes = Swap32IfLE(hdr.nBytes);

	// Read in the compressed data
    CheckBufferSize(numCompBytes);
	myConnection->GetSocket()->ReadExact(myNetBuf, numCompBytes);

	// Verify enough buffer space for screen update.
	CheckZlibBufferSize(numRawBytes);

	m_decompStream.next_in = (unsigned char *)myNetBuf;
	m_decompStream.avail_in = numCompBytes;
	m_decompStream.next_out = m_zlibbuf;
	m_decompStream.avail_out = numRawBytes;
	m_decompStream.data_type = Z_BINARY;
		
	// Insure the inflator is initialized
	if ( m_decompStreamInited == false ) {
		m_decompStream.total_in = 0;
		m_decompStream.total_out = 0;
		m_decompStream.zalloc = Z_NULL;
		m_decompStream.zfree = Z_NULL;
		m_decompStream.opaque = Z_NULL;

		inflateResult = inflateInit( &m_decompStream );
		if ( inflateResult != Z_OK ) {
			fprintf( stderr, "%s: zlib inflate error: %d\n", App::GetApp()->GetName(), inflateResult );
			return;
		}
		m_decompStreamInited = true;
	}

	// Decompress screen data
	inflateResult = inflate( &m_decompStream, Z_SYNC_FLUSH );
	if ( inflateResult < 0 ) {
		fprintf( stderr, "%s: zlib inflate error: %d\n", App::GetApp()->GetName(), inflateResult );//log.Print(0, _T("zlib inflate error: %d\n"), inflateResult);
		return;
	}

	switch (myBitsPerPixel) 
	{
	case 8:
		SETPIXELSBUFFER( m_zlibbuf, 8, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)
			break;
	case 16:
		SETPIXELSBUFFER( m_zlibbuf, 16, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)
			break;
	case 24:
	case 32:
		SETPIXELSBUFFER( m_zlibbuf, 32, pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h)            
			break;
	default:
		fprintf( stderr, "%s: Invalid number of bits per pixel: %d\n", App::GetApp()->GetName(), myBitsPerPixel );
		return;
	}
}

// Makes sure zlibbuf is at least as big as the specified size.
// Note that zlibbuf itself may change as a result of this call.
// Throws an exception on failure.
void ViewConnection::CheckZlibBufferSize(int bufsize)
{
	unsigned char *newbuf;

	if (m_zlibbufsize > bufsize) return;


	newbuf = (unsigned char *)new char[bufsize+256];
	if (newbuf == NULL) 
	{
		fprintf( stderr, "Insufficient memory to allocate zlib buffer.");
		return;
	}

	// Only if we're successful...

	if (m_zlibbuf != NULL)
		delete [] m_zlibbuf;
	m_zlibbuf = newbuf;
	m_zlibbufsize=bufsize + 256;
//	log.Print(4, _T("zlibbufsize expanded to %d\n"), m_zlibbufsize);

}

/*
**	Hextile Encoding.
*/
void
ViewConnection::HextileRect( rfbFramebufferUpdateRectHeader* pfburh )
{
	switch (myBitsPerPixel)
	{
	case 8:
		HandleHextileEncoding8( pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h );
		break;
	case 16:
		HandleHextileEncoding16( pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h );
		break;
	case 32:
		HandleHextileEncoding32( pfburh->r.x, pfburh->r.y, pfburh->r.w, pfburh->r.h );
		break;
	}
}

#define DEFINE_HEXTILE(bpp)															\
void ViewConnection::HandleHextileEncoding##bpp(int rx, int ry, int rw, int rh)		\
{																					\
	rgb_color	bgcolor, fgcolor;													\
    CARD##bpp	bg, fg;																\
    CARD8*		ptr;																\
    int			i;																	\
    int			x, y;																\
    uint		w, h;																\
    int			sx, sy;																\
    uint		sw, sh;																\
    CARD8		subencoding;														\
    CARD8		nSubrects;															\
    /*bool		ok;*/\
																					\
    CheckBufferSize( 16 * 16 * bpp );												\
																					\
    for (y = ry; y < ry+rh; y += 16) {												\
        for (x = rx; x < rx+rw; x += 16) {											\
            w = h = 16;																\
            if (rx+rw - x < 16)														\
                w = rx+rw - x;														\
            if (ry+rh - y < 16)														\
                h = ry+rh - y;														\
																					\
			/*ok =*/ myConnection->GetSocket()->ReadExact( (char*)&subencoding, 1 );			\
			/*printf( "$HandleHextileEncoding: 1 ReadExact %s\n", ok ? "OK" : "ERROR" );*/\
																					\
            if (subencoding & rfbHextileRaw) {										\
                /*ok =*/ myConnection->GetSocket()->ReadExact( myNetBuf, w * h * (bpp/8) );	\
			/*printf( "$HandleHextileEncoding: 1.1 ReadExact %s\n", ok ? "OK" : "ERROR" );*/\
                SETPIXELS( bpp, x,y,w,h )											\
                continue;															\
            }																		\
																					\
		    if (subencoding & rfbHextileBackgroundSpecified) {						\
                /*ok =*/ myConnection->GetSocket()->ReadExact( (char*)&bg, (bpp/8) );		\
			/*printf( "$HandleHextileEncoding: 1.2 ReadExact %s\n", ok ? "OK" : "ERROR" );*/\
				bgcolor = COLOR_FROM_PIXEL##bpp##_ADDRESS(&bg);						\
			}																		\
			myViewOff->SetHighColor( bgcolor );										\
			myViewOff->FillRect( BRect(x,y,x+w-1,y+h-1) );							\
																					\
            if (subencoding & rfbHextileForegroundSpecified)  {						\
                /*ok =*/ myConnection->GetSocket()->ReadExact( (char*)&fg, (bpp/8) );		\
			/*printf( "$HandleHextileEncoding: 1.2 ReadExact %s\n", ok ? "OK" : "ERROR" );*/\
				fgcolor = COLOR_FROM_PIXEL##bpp##_ADDRESS(&fg);						\
			}																		\
																					\
            if (!(subencoding & rfbHextileAnySubrects)) {							\
                continue;															\
            }																		\
																					\
            /*ok =*/ myConnection->GetSocket()->ReadExact( (char*)&nSubrects, 1 );			\
			/*printf( "$HandleHextileEncoding: 2 ReadExact %s\n", ok ? "OK" : "ERROR" );*/\
																					\
            ptr = (CARD8*)myNetBuf;													\
																					\
            if (subencoding & rfbHextileSubrectsColoured) {							\
																					\
                /*ok =*/ myConnection->GetSocket()->ReadExact( myNetBuf, nSubrects * (2 + (bpp/8)) ); \
			/*printf( "$HandleHextileEncoding: 3 ReadExact %s; nSubrects = %d\n", ok ? "OK" : "ERROR", nSubrects );*/\
																					\
                for (i = 0; i < nSubrects; i++) {									\
                    fgcolor = COLOR_FROM_PIXEL##bpp##_ADDRESS(ptr);					\
					ptr	+= (bpp/8);													\
                    sx = ((u8)(*ptr)) >> 4;											\
                    sy = ((u8)(*ptr++)) & 0x0f;										\
                    sw = (((u8)(*ptr)) >> 4);										\
                    sh = (((u8)(*ptr++)) & 0x0f);									\
                    myViewOff->SetHighColor( fgcolor );								\
                    myViewOff->FillRect( BRect(x+sx, y+sy, x+sx+sw, y+sy+sh) );		\
                }																	\
																					\
            } else {																\
                /*ok =*/ myConnection->GetSocket()->ReadExact( myNetBuf, nSubrects * 2 );	\
			/*printf( "$HandleHextileEncoding: 4 ReadExact %s; nSubrects = %d\n", ok ? "OK" : "ERROR", nSubrects );*/\
																					\
                myViewOff->SetHighColor( fgcolor );									\
                for (i = 0; i < nSubrects; i++) {									\
                    sx = ((u8)(*ptr)) >> 4;											\
                    sy = ((u8)(*ptr++)) & 0x0f;										\
                    sw = (((u8)(*ptr)) >> 4);										\
                    sh = (((u8)(*ptr++)) & 0x0f);									\
                    myViewOff->FillRect( BRect(x+sx, y+sy, x+sx+sw, y+sy+sh) );		\
                }																	\
            }																		\
        }																			\
    }																				\
																					\
}

DEFINE_HEXTILE(8)
DEFINE_HEXTILE(16)
DEFINE_HEXTILE(32)
