#pragma once
#include "stdafx.h"
#include <MMSystem.h>

// 사용하지 않음.
// 원래는 Help창 생성 및 Notepad에 키 입력, 사운드 출력하는 클래스

class CKeyPrint
{
public:
	CKeyPrint(void);
	~CKeyPrint(void);

	void InputKey( string str );
	void SetKoreanHelp();
	void SetEngLargeHelp();
	void SetEngSmallHelp();
	void SetNumberHelp();
	void SetCharacterHelp();

private:
	BOOL CheckInjection( DWORD dwPID, LPCTSTR szDllName );
	BOOL Inject( DWORD dwPID, LPCTSTR szDllName );
	BOOL Eject( DWORD dwPID, LPCTSTR szDllName );

	void SetOperationHelp( IplImage *pHelpImage );

	void PrintHelp();
	void PrintKoreanHelp();
	void PrintControlHelp();
	
	void PrintKey();
	void PrintKoreanKey();
	void PrintEnglishSmallKey();
	void PrintEnglishLargeKey();
	void PrintNumberKey();
	void PrintCharacterKey();
	void PrintControlKey();
	void PushKey( char cKey, int nShift );

	void SoundOut( string str );

	IplImage *m_pHelpKorean;
	IplImage *m_pHelpEngLarge;
	IplImage *m_pHelpEngSmall;
	IplImage *m_pHelpNumber;
	IplImage *m_pHelpCharacter;
	IplImage *m_pCurrentHelp;

	HMODULE m_hDll;
	char m_cXMLLang;
	int m_nXMLCode;
	int m_nMode;
	int m_nKey;
	int m_nInject;
};

