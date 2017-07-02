#include "LineSegment.h"


CLineSegment::CLineSegment(void)
{
	m_pResult = cvCreateImage( cvSize(640, 480), IPL_DEPTH_8U, 3 );
	memset( m_pResult->imageData, 0, m_pResult->imageSize );
	m_arrInput.clear();
	m_nTemp = 0;
}


CLineSegment::~CLineSegment(void)
{
	m_arrInput.clear();
	cvReleaseImage( &m_pResult );
}

void CLineSegment::Input( Path2D pArray )
{
	m_arrInput.clear();
	m_arrResult.clear();
	m_arrInput = pArray;
}

void CLineSegment::Process( BOOL bIgnoreFirstLine )
{
	Path2D vecDifference1, vecDifference2;
	vector<double> vecAngle;

	if( m_arrInput.size() == 0 )
		return;

	double dX1 = 0, dX2 = 1, dX3 = -1;
	double dY1 = 0, dY2 = 1, dY3 = -1;

	m_arrResult.push_back( m_arrInput[0] );
	for( int i = 0; i < (int)(m_arrInput.size() - 1); i++ )
	{
		// 연결되는 점의 vector와 각도 측정
		CvPoint2D32f pt;
		pt.x = m_arrInput[i].x - m_arrInput[i+1].x;
		pt.y = m_arrInput[i].y - m_arrInput[i+1].y;
		vecDifference1.push_back( pt );
		double dAngle = atan( pt.y / pt.x );
		if( pt.x < 0 && pt.y < 0 )										;
		if( pt.x > 0 && pt.y < 0 )				dAngle += 3.1415926535f;
		if( pt.x > 0 && pt.y > 0 )				dAngle += 3.1415926535f;
		if( pt.x < 0 && pt.y > 0 )				dAngle += 3.1415926535f * 2;
		if( pt.x == 0 )
		{
			if( pt.y > 0 )
			{
				if( dAngle < 0 )				dAngle += 3.1415926535f;
				dAngle += 3.1415926535f;
			}
			else if (pt.y < 0 )
				if( dAngle < 0 )				dAngle += 3.1415926535f;
		}
		if( pt.x > 0 && pt.y == 0 )				dAngle += 3.1415926535f;


		vecAngle.push_back( dAngle );
	}

	for( int i = 0; i < (int)(vecAngle.size() - 1); i++ )
	{
		// vector와 각도가 일정이상 차이나면 꺽여있다고 추정, 결과에 추가함.
		double dDiff = fabs(vecAngle[i] - vecAngle[i+1]);
		double dThresDegree = 3.1415926535f / 4.0f;
		if( dDiff < dThresDegree || 
			fabs(3.1415926535f * 2 - dDiff) < dThresDegree || 
			fabs(3.1415926535f * 2 + dDiff) < dThresDegree )
			vecAngle[i+1] = vecAngle[i];
		else
			m_arrResult.push_back( m_arrInput[i+1] );
	}
	if( m_arrInput.size() != 0 )
		m_arrResult.push_back( m_arrInput[m_arrInput.size()-1] );

	if( bIgnoreFirstLine )
	{
		m_prevPoint.x = 0;
		m_prevPoint.y = 0;
		if( m_arrResult.size() == 2 )
			m_arrResult.clear();
		else
		{
			m_prevPoint = m_arrResult[0];
			m_arrResult.erase( m_arrResult.begin(), m_arrResult.begin() + 1 );
		}
	}

	// 예외처리
	double dAvgDist = 0;
	for( int i = 0; i < (int)(m_arrResult.size()-1); i++ )
	{
		double dDist = sqrt( pow(m_arrResult[i].x - m_arrResult[i+1].x, 2) + pow(m_arrResult[i].y - m_arrResult[i+1].y, 2) );
		dAvgDist += dDist;
	}

	dAvgDist /= m_arrResult.size()-1;

	for( int i = 0; i < (int)(m_arrResult.size()-1); i++ )
	{
		double dDist = sqrt( pow(m_arrResult[i].x - m_arrResult[i+1].x, 2) + pow(m_arrResult[i].y - m_arrResult[i+1].y, 2) );
		if( dDist < dAvgDist / 5 )
		{
			m_arrResult.erase( m_arrResult.begin() + i, m_arrResult.begin() + i + 1 );
			i--;
		}
	}


	vecAngle.clear();
	vecDifference2.clear();
	vecDifference1.clear();
}

void CLineSegment::DrawLineSegment( IplImage *pImage )
{
	if( m_prevPoint.x && m_prevPoint.y )
	{
		cvCircle( pImage, cvPoint(m_prevPoint.x*2, m_prevPoint.y*2), 7, cvScalar(50, 30, 10), -1 );
		cvLine( pImage, cvPoint(m_prevPoint.x*2, m_prevPoint.y*2), cvPoint(m_arrResult[0].x*2, m_arrResult[0].y*2), cvScalar(50, 30, 10), 3 );
	}
	for( int i = 0; i < (int)(m_arrResult.size()-1); i++ )
	{
		cvCircle( pImage, cvPoint(m_arrResult[i].x*2, m_arrResult[i].y*2), 7, cvScalar(10, 50, 30), -1 );
		cvLine( pImage, cvPoint(m_arrResult[i].x*2, m_arrResult[i].y*2), cvPoint(m_arrResult[i+1].x*2, m_arrResult[i+1].y*2), cvScalar(30, 50, 10), 3 );
		cvCircle( pImage, cvPoint(m_arrResult[i+1].x*2, m_arrResult[i+1].y*2), 7, cvScalar(10, 50, 30), -1 );
	}
}
