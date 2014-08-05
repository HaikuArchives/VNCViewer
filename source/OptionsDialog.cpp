/*
**
**	$Filename: OptionsDialog.cpp $
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

#include <Application.h>
#include <InterfaceKit.h>
#include "OptionsDialog.h"

#include "App.h"
#include "VNCOptions.h"

OptionsDialog::OptionsDialog(BMessage *archive, VNCOptions* pOptions)
			  :BWindow(archive)
{
	m_pOptions = pOptions;
	
	BCheckBox* shareSession = (BCheckBox *)FindView("sharedSession");
	shareSession->SetValue( m_pOptions->m_Shared );
	
	BCheckBox* deIconifyOnBell = (BCheckBox *)FindView("deiconifyonbell");
	deIconifyOnBell->SetValue( m_pOptions->m_DeiconifyOnBell );
	
	BCheckBox* disableClipboard = (BCheckBox *)FindView("disableClipboard");
	disableClipboard->SetValue( m_pOptions->m_DisableClipboard );
	
	BCheckBox* emulatethreemouse = (BCheckBox *)FindView("emulate3with2");
	emulatethreemouse->SetValue( m_pOptions->m_Emul3Buttons );
	
	BCheckBox* swap2and3 = (BCheckBox *)FindView("swap2and3");
	swap2and3->SetValue( m_pOptions->m_SwapMouse );
	
	BCheckBox* restrict8bit = (BCheckBox *)FindView("restrictto8bit");
	restrict8bit->SetValue( m_pOptions->m_Use8Bit );
	
	BCheckBox* viewOnly = (BCheckBox *)FindView("viewOnly");
	viewOnly->SetValue( m_pOptions->m_ViewOnly );
	
	BCheckBox* fullscreen = (BCheckBox *)FindView("fullscreen");
	fullscreen->SetValue( m_pOptions->m_FullScreen );
	
	BCheckBox* copyRect = (BCheckBox *)FindView("copyRect");
	copyRect->SetValue( m_pOptions->m_UseEnc[rfbEncodingCopyRect] );

	BRadioButton* pButton = NULL;
	
	switch( m_pOptions->m_PreferredEncoding )
	{	
		case rfbEncodingRaw:
			pButton = (BRadioButton *)FindView("rawencoding");
			break;		
		case rfbEncodingRRE:
			pButton = (BRadioButton *)FindView("rreencoding");
			break;		
		case rfbEncodingCoRRE:
			pButton = (BRadioButton *)FindView("correencoding");
			break;		
		case rfbEncodingHextile:
			pButton = (BRadioButton *)FindView("hextile");
			break;	
		case rfbEncodingZlib:
			pButton = (BRadioButton *)FindView("zlibencoding");
			break;
		case rfbEncodingTight:
			pButton = (BRadioButton *)FindView("tightEncoding");
			break;
	}
	
	if( pButton )
		pButton->SetValue(1);
}

OptionsDialog::~OptionsDialog()
{


}

void OptionsDialog::_SaveSettings()
{
	BCheckBox* shareSession = (BCheckBox *)FindView("sharedSession");
	m_pOptions->m_Shared = shareSession->Value();
	
	BCheckBox* deIconifyOnBell = (BCheckBox *)FindView("deiconifyonbell");
	 m_pOptions->m_DeiconifyOnBell = deIconifyOnBell->Value( );
	
	BCheckBox* disableClipboard = (BCheckBox *)FindView("disableClipboard");
	m_pOptions->m_DisableClipboard = disableClipboard->Value(  );
	
	BCheckBox* emulatethreemouse = (BCheckBox *)FindView("emulate3with2");
	m_pOptions->m_Emul3Buttons = emulatethreemouse->Value( );
	
	BCheckBox* swap2and3 = (BCheckBox *)FindView("swap2and3");
	m_pOptions->m_SwapMouse = swap2and3->Value( );
	
	BCheckBox* restrict8bit = (BCheckBox *)FindView("restrictto8bit");
	m_pOptions->m_Use8Bit = restrict8bit->Value();
	
	BCheckBox* viewOnly = (BCheckBox *)FindView("viewOnly");
	m_pOptions->m_ViewOnly = viewOnly->Value(  );
	
	BCheckBox* fullscreen = (BCheckBox *)FindView("fullscreen");
	m_pOptions->m_FullScreen = fullscreen->Value();
	
	BCheckBox* copyRect = (BCheckBox *)FindView("copyRect");
	m_pOptions->m_UseEnc[rfbEncodingCopyRect] = copyRect->Value();

	BRadioButton* pButton = NULL;
	
	pButton = (BRadioButton *)FindView("rawencoding");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingRaw;
				
	pButton = (BRadioButton *)FindView("rreencoding");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingRRE;

	pButton = (BRadioButton *)FindView("correencoding");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingCoRRE;
	
	pButton = (BRadioButton *)FindView("hextile");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingHextile;	
		
	pButton = (BRadioButton *)FindView("zlibencoding");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingZlib;	

	pButton = (BRadioButton *)FindView("tightEncoding");
	if( pButton->Value() )
		m_pOptions->m_PreferredEncoding = rfbEncodingTight;	
}

bool OptionsDialog::QuitRequested()
{
	return true;
}

void OptionsDialog::MessageReceived(BMessage *pMsg)
{
	switch(pMsg->what)
	{
		case 'okay':
			_SaveSettings();
			Quit();
			break;
			
		default:
			BWindow::MessageReceived(pMsg);
			break;
	}
}


// Convert "host:display" into host and port
// Returns true if valid format, false if not.
// Takes initial string, addresses of results and size of host buffer in wchars.
// If the display info passed in is longer than the size of the host buffer, it
// is assumed to be invalid, so false is returned.

#include <string.h>
#include <stdio.h>
bool ParseDisplay(char* display, char* phost, int hostlen, int *pport) 
{
    int tmpport;
    if (hostlen < (int) strlen(display))
        return false;
    char *colonpos = strchr( display, ':' );
    if (colonpos == NULL) {
        tmpport = 0;
		strncpy(phost, display, 64); // 64 == maxhostlen in netdb.h
	} else {
		strncpy(phost, display, colonpos-display);
		phost[colonpos-display] = L'\0';
		if (sscanf(colonpos+1, ("%d"), &tmpport) != 1) 
			return false;
	}
    if (tmpport < 100)
        tmpport += SERVERPORT;
    *pport = tmpport;
    return true;
}
