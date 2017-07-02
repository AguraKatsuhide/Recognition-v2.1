#pragma once
#include "1$Recognizer.h"

typedef struct _RECT32f
{
	float l;
	float r;
	float t;
	float b;
} RECT32f;

class CPatternMatch
{
public:
	CPatternMatch(void);
	~CPatternMatch(void);
public:
	void AddPattern( string path );
	CResult Matching( CvPointArray pArray );
	C1$Recognizer m_PatternRecognition;
	void DrawStatus( IplImage *pImage );

	double m_dMinScore;
	int m_nSetLineMode;

private:
	void LoadPattern( string path );
	RECT32f ExtractROI( Path2D path );
	void ExtractStartEndAngle( RECT32f roi, Path2D path );

	double CalcAngle( double dx, double dy );

	int m_nStartAngle;
	int m_nEndAngle;
	int m_nRate;
	double m_dRadStart;
	double m_dRadEnd;
	string m_strPatternName;
	Path2D m_pPatternArray;
	CResult m_result;
	TEMPLATE m_input;

	IplImage * m_pStatus;
};

