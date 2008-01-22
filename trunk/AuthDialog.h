/*
**
**	$Filename: AuthDialog.h $
**	$Date: 2001/1/3 14:46:44 $
**
**  Author: Christopher J. Plymire (chrisjp@eudoramail.com)
**
**	Copyright (C) 2001 by Christopher J. Plymire
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

#ifndef CJP_AUTHDIALOG_H___
#define CJP_AUTHDIALOG_H___

#include <OS.h>

#define AUTHDIALOG_CLOSE 'aucl'

class AuthenticationDialog : public BWindow
{
public:
	AuthenticationDialog(BMessage *archive);
	virtual ~AuthenticationDialog();
	
	virtual void MessageReceived(BMessage *pMsg);
	virtual bool QuitRequested();

	int DoModal();
	const char* GetPassText();
		
private:
	int nRes; // result that should be returned from DoModal()
	char passText[256];

	sem_id modalSem;
	status_t WaitForDelete(sem_id blocker);
};

#endif
