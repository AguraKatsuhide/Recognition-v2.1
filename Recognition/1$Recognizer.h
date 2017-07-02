#pragma once

#include "stdafx.h"
#include "LineSegment.h"
#define MAX_FLOAT (std::numeric_limits<float>::max())

typedef vector<CvPoint2D32f>::iterator Path2DIterator;

typedef struct _Rect32f
{
	float x;
	float y;
	float width;
	float height;
} Rect32f;

typedef struct _TEMPLATE
{
	Path2D path;
	string strMappingCode;
	int nStartAngle;			// 9방향 좌 좌상 상 우상 우 우하 하 좌하 중앙
	int nEndAngle;				// 시작점과 끝점 매칭
	int nRate;					// 0 : Normal , 1 : W:H = 1:2 이상, 2 : W:H = 2:1이상
	double dRadStart;
	double dRadEnd;
} TEMPLATE;

typedef map<string, TEMPLATE>::iterator ITEMPLATE;

class CResult
{
public:
	string m_strName;
	string m_strOutCode;
	double m_dScore;
	// 생성자
	CResult()
	{
		m_strName		= "";
		m_strOutCode	= "";
		m_dScore		= 0.f;
	}

	CResult( string _strName, string _strOutCode, double _dScore)
	{
		m_strName		= _strName;
		m_strOutCode	= _strOutCode;
		m_dScore		= _dScore;
	}

	// 매칭 점수에 따라 정렬할때 쓰이는 함수
	static bool SortFunc(const CResult& lho, const CResult& rho)
	{
		return ( lho.m_dScore > rho.m_dScore);
	}	
};

class C1$Recognizer
{
public:
	C1$Recognizer(void);
	~C1$Recognizer(void);

	
public:
	Path2D NormalizedPath( Path2D path, BOOL bIgnoreFirstLine = FALSE );
	Path2D LineSegement( Path2D path );
	CResult ModRecognizer( TEMPLATE temp );
	CResult $1Recognizer( Path2D path );
	void AddTemplete( string strName, string strMappingCode, Path2D path, int nStartAngle, int nEndAngle, int nRate, double dRadStart, double dRadEnd );
	map<string, TEMPLATE> m_mapTemplate;
	CLineSegment m_LineSegment;
	Path2D ResamplingPath( Path2D path );

	double m_dDegree;
	int m_nNumResamplePoints;

private:
	int m_nSquareSize;
	double m_dGoldenRatio;
	double m_dHalfDiagonal;
	double m_dAngleRange;
	double m_dAnglePrecision;

	Path2D Rotation2Zero( Path2D path );
	Path2D Scale2Square( Path2D path );
	Path2D Translate2Origin( Path2D path );

	double PathLength( Path2D path );
	double GetEuclideanDistance( CvPoint2D32f pt1, CvPoint2D32f pt2 );
	
	CvPoint2D32f Centroid( Path2D path );
	Path2D RotateBy( Path2D path, double dRotation );
	Rect32f BoundingBox( Path2D path );
	
	double DistanceAtBestAngle( Path2D path, Path2D refTemplate );
	double DistanceAtAngle( Path2D path, Path2D aTemplate, double dRotation );
	double PathDistance( Path2D path1, Path2D path2 );
};

