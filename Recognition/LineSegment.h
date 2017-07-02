#pragma once
#include "stdafx.h"
typedef vector<CvPoint2D32f> Path2D;

// Line Segment는 패턴을 단순화 시키는 작업
// 패턴이 휘는 부분을 찾아 이를 이용하여 단순화 시킴
class CLineSegment
{
public:
	CLineSegment(void);
	~CLineSegment(void);

	void Input( Path2D pArray );
	void Process( BOOL bIgnoreFirstLine = FALSE );
	void DrawLineSegment( IplImage *pImage );

	Path2D m_arrResult;

private:
	Path2D m_arrInput;
	IplImage *m_pResult;
	CvPoint2D32f m_prevPoint;

	int m_nTemp;
};

