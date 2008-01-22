#if !defined(WND_LOGIN_H)
#define WND_LOGIN_H
/*
**
**	$Id: WndLogin.h,v 1.4 1998/10/25 16:05:53 bobak Exp $
**	$Revision: 1.4 $
**	$Filename: WndLogin.h $
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
#include <Window.h>
#include "PassControl.h"

class WndLogin : public BWindow  
{
protected:

	// @constructors
	WndLogin( BRect frame );

public:

	// @destructor
	~WndLogin( void );

	// @methods{Public Methods}
	static WndLogin* Create( void );

protected:

	enum msg	// message types for this window
	{
		msg_ok	= B_SPECIFIERS_END,		// 'OK' button pressed
		msg_cancel,						// 'Cancel' button pressed
		msg_hostname,					// 'Hostname:' text changed
		msg_passwd,						// 'Password:' text changed

		msg_invalid						// no valid messages
	};

	// @methods{Helpers}
	BTextControl* AddTextCtrl( BView* parent, BRect& rect, char* id, char* label, msg mesg, float divider, bool passwd = false );
	BButton* AddButton( BView* parent, BRect& rect, char* id, char* label, msg mesg );

	// @methods{Handlers}
	virtual void MessageReceived( BMessage* );

private:

	// member objects
	static const char*	title;				// window title
	static const float	width;				// window width
	static const float	height;				// window height

	BButton*		myBtnOK;				// 'OK' button
	BButton*		myBtnCancel;			// 'Cancel' button
	BTextControl*	myTxtCtrlHostname;		// 'Hostname:' text field
	BTextControl*	myTxtCtrlLogin;			// 'Login:' text field
	PassControl*	myPassCtrlPassword;		// 'Password:' text field
};

#endif
