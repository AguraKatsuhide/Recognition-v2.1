#include "PatternMatch.h"


CPatternMatch::CPatternMatch(void)
{
	m_pStatus = cvCreateImage( cvSize(640, 160), IPL_DEPTH_8U, 3 );//480
	m_nSetLineMode = 0;
}


CPatternMatch::~CPatternMatch(void)
{
	cvReleaseImage( &m_pStatus );
}

void CPatternMatch::AddPattern( string path )
{
	// 미리 저장된 패턴 삽입
	m_nStartAngle = 0;
	m_nEndAngle = 0;
	m_dRadStart = 0;
	m_dRadEnd = 0;
	m_pPatternArray.clear();
	m_strPatternName.clear();

	LoadPattern( path );
	RECT32f rt = ExtractROI( m_pPatternArray );
	ExtractStartEndAngle( rt, m_pPatternArray );
	m_pPatternArray = m_PatternRecognition.NormalizedPath( m_pPatternArray );

	m_PatternRecognition.AddTemplete( m_strPatternName, m_strPatternName, m_pPatternArray, m_nStartAngle, m_nEndAngle, m_nRate, m_dRadStart, m_dRadEnd );
}

double CPatternMatch::CalcAngle( double dx, double dy )
{
	// (0,0) 기준 (dx,dy)의 각도 계산
	double dAngle = atan( dy / dx );

	if( dx < 0 && dy < 0 )										;
	if( dx > 0 && dy < 0 )				dAngle += 3.1415926535f;
	if( dx > 0 && dy > 0 )				dAngle += 3.1415926535f;
	if( dx < 0 && dy > 0 )				dAngle += 3.1415926535f * 2;
	if( dx == 0 )
	{
		if( dy > 0 )
		{
			if( dAngle < 0 )			dAngle += 3.1415926535f;
			dAngle += 3.1415926535f;
		}
		else if (dy < 0 )
			if( dAngle < 0 )			dAngle += 3.1415926535f;
	}
	if( dx > 0 && dy == 0 )				dAngle += 3.1415926535f;
	return dAngle;
}

void CPatternMatch::ExtractStartEndAngle( RECT32f roi, Path2D path )
{
	// 시작점의 각도 측정
	if( path.size() == 0 )
		return;
	double dWidth = roi.r - roi.l;
	double dHeight = roi.b - roi.t;
	m_nRate = 0;
	double dsx = ((dWidth - (path[0].x - roi.l) * 2) / dWidth);
	double dsy = ((dHeight - (path[0].y - roi.t) * 2) / dHeight);
	m_dRadStart = CalcAngle( dsx, dsy );
	double dex = ((dWidth - (path[path.size()-1].x - roi.l) * 2) / dWidth);
	double dey = ((dHeight - (path[path.size()-1].y - roi.t) * 2) / dHeight);
	m_dRadEnd = CalcAngle( dex, dey );
	if( dWidth / dHeight < 3 )
	{
		{
			double dH = path[0].y - roi.t;
			double dRate = dH / dHeight;
			if( dRate > 0.7 )
				m_nStartAngle |= POS_BOTTOM;
			else if ( dRate < 0.3 )
				m_nStartAngle |= POS_TOP;
			else
				m_nStartAngle |= POS_NONE;
		}
		{
			double dH = path[path.size()-1].y - roi.t;
			double dRate = dH / dHeight;
			if( dRate > 0.7 )
				m_nEndAngle |= POS_BOTTOM;
			else if ( dRate < 0.3 )
				m_nEndAngle |= POS_TOP;
			else
				m_nEndAngle |= POS_NONE;
		}
	}
	else if( dWidth / dHeight > 5 )	m_nRate = 2;
	if( dHeight / dWidth < 3 )
	{
		{
			double dW = path[0].x - roi.l;
			double dRate = dW / dWidth;
			if( dRate > 0.7 )
				m_nStartAngle |= POS_RIGHT;
			else if ( dRate < 0.3 )
				m_nStartAngle |= POS_LEFT;
			else
				m_nStartAngle |= POS_NONE;
		}
		{
			double dW = path[path.size()-1].x - roi.l;
			double dRate = dW / dWidth;
			if( dRate > 0.7 )
				m_nEndAngle |= POS_RIGHT;
			else if ( dRate < 0.3 )
				m_nEndAngle |= POS_LEFT;
			else
				m_nEndAngle |= POS_NONE;
		}
	}
	else if( dHeight / dWidth > 5 )	m_nRate = 1;
}

RECT32f CPatternMatch::ExtractROI( Path2D path )
{
	// 현재 패턴의 ROI
	RECT32f rt;
	rt.l = rt.t = MAXINT;
	rt.b = rt.r = 0;
	for( int i = 0; i < path.size(); i++ )
	{
		if( path[i].x < rt.l )
			rt.l = path[i].x;
		if( path[i].x > rt.r )
			rt.r = path[i].x;
		if( path[i].y < rt.t )
			rt.t = path[i].y;
		if( path[i].y > rt.b )
			rt.b = path[i].y;
	}
	return rt;
}

void CPatternMatch::LoadPattern( string path )
{
	// 패턴을 불러옴
	m_strPatternName.clear();
	m_pPatternArray.clear();

	// 이름 로드
	for( int i = (int)(path.size() - 1); i >= 0; i-- )
	{
		if( path[i] == '\\' )
		{
			for( i = i + 1; i < path.size(); i++ )
				m_strPatternName.push_back( path[i] );
			break;
		}
	}
	// 패턴 로드
	CvFileNode *pNode1 = NULL, *pNode2 = NULL;
	int nCount = 0;
	CvFileStorage *pStorage = cvOpenFileStorage( path.data(), NULL, CV_STORAGE_READ );
	
	pNode1 = cvGetFileNodeByName( pStorage, NULL, "Parameter" );
	pNode2 = cvGetFileNodeByName( pStorage, pNode1, "Count" );
	nCount = cvReadInt( pNode2 );
	
	float *pData = (float*)malloc(sizeof(float) * (nCount * 2) );
	memset( pData, 0, sizeof(float) * (nCount * 2) );
	pNode1 = cvGetFileNodeByName( pStorage, NULL, "Path2D" );
	cvReadRawData( pStorage, pNode1, pData, "f" );

	for( int i = 0; i < nCount; i++ )
	{
		CvPoint2D32f point;
		point.x = pData[i*2];
		point.y = pData[i*2+1];
		m_pPatternArray.push_back( point );
	}
	
	delete pData;
}

void CPatternMatch::DrawStatus( IplImage *pImage )
{
	// 인식 결과를 화면에 출력
	CvFont font = cvFont(1.5, 2);
	memset( m_pStatus->imageData, 0, m_pStatus->imageSize );


	char strTemp[256];

	memset( strTemp, 0, 256 );
	sprintf( strTemp, "Recognized Result" );
	cvPutText( m_pStatus, strTemp, cvPoint(10, 70) , &font, cvScalar( 255, 255, 255 ) );

	memset( strTemp, 0, 256 );
	sprintf( strTemp, "Pattern type", m_result.m_strName.data() );
	cvPutText( m_pStatus, strTemp, cvPoint(10, 110) , &font, cvScalar( 255, 255, 255 ) );

	if( m_result.m_dScore < m_dMinScore || (int)(m_PatternRecognition.m_LineSegment.m_arrResult.size()) < 3 )
	{
		memset( strTemp, 0, 256 );
		sprintf( strTemp, "RESET" );
		cvPutText( m_pStatus, strTemp, cvPoint(220, 110) , &font, cvScalar( 255, 255, 255 ) );
	}
	else
	{
		memset( strTemp, 0, 256 );
		sprintf( strTemp, "Input" );
		cvPutText( m_pStatus, strTemp, cvPoint(220, 110) , &font, cvScalar( 255, 255, 255 ) );
	}
	
	memset( strTemp, 0, 256 );
	sprintf( strTemp, "%s", m_result.m_strName.data() );
	cvPutText( m_pStatus, strTemp, cvPoint(320, 110) , &font, cvScalar( 255, 255, 255 ) );
	
	memset( strTemp, 0, 256 );
	sprintf( strTemp, "Matching Score" );
	cvPutText( m_pStatus, strTemp, cvPoint(10, 150) , &font, cvScalar( 255, 255, 255 ) );

	memset( strTemp, 0, 256 );
	sprintf( strTemp, "%lf", m_result.m_dScore );
	cvPutText( m_pStatus, strTemp, cvPoint(220, 150) , &font, cvScalar( 255, 255, 255 ) );

	memcpy( pImage->imageData, m_pStatus->imageData + 30 * m_pStatus->widthStep, m_pStatus->imageSize - 30 * m_pStatus->widthStep );
}

CResult CPatternMatch::Matching( CvPointArray pArray )
{
	// 입력 패턴을 미리 저장된 패턴과 매칭함
	Path2D path;
	CResult result, result_org;
	CLineSegment segment;
	memset( &result, 0, sizeof(CResult) );
	m_nStartAngle = 0;
	m_nEndAngle = 0;
	m_nRate = 0;
	m_dRadStart = 0;
	m_dRadEnd = 0;
	for( int i = 0; i < pArray.size(); i ++ )
	{
		CvPoint2D32f pt = cvPoint2D32f( pArray[i].x, pArray[i].y );
		path.push_back( pt );
	}
	segment.Input( path );
	segment.Process();

	if( m_nSetLineMode )
	{
		path = m_PatternRecognition.LineSegement( path );
		if( path.size() == 0 )
			return result;
	}

	RECT32f rt = ExtractROI( path );
	ExtractStartEndAngle( rt, path );
	
	TEMPLATE temp;
	temp.nStartAngle = m_nStartAngle;
	temp.nEndAngle = m_nEndAngle;
	temp.dRadStart = m_dRadStart;
	temp.dRadEnd = m_dRadEnd;
	temp.path = path;
	temp.nRate = m_nRate;
	result = m_PatternRecognition.ModRecognizer( temp );
	result_org = m_PatternRecognition.$1Recognizer( temp.path );
	m_result = result;
	m_input = temp;

	return result;
}