//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

// Modified by: Christopher J. Plymire (chrisjp@eudoramail.com)

// VNCOptions.cpp: implementation of the VNCOptions class.

#include <string.h>
#include <File.h>
#include <Alert.h>
#include <Application.h>
#include <StorageDefs.h>
#include <stdio.h>

#include "App.h"//#include "rfb.h"
#include "VNCOptions.h"

#include "Utility.h"
#include "OptionsDialog.h"

bool ParseDisplay(char* display, char* phost, int hostlen, int *port);
//#include "Exception.h"

VNCOptions::VNCOptions()
{
	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++)
		m_UseEnc[i] = true;
	
	m_ViewOnly = false;
	m_FullScreen = false;
	m_Use8Bit = false;

	m_PreferredEncoding = rfbEncodingHextile;

	m_SwapMouse = false;
	m_Emul3Buttons = true;
	m_Emul3Timeout = 100; // milliseconds
	m_Emul3Fuzz = 4;      // pixels away before emulation is cancelled
	m_Shared = false;
	m_DeiconifyOnBell = false;
	m_DisableClipboard = false;
	m_localCursor = DOTCURSOR;
	m_scaling = false;
	m_scale_num = 1;
	m_scale_den = 1;
	
	m_host[0] = '\0';
	m_port = -1;
	
	m_kbdname[0] = '\0';
	m_kbdSpecified = false;
	
	m_logLevel = 0;
	m_logToConsole = false;
	m_logToFile = false;
	m_logFilename[0] = '\0';
	
	m_delay=0;
	m_connectionSpecified = false;
	m_configSpecified = false;
	m_configFilename[0] = '\0';
	m_listening = false;
	m_restricted = false;
}

VNCOptions& VNCOptions::operator=(VNCOptions& s)
{
	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++)
		m_UseEnc[i] = s.m_UseEnc[i];
	
	m_ViewOnly			= s.m_ViewOnly;
	m_FullScreen		= s.m_FullScreen;
	m_Use8Bit			= s.m_Use8Bit;
	m_PreferredEncoding = s.m_PreferredEncoding;
	m_SwapMouse			= s.m_SwapMouse;
	m_Emul3Buttons		= s.m_Emul3Buttons;
	m_Emul3Timeout		= s.m_Emul3Timeout;
	m_Emul3Fuzz			= s.m_Emul3Fuzz;      // pixels away before emulation is cancelled
	m_Shared			= s.m_Shared;
	m_DeiconifyOnBell	= s.m_DeiconifyOnBell;
	m_DisableClipboard  = s.m_DisableClipboard;
	m_scaling			= s.m_scaling;
	m_scale_num			= s.m_scale_num;
	m_scale_den			= s.m_scale_den;
	m_localCursor		= s.m_localCursor;
	
	strcpy(m_host, s.m_host);
	m_port				= s.m_port;
	
	strcpy(m_kbdname, s.m_kbdname);
	m_kbdSpecified		= s.m_kbdSpecified;
	
	m_logLevel			= s.m_logLevel;
	m_logToConsole		= s.m_logToConsole;
	m_logToFile			= s.m_logToFile;
	strcpy(m_logFilename, s.m_logFilename);

	m_delay				= s.m_delay;
	m_connectionSpecified = s.m_connectionSpecified;
	m_configSpecified   = s.m_configSpecified;
	strcpy(m_configFilename, s.m_configFilename);

	m_listening			= s.m_listening;
	m_restricted		= s.m_restricted;
	
	return *this;
}

VNCOptions::~VNCOptions()
{
	
}

inline bool SwitchMatch(char* arg, char* swtch) {
	return (arg[0] == '-' || arg[0] == '/') &&
		(strcasecmp(&arg[1], swtch) == 0);
}

static void ArgError(char* msg) {
    BAlert* pAlert = new BAlert("VNCViewer - Error" , msg , "OK"); //// MessageBox(NULL,  msg, _T("Argument error"),MB_OK | MB_TOPMOST | MB_ICONSTOP);
	pAlert->Go();
}

// Greatest common denominator, by Euclid
int gcd(int a, int b) {
	if (a < b) return gcd(b,a);
	if (b == 0) return a;
	return gcd(b, a % b);
}

void VNCOptions::FixScaling() {
	if (m_scale_num < 1 || m_scale_den < 1) 
	{
		BAlert* pAlert = new BAlert( "VNCViewer - Error" , "Invalid scale factor - resetting to normal scale" , "OK" );//MessageBox(NULL,  _T("Invalid scale factor - resetting to normal scale"), 
		pAlert->Go();
			//_T("Argument error"),MB_OK | MB_TOPMOST | MB_ICONWARNING);
		m_scale_num = 1;
		m_scale_den = 1;	
		m_scaling = false;
	}
	int g = gcd(m_scale_num, m_scale_den);
	m_scale_num /= g;
	m_scale_den /= g;	
}

void VNCOptions::SetFromCommandLine(char* szCmdLine) {
	// We assume no quoting here.
	// Copy the command line - we don't know what might happen to the original
	int cmdlinelen = strlen(szCmdLine);
	if (cmdlinelen == 0) return;
	
	char *cmd = new char[cmdlinelen + 1];
	strcpy(cmd, szCmdLine);
	
	// Count the number of spaces
	// This may be more than the number of arguments, but that doesn't matter.
	int nspaces = 0;
	char *p = cmd;
	char *pos = cmd;
	while ( ( pos = strchr(p, ' ') ) != NULL ) {//while ( ( pos = _tcschr(p, ' ') ) != NULL ) {
		nspaces ++;
		p = pos + 1;
	}
	
	// Create the array to hold pointers to each bit of string
	char **args = new char*[nspaces + 1];
	
	// replace spaces with nulls and
	// create an array of TCHAR*'s which points to start of each bit.
	pos = cmd;
	int i = 0;
	args[i] = cmd;
	bool inquote=false;
	for (pos = cmd; *pos != 0; pos++) {
		// Arguments are normally separated by spaces, unless there's quoting
		if ((*pos == ' ') && !inquote) {
			*pos = '\0';
			p = pos + 1;
			args[++i] = p;
		}
		if (*pos == '"') {  
			if (!inquote) {      // Are we starting a quoted argument?
				args[i] = ++pos; // It starts just after the quote
			} else {
				*pos = '\0';     // Finish a quoted argument?
			}
			inquote = !inquote;
		}
	}
	i++;

	bool hostGiven = false, portGiven = false;
	// take in order.
	for (int j = 0; j < i; j++) {
		if ( SwitchMatch(args[j], ("help")) ||
			SwitchMatch(args[j], ("?")) ||
			SwitchMatch(args[j], ("h"))) {
			ShowUsage();
			be_app->PostMessage(B_QUIT_REQUESTED);//PostQuitMessage(1);
		} else if ( SwitchMatch(args[j], ("listen"))) {
			m_listening = true;
		} else if ( SwitchMatch(args[j], ("restricted"))) {
			m_restricted = true;
		} else if ( SwitchMatch(args[j], ("viewonly"))) {
			m_ViewOnly = true;
		} else if ( SwitchMatch(args[j], ("fullscreen"))) {
			m_FullScreen = true;
		} else if ( SwitchMatch(args[j], ("8bit"))) {
			m_Use8Bit = true;
		} else if ( SwitchMatch(args[j], ("shared"))) {
			m_Shared = true;
		} else if ( SwitchMatch(args[j], ("swapmouse"))) {
			m_SwapMouse = true;
		} else if ( SwitchMatch(args[j], ("nocursor"))) {
			m_localCursor = NOCURSOR;
		} else if ( SwitchMatch(args[j], ("dotcursor"))) {
			m_localCursor = DOTCURSOR;
		} else if ( SwitchMatch(args[j], ("normalcursor"))) {
			m_localCursor = NORMALCURSOR;
		} else if ( SwitchMatch(args[j], ("belldeiconify") )) {
			m_DeiconifyOnBell = true;
		} else if ( SwitchMatch(args[j], ("emulate3") )) {
			m_Emul3Buttons = true;
		} else if ( SwitchMatch(args[j], ("noemulate3") )) {
			m_Emul3Buttons = false;
		} else if ( SwitchMatch(args[j], ("scale") )) {
			if (++j == i) {
				ArgError(("No scaling factor specified"));
				continue;
			}
			int numscales = sscanf(args[j], ("%d/%d"), &m_scale_num, &m_scale_den);
			if (numscales < 1) { 
				ArgError(("Invalid scaling specified"));
				continue;
			}
			if (numscales == 1) 
				m_scale_den = 1; // needed if you're overriding a previous setting

		} else if ( SwitchMatch(args[j], ("emulate3timeout") )) {
			if (++j == i) {
				ArgError(("No timeout specified"));
				continue;
			}
			if (sscanf(args[j], ("%d"), &m_Emul3Timeout) != 1) {
				ArgError(("Invalid timeout specified"));
				continue;
			}
			
		} else if ( SwitchMatch(args[j], ("emulate3fuzz") )) {
			if (++j == i) {
				ArgError(("No fuzz specified"));
				continue;
			}
			if (sscanf(args[j], ("%d"), &m_Emul3Fuzz) != 1) {
				ArgError(("Invalid fuzz specified"));
				continue;
			}
			
		} else if ( SwitchMatch(args[j], ("disableclipboard") )) {
			m_DisableClipboard = true;
		}
		else if ( SwitchMatch(args[j], ("delay") )) {
			if (++j == i) {
				ArgError(("No delay specified"));
				continue;
			}
			if (sscanf(args[j], ("%d"), &m_delay) != 1) {
				ArgError(("Invalid delay specified"));
				continue;
			}
			
		} else if ( SwitchMatch(args[j], ("loglevel") )) {
			if (++j == i) {
				ArgError(("No loglevel specified"));
				continue;
			}
			if (sscanf(args[j], ("%d"), &m_logLevel) != 1) {
				ArgError(("Invalid loglevel specified"));
				continue;
			}
			
		} else if ( SwitchMatch(args[j], ("console") )) {
			m_logToConsole = true;
		} else if ( SwitchMatch(args[j], ("logfile") )) {
			if (++j == i) {
				ArgError(("No logfile specified"));
				continue;
			}
			if (sscanf(args[j], ("%s"), &m_logFilename) != 1) {
				ArgError(("Invalid logfile specified"));
				continue;
			} else {
				m_logToFile = true;
			}
		} 
		#if 0 // Todo later perhaps.
		
		else if ( SwitchMatch(args[j], ("config") )) {
			if (++j == i) {
				ArgError(("No config file specified"));
				continue;
			}
			// The GetPrivateProfile* stuff seems not to like some relative paths
			_fullpath(m_configFilename, args[j], MAXPATHLEN);
			if (_access(m_configFilename, 04)) {
				ArgError(("Can't open specified config file for reading."));
				be_app->PostMessage(B_QUIT_REQUESTED);//PostQuitMessage(1);
				continue;
			} else {
				Load(m_configFilename);
				m_configSpecified = true;
			}
		} 
		#endif
		else if ( SwitchMatch(args[j], ("register") )) {
			Register();
			be_app->PostMessage(B_QUIT_REQUESTED);//PostQuitMessage(0);
		} else {
			char phost[256];
			if (!ParseDisplay(args[j], phost, 255, &m_port)) {
				ShowUsage(("Invalid VNC server specified."));
				be_app->PostMessage(B_QUIT_REQUESTED);//PostQuitMessage(1);
			} else {
				strcpy(m_host, phost);
				m_connectionSpecified = true;
			}
		}
	}       
	
	if (m_scale_num != 1 || m_scale_den != 1) 			
		m_scaling = true;

	// reduce scaling factors by greatest common denominator
	if (m_scaling) {
		FixScaling();
	}
	// tidy up
	delete [] cmd;
	delete [] args;
}

void VNCOptions::saveInt(char *name, int value, char *fname) 
{	
	if( HasInt32( name ) )
		ReplaceInt32( name , 0 , value );
	else	
		AddInt32(name , value );

#if 0
	char buf[4];
	sprintf(buf, "%d", value); 
	WritePrivateProfileString("options", name, buf, fname);
#endif
}

int VNCOptions::readInt(char *name, int defval, char *fname)
{
	int32 val = -1;
	
	if( FindInt32(name , &val) == B_OK)
		return val;

	return defval;
	
//	return GetPrivateProfileInt("options", name, defval, fname);
}

void VNCOptions::Save(char *fname)
{	
	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
		char buf[128];
		sprintf(buf, "use_encoding_%d", i);
		saveInt(buf, m_UseEnc[i], fname);
	}
	saveInt("preferred_encoding",	m_PreferredEncoding,fname);
	saveInt("restricted",			m_restricted,		fname);
	saveInt("viewonly",				m_ViewOnly,			fname);
	saveInt("fullscreen",			m_FullScreen,		fname);
	saveInt("8bit",					m_Use8Bit,			fname);
	saveInt("shared",				m_Shared,			fname);
	saveInt("swapmouse",			m_SwapMouse,		fname);
	saveInt("belldeiconify",		m_DeiconifyOnBell,	fname);
	saveInt("emulate3",				m_Emul3Buttons,		fname);
	saveInt("emulate3timeout",		m_Emul3Timeout,		fname);
	saveInt("emulate3fuzz",			m_Emul3Fuzz,		fname);
	saveInt("disableclipboard",		m_DisableClipboard, fname);
	saveInt("localcursor",			m_localCursor,		fname);
	saveInt("scale_num",			m_scale_num,		fname);
	saveInt("scale_den",			m_scale_den,		fname);

	// Flatten and save the BMessage.
	BFile file;
	
	if (file.SetTo(fname, B_WRITE_ONLY | B_CREATE_FILE) == B_OK)
	{
		Flatten(&file);
	}

	printf("Saved preferences:---------\n");
	PrintToStream();
	printf("----------------------------\n");	

}

void VNCOptions::Load(char *fname)
{	
	BFile file;
	status_t status = file.SetTo(fname, B_READ_ONLY);
	if (status == B_OK) {
		status = Unflatten(&file);
	}
	
	printf("Loaded preferences:---------\n");
	PrintToStream();
	printf("----------------------------\n");	

	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
		char buf[128];
		sprintf(buf, "use_encoding_%d", i);
		m_UseEnc[i] =   readInt(buf, m_UseEnc[i], fname) != 0;
	}
	m_PreferredEncoding =	readInt("preferred_encoding", m_PreferredEncoding,	fname);
	m_restricted =			readInt("restricted",		m_restricted,	fname) != 0 ;
	m_ViewOnly =			readInt("viewonly",			m_ViewOnly,		fname) != 0;
	m_FullScreen =			readInt("fullscreen",		m_FullScreen,	fname) != 0;
	m_Use8Bit =				readInt("8bit",				m_Use8Bit,		fname) != 0;
	m_Shared =				readInt("shared",			m_Shared,		fname) != 0;
	m_SwapMouse =			readInt("swapmouse",		m_SwapMouse,	fname) != 0;
	m_DeiconifyOnBell =		readInt("belldeiconify",	m_DeiconifyOnBell, fname) != 0;
	m_Emul3Buttons =		readInt("emulate3",			m_Emul3Buttons, fname) != 0;
	m_Emul3Timeout =		readInt("emulate3timeout",	m_Emul3Timeout, fname);
	m_Emul3Fuzz =			readInt("emulate3fuzz",		m_Emul3Fuzz,    fname);
	m_DisableClipboard =	readInt("disableclipboard", m_DisableClipboard, fname) != 0;
	m_localCursor =			readInt("localcursor",		m_localCursor,	fname);
	m_scale_num =			readInt("scale_num",		m_scale_num,	fname);
	m_scale_den =			readInt("scale_den",		m_scale_den,	fname);


}



// Record the path to the VNC viewer and the type
// of the .vnc files in the registry
void VNCOptions::Register()
{
#if 0
	char keybuf[_MAX_PATH * 2 + 20];
	HKEY hKey, hKey2;
	if ( RegCreateKey(HKEY_CLASSES_ROOT, ".vnc", &hKey)  == ERROR_SUCCESS ) {
		RegSetValue(hKey, NULL, REG_SZ, "VncViewer.Config", 0);
		RegCloseKey(hKey);
	} else {
		log.Print(0, "Failed to register .vnc extension\n");
	}

	char filename[_MAX_PATH];
	if (GetModuleFileName(NULL, filename, _MAX_PATH) == 0) {
		log.Print(0, "Error getting vncviewer filename\n");
		return;
	}
	log.Print(2, "Viewer is %s\n", filename);

	if ( RegCreateKey(HKEY_CLASSES_ROOT, "VncViewer.Config", &hKey)  == ERROR_SUCCESS ) {
		RegSetValue(hKey, NULL, REG_SZ, "VNCviewer Config File", 0);
		
		if ( RegCreateKey(hKey, "DefaultIcon", &hKey2)  == ERROR_SUCCESS ) {
			sprintf(keybuf, "%s,0", filename);
			RegSetValue(hKey2, NULL, REG_SZ, keybuf, 0);
			RegCloseKey(hKey2);
		}
		if ( RegCreateKey(hKey, "Shell\\open\\command", &hKey2)  == ERROR_SUCCESS ) {
			sprintf(keybuf, "\"%s\" -config \"%%1\"", filename);
			RegSetValue(hKey2, NULL, REG_SZ, keybuf, 0);
			RegCloseKey(hKey2);
		}

		RegCloseKey(hKey);
	}

	if ( RegCreateKey(HKEY_LOCAL_MACHINE, 
			"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\vncviewer.exe", 
			&hKey)  == ERROR_SUCCESS ) {
		RegSetValue(hKey, NULL, REG_SZ, filename, 0);
		RegCloseKey(hKey);
	}
#endif
}



void VNCOptions::ShowUsage(char* info) {
    char msg[1024];
    char *tmpinf = ("");
    if (info != NULL) 
        tmpinf = info;
    sprintf(msg, 
    
		("%s\n\rUsage includes:\n\r"
			"  vncviewer [/8bit] [/swapmouse] [/shared] [/belldeiconify] \n\r"
			"      [/listen] [/fullscreen] [/viewonly] [/emulate3] \n\r"
			"      [/scale a/b] [/config configfile] [server:display]\n\r"
			"For full details see documentation."), 

        tmpinf);
    BAlert* pAlert = new BAlert( "VNCViewer" , msg , "OK");//MessageBox(NULL,  msg, _T("VNC error"), MB_OK | MB_ICONSTOP | MB_TOPMOST);
	pAlert->Go();
}

// The dialog box allows you to change the session-specific parameters
int VNCOptions::DoDialog(bool running)
{
	m_running = running;
 
 	BMessage archive;
 	if( RehydrateWindow("OptionsWindow" , &archive) )
 	{
 		OptionsDialog* pDialog = new OptionsDialog(&archive , this);
 		if(pDialog)
	 		pDialog->Show();
 	}	
 	//return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_OPTIONDIALOG), 
	//		NULL, (DLGPROC) OptDlgProc, (LONG) this);
}

const char* VNCOptions::GetConnectionItem( int nIndex )
{
	const char* pItem = NULL;
	if( FindString( "mru" , nIndex , &pItem) == B_OK)
		return pItem;
		
	return NULL;
}

void VNCOptions::AddConnectionItem(const char* url)
{
	// Determine if the string is already in the most recently used list.
	int i = 0;
	const char* string;
	
	// Determine the end of the list.
	int nLength = 0;

	while( FindString( "mru" , nLength , &string) == B_OK)
		nLength++;

	while( FindString("mru" , i , &string) == B_OK)
	{
		if( strcmp( url , string) == 0)
		{
			// Move it to the head of the list and return.
			for(int j = i; j < nLength - 1; j++)
			{
				const char* replaceString;
				if( FindString("mru" , j+1 , &replaceString) == B_OK)
					ReplaceString( "mru" , j , replaceString);
			}
		
			ReplaceString( "mru" , nLength -1 , url );//AddString("mru" , url );
			return;
		}
		i++;	
	}

#if 0
	for(int j = nLength + 1; j > 0 ; j--)
	{
		const char* replaceString;
		if( FindString("mru" , j-1 , &replaceString) == B_OK)
			ReplaceString( "mru" , j , replaceString);	
	}	
#endif

	AddString("mru"  , url );
}

#if 0
BOOL CALLBACK VNCOptions::OptDlgProc(  HWND hwnd,  UINT uMsg,  
									 WPARAM wParam, LPARAM lParam ) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
	VNCOptions *_this = (VNCOptions *) GetWindowLong(hwnd, GWL_USERDATA);
	
	switch (uMsg) {
		
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, GWL_USERDATA, lParam);
			_this = (VNCOptions *) lParam;
			// Initialise the controls
			for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
				HWND hPref = GetDlgItem(hwnd, IDC_RAWRADIO + (i-rfbEncodingRaw));
				SendMessage(hPref, BM_SETCHECK, 
					(i== _this->m_PreferredEncoding), 0);
				EnableWindow(hPref, _this->m_UseEnc[i]);
			}
			
			HWND hCopyRect = GetDlgItem(hwnd, ID_SESSION_SET_CRECT);
			SendMessage(hCopyRect, BM_SETCHECK, _this->m_UseEnc[rfbEncodingCopyRect], 0);
			
			HWND hSwap = GetDlgItem(hwnd, ID_SESSION_SWAPMOUSE);
			SendMessage(hSwap, BM_SETCHECK, _this->m_SwapMouse, 0);
			
			HWND hDeiconify = GetDlgItem(hwnd, IDC_BELLDEICONIFY);
			SendMessage(hDeiconify, BM_SETCHECK, _this->m_DeiconifyOnBell, 0);
			
			HWND h8bit = GetDlgItem(hwnd, IDC_8BITCHECK);
			SendMessage(h8bit, BM_SETCHECK, _this->m_Use8Bit, 0);
			
			HWND hShared = GetDlgItem(hwnd, IDC_SHARED);
			SendMessage(hShared, BM_SETCHECK, _this->m_Shared, 0);
			EnableWindow(hShared, !_this->m_running);
			 
			HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY);
			SendMessage(hViewOnly, BM_SETCHECK, _this->m_ViewOnly, 0);


			HWND hScaling = GetDlgItem(hwnd, IDC_SCALING);
			SendMessage(hScaling, BM_SETCHECK, _this->m_scaling, 0);

			SetDlgItemInt( hwnd, IDC_SCALE_NUM, _this->m_scale_num, FALSE);
			SetDlgItemInt( hwnd, IDC_SCALE_DEN, _this->m_scale_den, FALSE);

			CentreWindow(hwnd);
			
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
					HWND hPref = GetDlgItem(hwnd, IDC_RAWRADIO+i-rfbEncodingRaw);
					if (SendMessage(hPref, BM_GETCHECK, 0, 0) == BST_CHECKED)
						_this->m_PreferredEncoding = i;
				}
				
				HWND hCopyRect = GetDlgItem(hwnd, ID_SESSION_SET_CRECT);
				_this->m_UseEnc[rfbEncodingCopyRect] =
					(SendMessage(hCopyRect, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hSwap = GetDlgItem(hwnd, ID_SESSION_SWAPMOUSE);
				_this->m_SwapMouse =
					(SendMessage(hSwap, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hDeiconify = GetDlgItem(hwnd, IDC_BELLDEICONIFY);
				_this->m_DeiconifyOnBell =
					(SendMessage(hDeiconify, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hDisableClip = GetDlgItem(hwnd, IDC_DISABLECLIPBOARD);
				_this->m_DisableClipboard =
					(SendMessage(hDisableClip, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND h8bit = GetDlgItem(hwnd, IDC_8BITCHECK);
				_this->m_Use8Bit =
					(SendMessage(h8bit, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hShared = GetDlgItem(hwnd, IDC_SHARED);
				_this->m_Shared =
					(SendMessage(hShared, BM_GETCHECK, 0, 0) == BST_CHECKED);
				
				HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY);
				_this->m_ViewOnly = 
					(SendMessage(hViewOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hScaling = GetDlgItem(hwnd, IDC_SCALING);
				_this->m_scaling = 
					(SendMessage(hScaling, BM_GETCHECK, 0, 0) == BST_CHECKED);

				if (_this->m_scaling) {
					_this->m_scale_num = GetDlgItemInt( hwnd, IDC_SCALE_NUM, NULL, TRUE);
					_this->m_scale_den = GetDlgItemInt( hwnd, IDC_SCALE_DEN, NULL, TRUE);
					_this->FixScaling();
					if (_this->m_scale_num == 1 && _this->m_scale_den == 1)
						_this->m_scaling = false;
				} else {
					_this->m_scale_num = 1;
					_this->m_scale_den = 1;
				}

				EndDialog(hwnd, TRUE);
				
				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
		break;
        case WM_DESTROY:
			EndDialog(hwnd, FALSE);
			return TRUE;
	}
	return 0;
}

#endif

