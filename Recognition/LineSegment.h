#pragma once
#include "stdafx.h"
typedef vector<CvPoint2D32f> Path2D;

// Line Segment�� ������ �ܼ�ȭ ��Ű�� �۾�
// ������ �ִ� �κ��� ã�� �̸� �̿��Ͽ� �ܼ�ȭ ��Ŵ
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

