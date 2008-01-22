/*
**
**	$Id: App.cpp,v 1.2 1998/04/19 14:46:44 bobak Exp $
**	$Revision: 1.2 $
**	$Filename: App.cpp $
**	$Date: 1998/04/19 14:46:44 $
**
**
**	Original Author: Andreas F. Bobak (bobak@abstrakt.ch)
**  Modified by: Christopher J. Plymire (chrisjp@eudoramail.com)
**
**	Copyright (C) 1998 by Abstrakt SEC, Andreas F. Bobak.
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

#include <MessageFilter.h>
#include <Application.h>
#include <FindDirectory.h>
#include <InterfaceKit.h>
#include <Path.h>
#include <stdio.h>
#include <memory.h>

#include "App.h"
#include "Connection.h"
#include "WndLogin.h"

// C-A-D stuff
#include "keysymdef.h"

// this is to get the application to run
int main( void ) { App app; app.Run(); return 0; }

#define PREFS_FILE "vnc.viewersettings"

/*****************************************************************************
**	App
*/

/****
**	@purpose	Constructs a new App instance.
*/
App::App( void )
	: BApplication("application/x-vnd.Abstrakt-VNCviewer"),
	  myAppName("VNCviewer"),
	  myConnection(NULL),
	  myAddCopyRect(true),
	  myAddHextile(true),
	  myAddCoRRE(false),
	  myAddRRE(false),
	  myIsBatchMode(false),
//	  myIsLittleEndian(true),
	  myIsListenSpecified(false),
	  myIsShareDesktop(false),
	  myIsUse8bit(false),
	  myExplicitEnc(0)
{
	BPath path;
	
	// Load our settings from the file if possible.
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		return;
	}
	
	path.Append(PREFS_FILE);
	m_opts.Load( (char *)path.Path() );
	

	memset( myExplicitEncodings, 0, sizeof(myExplicitEncodings) );
	WndLogin::Create();
}

/****
**	@purpose	Destroys an existing App instance.
*/
App::~App( void )
{
}


/****
**	@purpose	Shows a simple BeAlert.
**	@param		body	Text to show.
**	@param		arg		passed to sprintf.
*/
void
App::Alert( const char* body, const char* arg )
{
	char buf[256];
	sprintf( buf, body, arg );
	BAlert* alert = new BAlert( App::GetApp()->GetName(), buf, "OK" );
	App::GetApp()->ShowCursor();
	alert->Go();
}

/****
**	@purpose	See the BeBook.
*/
void
App::AboutRequested( void )
{
	char about_text[] =
	"VNCviewer for BeOS\nVersion 3.3.1, Release 6\n\n"
	"VNC is Copyright (C) by Olivetti & Oracle Research Laboratory.\n"
	"VNCviewer for BeOS is Copyright (C) by Abstrakt Design, Andreas F. Bobak,\n"
	"Christopher J. Plymire (chrisjp@eudoramail.com),\n"
	"François Revol (revol@free.fr).\n\n"//XXX: add
	"This software is distributed under the GNU General Public Licence as "
	"published by the Free Software Foundation.\n\n";
//	"http://www.abstrakt.ch/be/";

	BAlert* alert = new BAlert( App::GetApp()->GetName(), about_text, "OK" );
	App::GetApp()->ShowCursor();
	alert->Go();
}



bool ParseDisplay(char* display, char* phost, int hostlen, int *pport) ;

/****
**	@purpose	See the BeBook.
**	@param		msg		A pointer to the received message.
*/
void
App::MessageReceived( BMessage* msg )
{
	switch (msg->what)
	{
	case msg_connect:
	  {
		char	*host;// *passwd;
		char	hostname[256];
		int		port;

		msg->FindString( "hostname", (const char **)&host );
//		msg->FindString( "password", (const char **)&passwd );

		if (ParseDisplay( host , hostname , 256 , &port ) )
		{
			BPath path;
	
			// Save our settings to the file if possible.
			status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
			if (status != B_OK) {
				return;
			}
	
			path.Append(PREFS_FILE);
			m_opts.AddConnectionItem( host );
			m_opts.Save( (char *)path.Path() );
	
			myConnection = new Connection();
			if (myConnection->Connect( hostname, port ))
			{
				// OK
			}
			else
			{
				delete myConnection;
				myConnection = NULL;
				PostMessage( B_QUIT_REQUESTED );
			}
		}
		else
		{
			char	buf[256];
			sprintf( buf, "'%s' doesn't specify a hostname + port number.\n", host );
			BAlert* alert = new BAlert( App::GetName(), buf, "OK" );
			alert->Go();
			PostMessage( B_QUIT_REQUESTED );
		}
		break;
	  }

	case msg_window_closed:
		ShowCursor();
		if (myConnection != NULL)
			myConnection->PostMessage( B_QUIT_REQUESTED );
		be_app->PostMessage( B_QUIT_REQUESTED );
		break;

	case msg_send_ctrlaltdel:
		if (myConnection != NULL)
		{
			myConnection->SendKeyEvent( XK_Control_L, true );
			myConnection->SendKeyEvent( XK_Alt_L, true );
			myConnection->SendKeyEvent( XK_Delete, true );
			myConnection->SendKeyEvent( XK_Delete, false );
			myConnection->SendKeyEvent( XK_Alt_L, false );
			myConnection->SendKeyEvent( XK_Control_L, false );
			
			// XK_Terminate_Server
//			myConnection->SendKeyEvent( 0x0FED5, true );
//			myConnection->SendKeyEvent( 0x0FED5, false );
			
		}
		break;

	default:
		BApplication::MessageReceived( msg );
		break;
	}
}
