#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include <exception>

#include <opencv\cv.h>
#include <opencv\cvaux.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional> 
#include <limits>
#include <math.h>

using namespace std;

typedef vector<CvPoint> CvPointArray;

//#define GET2D8U(IMAGE,X,Y) (*( ( (uchar*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))
//#define GET2D8U3CH(IMAGE,X,Y) ( ( (uchar*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + ( 3 * (X) ) )
//#define GET2D16U(IMAGE,X,Y) (*( ( (ushort*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))
//#define GET2D16S(IMAGE,X,Y) (*( ( (short*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))
//#define GET2D32F(IMAGE,X,Y) (*( ( (float*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))

#define GET2D8U(IMAGE,X,Y) (*( ( (uchar*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) )) //unsigned 8진수 이미지 값 추출 메크로
#define GET2D8U3CH(IMAGE,X,Y) ( ( (uchar*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + ( 3 * (X) ) )//unsigned 8진수 3채널 이미지 값 추출 메크로
#define GET2D8U4CH(IMAGE,X,Y) ( ( (uchar*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + ( 4 * (X) ) )//unsigned 8진수 4채널 이미지 값 추출 메크로
//use :  GET2D8U3CH(IMAGE,X,Y)[0], GET2D8U3CH(IMAGE,X,Y)[1], GET2D8U3CH(IMAGE,X,Y)[2]
#define GET2D16U(IMAGE,X,Y) (*( ( (ushort*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))//unsigned 16진수 이미지 값 추출 메크로
#define GET2D16S(IMAGE,X,Y) (*( ( (short*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))//signed 16진수 이미지 값 추출 메크로
#define GET2D32F(IMAGE,X,Y) (*( ( (float*)( ( (IMAGE) -> imageData ) + (Y) * ( (IMAGE) -> widthStep ) ) ) + (X) ))//float 32진수 이미지 값 추출 메크로

#define DEPTH_WIDTH 320   //320
#define DEPTH_HEIGHT 240  //240

#define COLOR_WIDTH 640   //640
#define COLOR_HEIGHT 480  //480

#define INTEGRAL2RECT_SUM(INTEGRAL,L,T,R,B) \
	( GET2D32F(INTEGRAL,R+1,B+1)-GET2D32F(INTEGRAL,R+1,T)-GET2D32F(INTEGRAL,L,B+1)+GET2D32F(INTEGRAL,L,T) )

typedef unsigned int UINT;
typedef int INT;
#define MAXUINT     ((UINT)~((UINT)0))
#define MAXINT      ((INT)(MAXUINT >> 1))
#define MININT      ((INT)~MAXINT)

#define POS_LEFT	1
#define POS_RIGHT	2
#define POS_TOP		4
#define POS_BOTTOM	8
#define POS_NONE	0

#define NON_MOVE