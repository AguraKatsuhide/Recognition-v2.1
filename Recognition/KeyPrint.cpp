#include "KeyPrint.h"
#include <TlHelp32.h>
#include <Psapi.h>

#pragma data_seg("SHAREDATA")
static int g_bGetMode = 0;
static DWORD g_nKeyMode = -1;
static DWORD g_nKeySentence = -1;
#pragma data_seg()
#pragma comment( linker, "/SECTION:.SHAREDATA,RWS")

typedef int* (CALLBACK* GetMode)();
typedef DWORD* (CALLBACK* GetKeyMode)();

CKeyPrint::CKeyPrint(void)
{
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	strcat(path,"\\KeyMode.dll");
	m_hDll = LoadLibrary(path);
	m_pHelpKorean = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
	m_pCurrentHelp = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
	m_pHelpEngLarge = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
	m_pHelpEngSmall = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
	m_pHelpCharacter = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
	m_pHelpNumber = cvCreateImage( cvSize(504, 872), IPL_DEPTH_8U, 3 );
}


CKeyPrint::~CKeyPrint(void)
{
	FreeLibrary( m_hDll );
	if( m_pHelpKorean )			cvReleaseImage( &m_pHelpKorean );
	if( m_pCurrentHelp )		cvReleaseImage( &m_pCurrentHelp );
	if( m_pHelpEngLarge )		cvReleaseImage( &m_pHelpEngLarge );
	if( m_pHelpEngSmall )		cvReleaseImage( &m_pHelpEngSmall );
	if( m_pHelpNumber )			cvReleaseImage( &m_pHelpNumber );
	if( m_pHelpCharacter )		cvReleaseImage( &m_pHelpCharacter );
}

void CKeyPrint::SetKoreanHelp()
{
	memset( m_pHelpKorean->imageData, 255, m_pHelpKorean->imageSize );
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	for( int i = 1; i < 20; i++ )	// 자음
	{
		sprintf( strPath, "%s\\Pattern\\Korean\\images\\K%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 106;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpKorean->imageData + (nsy+y) * m_pHelpKorean->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
//		for( int y = 76; y < 105; y++ )
//			memset( m_pHelpKorean->imageData + (nsy+y) * m_pHelpKorean->widthStep + nsx * 3, 0, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	for( int i = 20; i < 34; i++ )	// 모음
	{
		sprintf( strPath, "%s\\Pattern\\Korean\\images\\K%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = i % 5 * 101;
		int nsy = i / 5 * 106;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpKorean->imageData + (nsy+y) * m_pHelpKorean->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
//		for( int y = 76; y < 105; y++ )
//			memset( m_pHelpKorean->imageData + (nsy+y) * m_pHelpKorean->widthStep + nsx * 3, 0, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	cvRectangle( m_pHelpKorean, cvPoint(4*101, 106*3), cvPoint(4*101+99, 106*3+74), cvScalar(0,0,0), -1 );
	cvRectangle( m_pHelpKorean, cvPoint(4*101, 106*6), cvPoint(4*101+99, 106*6+74), cvScalar(0,0,0), -1 );
	
	SetOperationHelp( m_pHelpKorean );
	memcpy( m_pCurrentHelp->imageData, m_pHelpKorean->imageData, m_pCurrentHelp->imageSize );
	cvShowImage( "Help", m_pCurrentHelp );
}

void CKeyPrint::SetOperationHelp( IplImage *pHelpImage )
{
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	cvPutText( pHelpImage, "Control", cvPoint(10, 7*106+20), &cvFont( 1.3, 1 ), cvScalar( 0, 0, 0 ) );
	for( int i = 1; i < 4; i++ )	// 제어키 로드
	{
		sprintf( strPath, "%s\\Pattern\\Operation\\images\\O%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = 7 * 106 + 25;
		for( int y = 0; y < 75; y++ )
			memcpy( pHelpImage->imageData + (nsy+y) * pHelpImage->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
//		for( int y = 76; y < 105; y++ )
//			memset( m_pHelpKorean->imageData + (nsy+y) * m_pHelpKorean->widthStep + nsx * 3, 0, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	cvRectangle( pHelpImage, cvPoint(3*101, 106*7+25), cvPoint(3*101+99, 106*7+74+25), cvScalar(0,0,0), -1 );
	cvRectangle( pHelpImage, cvPoint(4*101, 106*7+25), cvPoint(4*101+99, 106*7+74+25), cvScalar(0,0,0), -1 );
}

void CKeyPrint::SetEngLargeHelp()
{
	memset( m_pHelpEngLarge->imageData, 255, m_pHelpEngLarge->imageSize );
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	for( int i = 1; i < 27; i++ )	// 자음
	{
		sprintf( strPath, "%s\\Pattern\\Large\\images\\L%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpEngLarge->imageData + (nsy+y) * m_pHelpEngLarge->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	for( int i = 27; i < 36; i++ )
	{
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		cvRectangle( m_pHelpEngLarge, cvPoint(nsx, nsy), cvPoint(nsx+99, nsy+74), cvScalar(0,0,0), -1 );
	}
	SetOperationHelp( m_pHelpEngLarge );
	memcpy( m_pCurrentHelp->imageData, m_pHelpEngLarge->imageData, m_pCurrentHelp->imageSize );
	cvShowImage( "Help", m_pCurrentHelp );
}

void CKeyPrint::SetEngSmallHelp()
{
	memset( m_pHelpEngSmall->imageData, 255, m_pHelpEngSmall->imageSize );
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	for( int i = 1; i < 27; i++ )	// 자음
	{
		sprintf( strPath, "%s\\Pattern\\Small\\images\\S%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpEngSmall->imageData + (nsy+y) * m_pHelpEngSmall->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	for( int i = 27; i < 36; i++ )
	{
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		cvRectangle( m_pHelpEngSmall, cvPoint(nsx, nsy), cvPoint(nsx+99, nsy+74), cvScalar(0,0,0), -1 );
	}
	SetOperationHelp( m_pHelpEngSmall );
	memcpy( m_pCurrentHelp->imageData, m_pHelpEngSmall->imageData, m_pCurrentHelp->imageSize );
	cvShowImage( "Help", m_pCurrentHelp );
}

void CKeyPrint::SetCharacterHelp()
{
	memset( m_pHelpCharacter->imageData, 255, m_pHelpCharacter->imageSize );
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	for( int i = 1; i < 33; i++ )	// 자음
	{
		sprintf( strPath, "%s\\Pattern\\Character\\images\\C%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpCharacter->imageData + (nsy+y) * m_pHelpCharacter->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	for( int i = 33; i < 36; i++ )
	{
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		cvRectangle( m_pHelpCharacter, cvPoint(nsx, nsy), cvPoint(nsx+99, nsy+74), cvScalar(0,0,0), -1 );
	}
	SetOperationHelp( m_pHelpCharacter );
	memcpy( m_pCurrentHelp->imageData, m_pHelpCharacter->imageData, m_pCurrentHelp->imageSize );
	cvShowImage( "Help", m_pCurrentHelp );
}

void CKeyPrint::SetNumberHelp()
{
	memset( m_pHelpNumber->imageData, 255, m_pHelpNumber->imageSize );
	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	char strPath[1024] = {0};
	for( int i = 1; i < 11; i++ )	// 자음
	{
		sprintf( strPath, "%s\\Pattern\\Number\\images\\N%02d.jpg", path, i );
		IplImage *pImage = cvLoadImage( strPath );
		IplImage *pResize = cvCreateImage( cvSize(100, 75), IPL_DEPTH_8U, 3 );
		cvResize( pImage, pResize );
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		for( int y = 0; y < 75; y++ )
			memcpy( m_pHelpNumber->imageData + (nsy+y) * m_pHelpNumber->widthStep + nsx * 3, pResize->imageData + y * pResize->widthStep, pResize->widthStep );
		cvReleaseImage( &pImage );
	}
	for( int i = 11; i < 36; i++ )
	{
		int nsx = (i - 1) % 5 * 101;
		int nsy = (i - 1) / 5 * 76;
		cvRectangle( m_pHelpNumber, cvPoint(nsx, nsy), cvPoint(nsx+99, nsy+74), cvScalar(0,0,0), -1 );
	}
	SetOperationHelp( m_pHelpNumber );
	memcpy( m_pCurrentHelp->imageData, m_pHelpNumber->imageData, m_pCurrentHelp->imageSize );
	cvShowImage( "Help", m_pCurrentHelp );
}

BOOL CKeyPrint::Inject( DWORD dwPID, LPCTSTR szDllName )
{
	GetMode GetModePtr = NULL;
	GetKeyMode GetKeyModePtr = NULL;
	
	HANDLE hProcess, hThread, hThread2;
	HMODULE hMod;
	LPVOID pRemoteBuf;
	DWORD dwBufSize = lstrlen(szDllName) + 1;
	LPTHREAD_START_ROUTINE pThreadProc;
	LPTHREAD_START_ROUTINE pThreadProc2;

	if( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )
		return FALSE;

	pRemoteBuf = VirtualAllocEx( hProcess, NULL, dwBufSize, MEM_COMMIT, PAGE_READWRITE );
	BOOL bWrite = WriteProcessMemory( hProcess, pRemoteBuf, (LPVOID)szDllName, dwBufSize, NULL );
	hMod = GetModuleHandle( "kernel32.dll" );
	pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress( hMod, "LoadLibraryA" );
	hThread = CreateRemoteThread( hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL );
	WaitForSingleObject( hThread, INFINITE );
	Sleep(10);

	GetModePtr = (GetMode)GetProcAddress( m_hDll, "GetMode" );
	GetKeyModePtr = (GetKeyMode)GetProcAddress( m_hDll, "GetKeyMode" );

	m_nMode = *GetModePtr();
	m_nKey = *GetKeyModePtr();

	CloseHandle( hThread );
	CloseHandle( hProcess );
	
	return TRUE;
}

BOOL CKeyPrint::Eject( DWORD dwPID, LPCTSTR szDllName )
{
	BOOL bMore = FALSE, bFound = FALSE;
	HANDLE hSnapshot, hProcess, hThread;
	HMODULE hModule = NULL;
	MODULEENTRY32 me = { sizeof(me) };
	LPTHREAD_START_ROUTINE pThreadProc;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);

	bMore = Module32First(hSnapshot, &me);
	while( bMore )
    {
		if( !_stricmp((LPCTSTR)me.szModule, szDllName) )
		{
			bFound = TRUE;
			break;
		}
		bMore = Module32Next(hSnapshot, &me);
	}

	if( !bFound )
	{
		CloseHandle(hSnapshot);
		return FALSE;
	}

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
	hModule = GetModuleHandle("kernel32.dll");
	pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "FreeLibrary");
	hThread = CreateRemoteThread(hProcess, NULL, 0, 
								 pThreadProc, me.modBaseAddr, 
								 0, NULL);
	WaitForSingleObject(hThread, INFINITE); 

	CloseHandle(hThread);
	CloseHandle(hProcess);
	CloseHandle(hSnapshot);

	return TRUE;
}

BOOL CKeyPrint::CheckInjection( DWORD dwPID, LPCTSTR szDllName )
{
	m_nMode = 0;
	HMODULE hMods[1024];
	DWORD cbNeeded;
	TCHAR szModName[MAX_PATH];
	HANDLE hProcess;
	
	if( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )
		return FALSE;

	if( EnumProcessModules( hProcess, hMods, sizeof(hMods), &cbNeeded ) )
	{
		for( int k = 0; k < (cbNeeded / sizeof(HMODULE)); k++ )
		{
			if( GetModuleFileNameEx( hProcess, hMods[k], szModName, sizeof(szModName)/sizeof(TCHAR)))
			{
				if( strstr( szModName, szDllName ) )
				{
					CloseHandle( hProcess );
					return TRUE;
				}
			}
		}
	}
	CloseHandle( hProcess );
	return FALSE;
}

void CKeyPrint::InputKey( string str )
{
	HWND hWnd = GetForegroundWindow();
	DWORD dwPID = 0;
	GetWindowThreadProcessId( hWnd, &dwPID );

	TCHAR path[512];
	GetCurrentDirectory(512,path);  //프로젝트 경로
	strcat(path,"\\KeyMode.dll");

	SoundOut( str );

	m_cXMLLang = str.data()[0];
	sscanf( &str.data()[1], "%d", &m_nXMLCode );

	PrintHelp();

	if( CheckInjection( dwPID, path ) )
	{
		if( GetCurrentProcessId() == dwPID )
			return;
		Eject( dwPID, "KeyMode.dll" );
		CheckInjection( dwPID, path );
	}

	m_nInject = Inject( dwPID, path );
	if( !m_nInject )
		return;
	
	if( m_nKey == 0 )
	{
		m_nKey = 1;
		keybd_event( VK_HANGUL, MapVirtualKey(VK_HANGUL, 0), 0, 0 );
		keybd_event( VK_HANGUL, MapVirtualKey(VK_HANGUL, 0), KEYEVENTF_KEYUP, 0 );
	}

	PrintKey();
	
	Eject( dwPID, "KeyMode.dll" );
}

void CKeyPrint::PrintHelp()
{
	switch( m_cXMLLang )
	{
	case 'k' :
	case 'K' :
		PrintKoreanHelp();
		break;
	case 'o' :
	case 'O' :
		PrintControlHelp();
		break;
	case 'l' :
	case 'L' :
		break;
	case 's' :
	case 'S' :
		break;
	case 'n' :
	case 'N' :
		break;
	case 'c':
	case 'C':
		break;
	}
}

void CKeyPrint::SoundOut( string str )
{
	TCHAR path[512];
	GetCurrentDirectory( 512, path );

	int nSize = str.size();
	str[nSize-3] = 'w';
	str[nSize-2] = 'a';
	str[nSize-1] = 'v';

	strcat( path, "\\Sound\\" );
	strcat( path, str.data() );
	PlaySound( NULL, NULL, NULL );
	PlaySound( path, NULL, SND_ASYNC );
}

void CKeyPrint::PrintKoreanHelp()
{
	IplImage *pImage = cvCreateImage( cvGetSize(m_pHelpKorean), IPL_DEPTH_8U, 3 );
	memcpy( pImage->imageData, m_pHelpKorean->imageData, pImage->imageSize );
	int x, y;
	if( m_nXMLCode < 20 )		// 자음
	{
		x = (m_nXMLCode - 1) % 5;
		y = (m_nXMLCode - 1) / 5;
	}
	else
	{
		x = m_nXMLCode % 5;
		y = m_nXMLCode / 5;
	}
	x = x * 101;
	y = y * 76;
	cvRectangle( pImage, cvPoint(x,y), cvPoint(x+99,y+74), cvScalar(0,0,255), 3 );
	cvShowImage( "Help", pImage );
	cvReleaseImage( &pImage );
}

void CKeyPrint::PrintControlHelp()
{
	IplImage *pImage = cvCreateImage( cvGetSize(m_pCurrentHelp), IPL_DEPTH_8U, 3 );
	memcpy( pImage->imageData, m_pCurrentHelp->imageData, pImage->imageSize );
	int x = (m_nXMLCode - 1) * 101;
	int y = 7 * 76 + 25;
	cvRectangle( pImage, cvPoint(x,y), cvPoint(x+99,y+74), cvScalar(0,0,255), 3 );
	cvShowImage( "Help", pImage );
	cvReleaseImage( &pImage );
}

void CKeyPrint::PrintKey()
{
	switch( m_cXMLLang )
	{
	case 'k' :
	case 'K' :
		PrintKoreanKey();
		break;
	case 'o' :
	case 'O' :
		PrintControlKey();
		break;
	case 'l' :
	case 'L' :
		PrintEnglishLargeKey();
		break;
	case 's' :
	case 'S' :
		PrintEnglishSmallKey();
		break;
	case 'n' :
	case 'N' :
		PrintNumberKey();
		break;
	case 'c':
	case 'C':
		PrintCharacterKey();
		break;
	}
}

void CKeyPrint::PrintNumberKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( '1', 0 );		break;
	case 2:		PushKey( '2', 0 );		break;
	case 3:		PushKey( '3', 0 );		break;
	case 4:		PushKey( '4', 0 );		break;
	case 5:		PushKey( '5', 0 );		break;
	case 6:		PushKey( '6', 0 );		break;
	case 7:		PushKey( '7', 0 );		break;
	case 8:		PushKey( '8', 0 );		break;
	case 9:		PushKey( '9', 0 );		break;
	case 10:	PushKey( '0', 0 );		break;
	}
}

void CKeyPrint::PrintCharacterKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( '`', 1 );		break;
	case 2:		PushKey( '`', 1 );		break;
	case 3:		PushKey( '1', 1 );		break;
	case 4:		PushKey( '2', 1 );		break;
	case 5:		PushKey( '3', 1 );		break;
	case 6:		PushKey( '4', 1 );		break;
	case 7:		PushKey( '5', 1 );		break;
	case 8:		PushKey( '6', 1 );		break;
	case 9:		PushKey( '7', 1 );		break;
	case 10:	PushKey( '8', 1 );		break;
	case 11:	PushKey( '9', 1 );		break;
	case 12:	PushKey( '0', 1 );		break;
	case 13:	PushKey( '-', 0 );		break;
	case 14:	PushKey( '=', 1 );		break;
	case 15:	PushKey( '-', 1 );		break;
	case 16:	PushKey( '=', 0 );		break;
	case 17:	PushKey( '\\', 0 );		break;
	case 18:	PushKey( '\\', 1 );		break;
	case 19:	PushKey( '[', 1 );		break;
	case 20:	PushKey( '[', 0 );		break;
	case 21:	PushKey( ']', 1 );		break;
	case 22:	PushKey( ']', 0 );		break;
	case 23:	PushKey( ';', 0 );		break;
	case 24:	PushKey( ';', 1 );		break;
	case 25:	PushKey( '\'', 0 );		break;
	case 26:	PushKey( '\'', 1 );		break;
	case 27:	PushKey( ',', 1 );		break;
	case 28:	PushKey( ',', 0 );		break;
	case 29:	PushKey( '.', 1 );		break;
	case 30:	PushKey( '.', 0 );		break;
	case 31:	PushKey( '/', 1 );		break;
	case 32:	PushKey( '/', 0 );		break;
	}
}

void CKeyPrint::PrintEnglishSmallKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( 'A', 0 );		break;
	case 2:		PushKey( 'B', 0 );		break;
	case 3:		PushKey( 'C', 0 );		break;
	case 4:		PushKey( 'D', 0 );		break;
	case 5:		PushKey( 'E', 0 );		break;
	case 6:		PushKey( 'F', 0 );		break;
	case 7:		PushKey( 'G', 0 );		break;
	case 8:		PushKey( 'H', 0 );		break;
	case 9:		PushKey( 'I', 0 );		break;
	case 10:	PushKey( 'J', 0 );		break;
	case 11:	PushKey( 'K', 0 );		break;
	case 12:	PushKey( 'L', 0 );		break;
	case 13:	PushKey( 'M', 0 );		break;
	case 14:	PushKey( 'N', 0 );		break;
	case 15:	PushKey( 'O', 0 );		break;
	case 16:	PushKey( 'P', 0 );		break;
	case 17:	PushKey( 'Q', 0 );		break;
	case 18:	PushKey( 'R', 0 );		break;
	case 19:	PushKey( 'S', 0 );		break;
	case 20:	PushKey( 'T', 0 );		break;
	case 21:	PushKey( 'U', 0 );		break;
	case 22:	PushKey( 'V', 0 );		break;
	case 23:	PushKey( 'W', 0 );		break;
	case 24:	PushKey( 'X', 0 );		break;
	case 25:	PushKey( 'Y', 0 );		break;
	case 26:	PushKey( 'Z', 0 );		break;
	}
}

void CKeyPrint::PrintEnglishLargeKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( 'A', 1 );		break;
	case 2:		PushKey( 'B', 1 );		break;
	case 3:		PushKey( 'C', 1 );		break;
	case 4:		PushKey( 'D', 1 );		break;
	case 5:		PushKey( 'E', 1 );		break;
	case 6:		PushKey( 'F', 1 );		break;
	case 7:		PushKey( 'G', 1 );		break;
	case 8:		PushKey( 'H', 1 );		break;
	case 9:		PushKey( 'I', 1 );		break;
	case 10:	PushKey( 'J', 1 );		break;
	case 11:	PushKey( 'K', 1 );		break;
	case 12:	PushKey( 'L', 1 );		break;
	case 13:	PushKey( 'M', 1 );		break;
	case 14:	PushKey( 'N', 1 );		break;
	case 15:	PushKey( 'O', 1 );		break;
	case 16:	PushKey( 'P', 1 );		break;
	case 17:	PushKey( 'Q', 1 );		break;
	case 18:	PushKey( 'R', 1 );		break;
	case 19:	PushKey( 'S', 1 );		break;
	case 20:	PushKey( 'T', 1 );		break;
	case 21:	PushKey( 'U', 1 );		break;
	case 22:	PushKey( 'V', 1 );		break;
	case 23:	PushKey( 'W', 1 );		break;
	case 24:	PushKey( 'X', 1 );		break;
	case 25:	PushKey( 'Y', 1 );		break;
	case 26:	PushKey( 'Z', 1 );		break;
	}
}

void CKeyPrint::PushKey( char cKey, int nShift )
{
	if( nShift )
		keybd_event( VK_SHIFT, MapVirtualKey(VK_SHIFT, 0), 0, 0 );
	keybd_event( cKey, 0, 0, 0 );
	keybd_event( cKey, 0, KEYEVENTF_KEYUP, 0 );
	if( nShift )
		keybd_event( VK_SHIFT, MapVirtualKey(VK_SHIFT, 0), KEYEVENTF_KEYUP, 0 );
}

void CKeyPrint::PrintControlKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( VK_SPACE, 0 );	break;
	case 2:		PushKey( VK_BACK, 0 );	break;
	case 3:		PushKey( VK_RETURN, 0 );break;
	}
}

void CKeyPrint::PrintKoreanKey()
{
	switch( m_nXMLCode )
	{
	case 1:		PushKey( 'R', 0 );		break;
	case 2:		PushKey( 'R', 1 );		break;
	case 3:		PushKey( 'S', 0 );		break;
	case 4:		PushKey( 'E', 0 );		break;
	case 5:		PushKey( 'E', 1 );		break;
	case 6:		PushKey( 'F', 0 );		break;
	case 7:		PushKey( 'A', 0 );		break;
	case 8:		PushKey( 'Q', 0 );		break;
	case 9:		PushKey( 'Q', 1 );		break;
	case 10:	PushKey( 'T', 0 );		break;
	case 11:	PushKey( 'T', 1 );		break;
	case 12:	PushKey( 'D', 0 );		break;
	case 13:	PushKey( 'W', 0 );		break;
	case 14:	PushKey( 'W', 1 );		break;
	case 15:	PushKey( 'C', 0 );		break;
	case 16:	PushKey( 'Z', 0 );		break;
	case 17:	PushKey( 'X', 0 );		break;
	case 18:	PushKey( 'V', 0 );		break;
	case 19:	PushKey( 'G', 0 );		break;
	case 20:	PushKey( 'K', 0 );		break;
	case 21:	PushKey( 'O', 0 );		break;
	case 22:	PushKey( 'I', 0 );		break;
	case 23:	PushKey( 'O', 1 );		break;
	case 24:	PushKey( 'J', 0 );		break;
	case 25:	PushKey( 'P', 0 );		break;
	case 26:	PushKey( 'U', 0 );		break;
	case 27:	PushKey( 'P', 1 );		break;
	case 28:	PushKey( 'H', 0 );		break;
	case 29:	PushKey( 'Y', 0 );		break;
	case 30:	PushKey( 'N', 0 );		break;
	case 31:	PushKey( 'B', 0 );		break;
	case 32:	PushKey( 'M', 0 );		break;
	case 33:	PushKey( 'L', 0 );		break;
	}
}