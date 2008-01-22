#ifndef APP_H
#define APP_H
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
#include <Application.h>
#include <Message.h>

#include "VNCOptions.h"
#include "rfbproto.h"

#define MAX_ENCODINGS		10
#define FLASHPORT  (5400)    /* Offset to listen for `flash' commands */
#define CLIENTPORT (5500)    /* Offset to listen for reverse connections */
#define SERVERPORT (5900)    /* Offset to server for regular connections */

class Connection;	// forward definition

class App : public BApplication 
{
public:

	enum msg
	{
		msg_window_closed = 128,
		msg_connect,
		msg_send_ctrlaltdel,
		msg_invalid
	};

	// @constructors
	App( void );

	// @destructor
	~App( void );

	// @methods(Selector)
	static App* GetApp( void )				{ return (App*)be_app; }
	const char* GetName( void ) const 		{ return (const char*)myAppName; }
	int GetExplicitEnc( void ) const		{ return myExplicitEnc; }
	CARD32* GetEncodings( void )			{ return myExplicitEncodings; }

	VNCOptions* GetOptions( void )			{return &m_opts; }
	
	bool AddCopyRect( void ) const			{ return myAddCopyRect; }
	bool AddHextile( void ) const			{ return myAddHextile; }
	bool AddCoRRE( void ) const				{ return myAddCoRRE; }
	bool AddRRE( void ) const				{ return myAddRRE; }
	bool IsBatchMode( void ) const			{ return myIsBatchMode; }
//XXX:useless	bool IsLittleEndian( void ) const		{ return myIsLittleEndian; }
	bool IsListenSpecified( void ) const	{ return myIsListenSpecified; }
	bool IsShareDesktop( void ) const		{ return m_opts.m_Shared;}//myIsShareDesktop; }
	bool IsSwapMouse( void ) const			{ return m_opts.m_SwapMouse; }
	bool IsUse8bit( void ) const			{ return m_opts.m_Use8Bit;}//return myIsUse8bit; }

	// @methods(Member Functions)
	static void Alert( const char* body, const char* arg = NULL );

	// @methods(Handlers)
	virtual void AboutRequested( void );
	virtual void MessageReceived( BMessage* msg );

private:

	// @members
	const char*	myAppName;
	Connection*	myConnection;

	// settings
	CARD32	myExplicitEncodings[MAX_ENCODINGS];
	int		myExplicitEnc;
	bool	myAddCopyRect;
	bool	myAddHextile;
	bool	myAddCoRRE;
	bool	myAddRRE;
	bool	myIsBatchMode;
//XXX:useless	bool	myIsLittleEndian;
	bool	myIsListenSpecified;
	bool	myIsShareDesktop;
	bool	myIsSwapMouse;
	bool	myIsUse8bit;
	
	VNCOptions m_opts;
};

#endif
