#include "StdAfx.h"
#include "1$Recognizer.h"


C1$Recognizer::C1$Recognizer(void)
{
	m_nNumResamplePoints = 64;
	m_nSquareSize = 250;
	m_dGoldenRatio = 0.5 * ( -1.0 + sqrt(5.0) );
	m_dHalfDiagonal = 0.5 * sqrt( double(m_nSquareSize* m_nSquareSize) + double(m_nSquareSize* m_nSquareSize));
	m_dAngleRange = 15.0f;
	m_dAnglePrecision = 2.0f;
}


C1$Recognizer::~C1$Recognizer(void)
{
}

CResult C1$Recognizer::ModRecognizer( TEMPLATE temp )
{
	CResult res;
	memset( &res, 0, sizeof(CResult) );
	if( (int)(m_mapTemplate.size()) < 1 )
		return res;

	temp.path = NormalizedPath( temp.path );
	
	ITEMPLATE IT_Max;

	vector<CResult> vecResult;
	int bMatched = FALSE;
	for( ITEMPLATE IT = m_mapTemplate.begin(); IT != m_mapTemplate.end(); IT++ )
	{
		double dDiffS = fabs(temp.dRadStart - IT->second.dRadStart);
		double dDiffE = fabs(temp.dRadEnd - IT->second.dRadEnd);
		double dThresVal = 3.1415926535f * (m_dDegree / 180.0f);
		if(/* temp.nRate == IT->second.nRate && temp.nRate == 0 &&*/
			(dDiffS < dThresVal ||
			 fabs(3.1415926535f * 2 - dDiffS) < dThresVal ||
			 fabs(3.1415926535f * 2 + dDiffS) < dThresVal ||
			 temp.nStartAngle == POS_NONE || IT->second.nStartAngle == POS_NONE )&&
			(dDiffE < dThresVal ||
			 fabs(3.1415926535f * 2 - dDiffE) < dThresVal ||
			 fabs(3.1415926535f * 2 + dDiffE) < dThresVal ||
			 temp.nEndAngle == POS_NONE || IT->second.nEndAngle == POS_NONE) )
		{
			bMatched = TRUE;
			Path2D& refTemplate = IT->second.path;
			double dDistance = DistanceAtBestAngle( temp.path, refTemplate );
			double dScore = 1.0f - (dDistance / m_dHalfDiagonal);
			string strGestureName = IT->first;
			string strCode = IT->second.strMappingCode;

			CResult bestMatch( strGestureName, strCode, dScore );
			vecResult.push_back( bestMatch );
		}
	}
	if( !bMatched )
		return res;

	std::sort( vecResult.begin(), vecResult.end(), CResult::SortFunc);
	return vecResult[0];
}

void C1$Recognizer::AddTemplete( string strName, string strMappingCode, Path2D path, int nStartAngle, int nEndAngle, int nRate, double dRadStart, double dRadEnd )
{
	m_mapTemplate[strName].path = path;
	m_mapTemplate[strName].strMappingCode;
	m_mapTemplate[strName].nStartAngle = nStartAngle;
	m_mapTemplate[strName].nEndAngle = nEndAngle;
	m_mapTemplate[strName].nRate = nRate;
	m_mapTemplate[strName].dRadStart = dRadStart;
	m_mapTemplate[strName].dRadEnd = dRadEnd;
}

CResult C1$Recognizer::$1Recognizer( Path2D path )
{
	CResult res;
	if( m_mapTemplate.size() < 1 )
		return res;

	path = NormalizedPath( path );
	
	ITEMPLATE IT_Max;

	vector<CResult> vecResult;
	for( ITEMPLATE IT = m_mapTemplate.begin(); IT != m_mapTemplate.end(); IT++ )
	{
		Path2D& refTemplate = IT->second.path;
		double dDistance = DistanceAtBestAngle( path, refTemplate );
		double dScore = 1.0f - (dDistance / m_dHalfDiagonal);
		string strGestureName = IT->first;
		string strCode = IT->second.strMappingCode;

		CResult bestMatch( strGestureName, strCode, dScore );

		vecResult.push_back( bestMatch );
	}

	std::sort( vecResult.begin(), vecResult.end(), CResult::SortFunc);
	return vecResult[0];
}

double C1$Recognizer::DistanceAtBestAngle( Path2D path, Path2D refTemplate )
{
	double dStartRange = -m_dAngleRange;
	double dEndRange = m_dAngleRange;

	double dAngle1 = m_dGoldenRatio * dStartRange + (1.0f - m_dGoldenRatio) * dEndRange;
	double dScore1 = DistanceAtAngle( path, refTemplate, dAngle1 );

	double dAngle2 = (1.0f - m_dGoldenRatio) * dStartRange + m_dGoldenRatio * dEndRange;
	double dScore2 = DistanceAtAngle( path, refTemplate, dAngle2 );

	while( fabs(dEndRange - dStartRange) > m_dAnglePrecision )
	{
		if( dScore1 < dScore2 )
		{
			dEndRange = dAngle2;
			dAngle2 = dAngle1;
			dScore2 = dScore1;
			dAngle1 = m_dGoldenRatio * dStartRange + (1.0f - m_dGoldenRatio) * dEndRange;
			dScore1 = DistanceAtAngle( path, refTemplate, dAngle1 );
		}
		else
		{
			dStartRange = dAngle1;
			dAngle1 = dAngle2;
			dScore1 = dScore2;
			dAngle2 = (1.0f - m_dGoldenRatio) * dStartRange + m_dGoldenRatio * dEndRange;
			dScore2 = DistanceAtAngle( path, refTemplate, dAngle2 );
		}
	}

	return min( dScore1, dScore2 );
}

double C1$Recognizer::DistanceAtAngle( Path2D path, Path2D refTemplate, double dRotation)
{
	Path2D newPath = RotateBy( path, dRotation );
	return PathDistance( newPath, refTemplate );
}

double C1$Recognizer::PathDistance( Path2D path1, Path2D path2 )
{
	double dDistance = 0.0f;
	for( int i = 0; i < (int)path1.size(); i++ )
		dDistance += GetEuclideanDistance( path1[i], path2[i] );
	return (dDistance / (double)path1.size() );
}

Path2D C1$Recognizer::NormalizedPath( Path2D path, BOOL bIgnoreFirstLine )
{
	path = ResamplingPath( path );
	m_LineSegment.Input( path );
	m_LineSegment.Process( bIgnoreFirstLine );
	path = ResamplingPath( m_LineSegment.m_arrResult );
	path = Rotation2Zero( path );
	path = Scale2Square( path );
	path = Translate2Origin( path );
	return path;
}

Path2D C1$Recognizer::LineSegement( Path2D path )
{
	path = ResamplingPath( path );
	m_LineSegment.Input( path );
	m_LineSegment.Process( TRUE );
	if( m_LineSegment.m_arrResult.size() == 0 )
		path.clear();
	else
		path = ResamplingPath( m_LineSegment.m_arrResult );
	return path;
}

Path2D C1$Recognizer::ResamplingPath( Path2D path )
{
	double dInterval = PathLength(path) / (m_nNumResamplePoints - 1);
	double dSavedDistnace = 0;
	Path2D newPath;
	
	newPath.push_back( path.front() );
	for( int i = 0; i < (int)(path.size() - 1); i++ )
	{
		CvPoint2D32f ptCurrPoint = path[i];
		CvPoint2D32f ptNextPoint = path[i+1];

		double dDistance = GetEuclideanDistance( ptCurrPoint, ptNextPoint );
		if( (dSavedDistnace + dDistance) >= dInterval )
		{
			double qx = ptCurrPoint.x + ((dInterval - dSavedDistnace) / dDistance) * (ptNextPoint.x - ptCurrPoint.x);
			double qy = ptCurrPoint.y + ((dInterval - dSavedDistnace) / dDistance) * (ptNextPoint.y - ptCurrPoint.y);
			CvPoint2D32f pt = cvPoint2D32f( qx, qy );
			newPath.push_back( pt );
			path.insert( path.begin() + i + 1, pt );
			dSavedDistnace = 0;
		}
		else
			dSavedDistnace += dDistance;
	}

	if( newPath.size() == (m_nNumResamplePoints - 1) )
	{
		newPath.push_back( path.back() );
	}

	return newPath;
}

Path2D C1$Recognizer::Rotation2Zero( Path2D path )
{
	CvPoint2D32f cen = Centroid( path );
	double dRotation = atan2( cen.y - path[0].y, cen.x - path[0].x );
	return RotateBy( path, -dRotation );
}

Path2D C1$Recognizer::Scale2Square( Path2D path )
{
	Rect32f box = BoundingBox( path );
	Path2D newPath;

	for( Path2DIterator i = path.begin(); i != path.end(); i++ )
	{
		CvPoint2D32f pt = *i;
		float qx = pt.x * (m_nSquareSize / box.width );
		float qy = pt.y * (m_nSquareSize / box.height );
		newPath.push_back( cvPoint2D32f(qx, qy) );
	}
	return newPath;
}

Path2D C1$Recognizer::Translate2Origin( Path2D path )
{
	CvPoint2D32f ptCen = Centroid( path );
	Path2D newPath;
	for( Path2DIterator i = path.begin(); i != path.end(); i++ )
	{
		CvPoint2D32f pt = *i;
		double qx = pt.x - ptCen.x;
		double qy = pt.y - ptCen.y;
		newPath.push_back( cvPoint2D32f(qx, qy) );
	}
	return newPath;
}

double C1$Recognizer::PathLength( Path2D path )
{
	double dDistance = 0;

	for( int i = 0; i < (int)(path.size() - 1); i++ )
		dDistance += GetEuclideanDistance( path[i], path[i+1] );
	return dDistance;
}

double C1$Recognizer::GetEuclideanDistance( CvPoint2D32f pt1, CvPoint2D32f pt2 )
{
	double dx = pt1.x - pt2.x;
	double dy = pt1.y - pt2.y;

	return sqrt( (dx * dx) + (dy * dy) );
}

CvPoint2D32f C1$Recognizer::Centroid( Path2D path )
{
	double dx = 0;
	double dy = 0;

	for( Path2DIterator i = path.begin(); i != path.end(); i++ )
	{
		CvPoint2D32f pt = *i;
		dx += pt.x;
		dy += pt.y;
	}
	dx /= (double)path.size();
	dy /= (double)path.size();
	return cvPoint2D32f( dx, dy );
}

Path2D C1$Recognizer::RotateBy( Path2D path, double dRotation )
{
	CvPoint2D32f ptCen = Centroid( path );
	double dCos = cos( dRotation );
	double dSin = sin( dRotation );

	Path2D newPath;
	for( Path2DIterator i = path.begin(); i != path.end(); i++ )
	{
		CvPoint2D32f pt = *i;
		double qx = (pt.x - ptCen.x) * dCos - (pt.y - ptCen.y) * dSin + ptCen.x;
		double qy = (pt.x - ptCen.x) * dSin + (pt.y - ptCen.y) * dCos + ptCen.y;
		newPath.push_back( cvPoint2D32f(qx, qy) );
	}
	return newPath;
}

Rect32f C1$Recognizer::BoundingBox( Path2D path )
{
	float fMinX = MAX_FLOAT;
	float fMinY = MAX_FLOAT;
	float fMaxX = -MAX_FLOAT;
	float fMaxY = -MAX_FLOAT;
	Rect32f rtResult;

	for( Path2DIterator i = path.begin(); i != path.end(); i++ )
	{
		CvPoint2D32f pt = *i;
		if( pt.x < fMinX )	fMinX = pt.x;
		if( pt.x > fMaxX )	fMaxX = pt.x;
		if( pt.y < fMinY )	fMinY = pt.y;
		if( pt.y > fMaxY )	fMaxY = pt.y;
	}
	rtResult.x = fMinX;
	rtResult.y = fMinY;
	rtResult.width = fMaxX - fMinX;
	rtResult.height = fMaxY - fMinY;

	return rtResult;
}