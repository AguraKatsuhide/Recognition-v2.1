#pragma once

#include <Kinect.h>
#include "PatternMatch.h"
#include "KeyPrint.h"
#include <math.h>
#include <Windows.h>
#include <mmsystem.h>

using namespace std;

//Context g_Context;
//DepthNode g_DNode;

BOOL g_bIsCloseMode = 0;
BOOL g_bDenoising = 1;
int g_uFrameRate = 30;
int g_uMinInfra = 405;

IplImage* g_imgDepthBuffer;
BOOL g_bFlagNewFrame = false;
CRITICAL_SECTION g_csDepthFrame;
HANDLE g_hThreadDepth;
DWORD g_IDThreadDepth;
volatile bool g_bClose = false;

//ColorNode g_CNode;

double g_dMinMatchRate;


IplImage *g_imgY;
IplImage *g_imgCb;
IplImage *g_imgCr;
IplImage* g_imgCalBuffer;
IplImage* g_imgCalibration;
IplImage* g_imgPreProcess;
IplImage* g_imgProcess1;
IplImage* g_imgProcess2;
CPatternMatch g_PatternMatchKorean;	// 패턴 인식 클래스
//CFingerTracker g_FingerTracker;		// 손끝 검출 및 추적 글래스
CKeyPrint g_KeyPrint;				// 인식 결과를 글자로 바꿔주는 함수. 현재 사용하지 않음.
int g_nTailCut;						// 인식에 있어 입력 데이터의 끝을 무시하는 개수
/////////////////////////////////////////////DS325 Code ---



bool initProgram();
void initPattern();
void initHelp();
void readSettingsFromINI();

void InitKinectV2(); // kinect2 카메라 초기화 함수
void exitKinectV2(); // kinect2 카메라 해제 함수

void initCamera();
void getDepthFrame();
void getDepthRanged();
void getBinary();
void processFrame();
void exitProgram();
void exitCamera();
void processKeyEvent(int key);


void onNodeConnected();

void SendResult(CResult res, CvPointArray pArray);
void SendResult(CResult res);
void SendTipPoint(CvPoint pPoint, int nIndex);

//bool checkReady();

//추가한 함수

//void FindIndex(IplImage* pBinary);
//CvPoint ExtractTopPoint(IplImage* pBinary);
//CvPoint* ExtractCheckPoint(IplImage *pBinary, CvPoint ptTop, int nFoot);
//int ExtractMaxXDist(IplImage *pBinary);
//
//void CheckTouch();

BOOL checkReady();
void getCenter(IplImage* depthImage);

//CV_MAT_ELEM_PTR
CvMemStorage* g_memStorage;
int g_uRangeMin;
int g_uRangeMax;

int g_uMaxDepth = 4500;
int g_uMinDepth = 500; //500

int g_uThresholdMin = 0;
int g_uThresholdMax = 255;

IplImage* g_imgDepth;
IplImage* g_imgDepthPrepro;
IplImage* g_imgDepthRangedGray;
IplImage* g_imgColor;
IplImage* g_imgBinary;

IplImage* g_IRimgBinary; // IR 이진화 영상 정의
IplImage* g_imgInfra;// 적외선 영상 정의

bool g_bRun = true;
char g_strFps[20] = "0.00fps";
int g_nTimeStart;
int g_nTimeEnd;
int g_nVTouch;
int g_nPrevX;
int g_nPrevY;
int g_nIndex;

//////////////////////////////////////////////
// Network
WSADATA g_wsaData;
SOCKET g_socket;
SOCKADDR_IN g_socketInfo;
short g_sPort;
char g_strIP[256];

SOCKET g_socketRecv;
SOCKADDR_IN g_socketInfoRecv;
short g_sPortRecv;
//char g_strIPRecv[256];
HANDLE g_hThreadRecv;
int g_bStart;


void initNetwork();
void exitNetwork();

int g_bNetwork;

//추가한 변수

int g_centerX;
int g_centerY;

int g_nDist;

int g_thumbX;
int g_thumbY;

CvPoint g_index;

float g_dist_index;
float g_dist_thumb;

char g_centerXtext[50] = "";
char g_centerYtext[50] = "";

char g_indexXtext[50] = "";
char g_indexYtext[50] = "";

char g_thumbXtext[50] = "";
char g_thumbYtext[50] = "";

char g_indexdist[50] = "";
char g_thumbdist[50] = "";

char g_clickText[50] = "";

int g_bVTouch = 0;
int g_bVTouch_pre = 0;

CvPointArray g_arr;
CvPointArray g_arr_pre;
CvPoint g_arr_result = cvPoint(-1,-1);

char g_dist_index_Text[50] = "";

int g_bInputStatus;
//
//CvPoint center_index = cvPoint(-1,-1);
//CvPoint index_out1[2];
//CvPoint index_out2[2];
//CvPoint2D32f index_output[2];


//CvMemStorage* g_memStorage_;
//
//IplImage* g_pDepth;
//
//int g_nDepth;
//
//int g_nDepthThres;
//
//double g_sin;
//double g_cos;
//
//double g_Alpha;
//
//CvPoint g_ptSampleLine[40];
//
//IplImage* g_contourImage;
//int g_nStateCheck=5;
//
//BOOL g_bCheckLine[40];
//
//CvPoint g_top_prev = cvPoint(-1, -1);
//CvPoint g_left_prev = cvPoint(-1, -1);
//
//double g_thumb_dist_prev[20] = { 0 };

CvPoint g_Center = cvPoint(0, 0); // 중점