/*
**
**	$Filename: AuthDialog.cpp $
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

#include <Window.h>
#include <TextControl.h>

#include <string.h>
#include <stdlib.h>

#include "AuthDialog.h"

AuthenticationDialog::AuthenticationDialog(BMessage *archive)
					 :BWindow(archive)
{
	BTextControl* pass = (BTextControl *)FindView("passText");
	if(pass)
		pass->MakeFocus(true);
		
}

AuthenticationDialog::~AuthenticationDialog()
{


}

bool AuthenticationDialog::QuitRequested()
{
	BWindow::QuitRequested();

	delete_sem(modalSem);

	nRes = 0;
	
	return false;
}

int AuthenticationDialog::DoModal()
{
	Show();
	
	modalSem = create_sem(0,"AuthSem");
	WaitForDelete(modalSem);
	
	Hide();
	
	return nRes;
}

const char* AuthenticationDialog::GetPassText()
{
	return (const char *)&passText;
}

void AuthenticationDialog::MessageReceived(BMessage* pMsg)
{
	switch(pMsg->what)
	{
		case 'okay':
			{
				BTextControl* pass = (BTextControl *)FindView("passText");
				if( pass )
				{
					strncpy( passText , pass->Text() , 256 );
				}
				else
					passText[0] = '\0';
					
				nRes = 1;
				delete_sem(modalSem);
			}
			break;
			
		default:
			BWindow::MessageReceived(pMsg);
			return;
	}
}


status_t AuthenticationDialog::WaitForDelete(sem_id blocker)
{
	status_t	result;
	thread_id	this_tid = find_thread(NULL);
	BLooper		*pLoop;
	BWindow		*pWin = 0;

	pLoop = BLooper::LooperForThread(this_tid);
	if (pLoop)
		pWin = dynamic_cast<BWindow*>(pLoop);

	// block until semaphore is deleted (modal is finished)
	if (pWin) {
		do {
			pWin->Unlock(); // Who will know?=)
			snooze(100);
			pWin->Lock();
			
			// update the window periodically			
			pWin->UpdateIfNeeded();
			result = acquire_sem_etc(blocker, 1, B_TIMEOUT, 1000);
		} while (result != B_BAD_SEM_ID);
	} else {
		do {
			// just wait for exit
			result = acquire_sem(blocker);
		} while (result != B_BAD_SEM_ID);
	}
	return result;
}
