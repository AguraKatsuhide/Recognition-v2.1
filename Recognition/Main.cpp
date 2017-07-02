#include <WinSock2.h>
#include "stdafx.h"
#include "Main.h"


template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease) //kinect2 메모리를 안전하게 해제하는 함수
{
	if (pInterfaceToRelease != NULL)
	{

		pInterfaceToRelease->Release(); // kinect2 내 interface를 release.
		pInterfaceToRelease = NULL;
	}
}

IKinectSensor*          g_pKinectSensor = 0; //kinect2 센서
IDepthFrameReader*      g_pDepthFrameReader;//깊이 값 읽는 변수 선언
IInfraredFrameReader*	g_pInfraredFrameReader; // 적외선 값을 읽는 변수 선언
IColorFrameReader*		g_pColorFrameReader;// color 값을 읽은 변수 선언

ICoordinateMapper*		g_pCoordinateMapper = 0; // mapping 하는 함수
ColorSpacePoint*		g_pColorCoordinates = 0; // 깊이 좌표


DWORD WINAPI ThreadProcRecv(LPVOID lParam)
{
	// KII-Control에서 UDP통신을 받는 부분. 
	// 구현 형식이 Recv함수가 받을 때까지 기다리는 형식이라 Thread를 생성해 별도로 받도록 구현
	while (g_bRun)
	{
		char pBuf[10] = { 0 };

		int nInfoSize = sizeof(g_socketInfoRecv);
		int nRecvSize = recvfrom(g_socketRecv, pBuf, 10, 0, (struct sockaddr*)&g_socketInfoRecv, &nInfoSize);
		printf("UDP RECV \"%s\" FROM %d.%d.%d.%d(%d)\r\n",
			pBuf,
			g_socketInfoRecv.sin_addr.S_un.S_un_b.s_b1,
			g_socketInfoRecv.sin_addr.S_un.S_un_b.s_b2,
			g_socketInfoRecv.sin_addr.S_un.S_un_b.s_b3,
			g_socketInfoRecv.sin_addr.S_un.S_un_b.s_b4,
			g_sPortRecv);
		if (!strcmp(pBuf, "START"))		// START라고 받음 시작
			g_bStart = 1;
		if (!strcmp(pBuf, "STOP"))		// STOP이라고 받음 멈춤
			g_bStart = 0;
	}
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{

	if (!initProgram())
	{
		printf("Fail to Load Camera\r\n");
		return 0;
	}
	initPattern();		// 패턴 불러옴

	// Help창 생성
	IplImage *pHelp = cvLoadImage("HelpKorean3.png");
	cvShowImage("Help", pHelp);

	//processFrame();
	while (g_bRun)
	{
		if (!g_bStart)	// KII-Control에서 제어하는 부분
		{
			processKeyEvent(cvWaitKey(5));
			continue;
		}

		int t = GetTickCount();

		getDepthFrame();
		getDepthRanged();

		///////////depth image 최적화/////////////////
		IplImage* img = cvCreateImage(cvSize(512, 424), 8, 1);//contour을 따기 위한 이미지.
		IplImage* imgMask = cvCreateImage(cvSize(512, 424), 8, 1);//contour를 적용한 마스크를 임시저장하는 이미지.

		//cvResize(g_imgDepthto8, g_imgDepthRangedGray, CV_INTER_CUBIC);

		// 
		cvCopy(g_imgDepthRangedGray, img);//-- 가장 큰 깊이 값 물체를 빼고 나머지 모두 제외

		CvSeq* contours = 0;	//외곽선.
		cvFindContours(g_imgDepthRangedGray, g_memStorage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//외곽선 검출
		CvSeq* select_contour = 0;
		float max_area = 0;

		while (contours)
		{
			float area = abs(cvContourArea(contours)); //넓이
			if (area>max_area)		//기존의 최대 이미지 크기보다 검색한 크기가 크면 해당 이미지를 최대 이미지로 저장
			{
				max_area = area;   //최대 이미지 크기 설정
				select_contour = contours;
			}

			contours = contours->h_next;
		}

		cvZero(imgMask); //imgMask를 0으로 초기화
		cvDrawContours(imgMask, select_contour, cvScalar(255), cvScalar(255), 0, CV_FILLED);//imgMask에 잘라낸 이미지 저장

		cvClearMemStorage(g_memStorage); //외곽선 정보 저장한 데이터 해제(없으면 메모리 누수 생김)

		cvZero(g_imgDepthRangedGray);
		cvCopy(img, g_imgDepthRangedGray, imgMask);//img에서 imgMask에 해당하는 부분을 imgProcess에 저장--//
		// 여기서 마스크 할때 임의로 넣은 원본 이미지와 마스킹 이미지를 적용해서 다시 g_imgDepthRangedGray에
		// 넣습니다. 결과가 다시 g_imgDepthRangedGray에 넣어지는 겁니다. 그리고 그걸
		// 쓰기위해 g_imgDepthPrepro 640 480 이미지에 다시 리사이즈 해서 g_imgDepthPrepro 을 씁니다.
		cvResize(g_imgDepthRangedGray, g_imgDepthPrepro, CV_INTER_CUBIC);

		cvReleaseImage(&img);//이미지 파일 메모리 해제
		cvReleaseImage(&imgMask);//이미지 파일 메모리 해제
		////////////////////////////
		//cvShowImage("16진수 전처리된 이미지", g_imgDepthPrepro);
		//getDepthRanged();

		getBinary();
		//cvShowImage("바이너리 ", g_imgBinary);
		processFrame();
		processKeyEvent(cvWaitKey(5));

		t = GetTickCount() - t;
		sprintf_s(g_strFps, 20, "%.2ffps", 1000.0f / t);


	}
	cvReleaseImage(&pHelp);
	exitProgram();
	return 0;
}

void SendResult(CResult res)
{
	// UDP를 통해 KII-GUI에 결과를 보내줌 (K01, K02 등)
	if (!g_bNetwork)		// 네트워크 관련 함수가 초기화 되지 않음 보내지 않음.
		return;

	char *strTemp = new char[4];
	memset(strTemp, 0, 4);
	strTemp[0] = res.m_strName[0];
	strTemp[1] = res.m_strName[1];
	strTemp[2] = res.m_strName[2];

	//	OutputDebugString( strTemp );
	printf("UDP SEND \"%s\" TO %d.%d.%d.%d(%d)\r\n",
		strTemp,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b1,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b2,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b3,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b4,
		g_sPort);
	int nSendSize = sendto(g_socket, strTemp, 4, 0, (struct sockaddr*)&g_socketInfo, sizeof(g_socketInfo));
	delete strTemp;
}

void SendTipPoint(CvPoint pPoint, int nIndex)
{
	// UDP통신을 이용해 손끝 점을 보냄 (형식 예제: P01X02Y03)
	if (!g_bNetwork)
		return;

	// 손끝 위치를 0~99 범위로 재 수정함.
	int nX = (pPoint.x  - 160) * 99 / (480 - 160);
	int nY = (pPoint.y  - 80) * 99 / (370 - 80);
	if (nX < 0) nX = 0;
	if (nX > 99) nX = 99;
	if (nY < 0) nY = 0;
	if (nY > 99) nY = 99;

	if (nX == g_nPrevX && nY == g_nPrevY)
		return;
	g_nPrevX = nX;
	g_nPrevY = nY;
	if (nIndex == 0)
		g_nIndex = 0;
	else
		g_nIndex++;
	if (g_nIndex > 99)
		g_nIndex = 99;

	char *strTemp = new char[10];
	memset(strTemp, 0, 10);
	strTemp[0] = 'P';
	sprintf_s(&strTemp[1], 9, "%02d", g_nIndex);
	strTemp[3] = 'X';
	sprintf_s(&strTemp[4], 6, "%02d", nX);
	strTemp[6] = 'Y';
	sprintf_s(&strTemp[7], 3, "%02d", nY);
	strTemp[9] = NULL;

	//	OutputDebugString( strTemp );

	printf("UDP SEND \"%s\" TO %d.%d.%d.%d(%d)\r\n",
		strTemp,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b1,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b2,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b3,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b4,
		g_sPort);
	int nSendSize = sendto(g_socket, strTemp, 10, 0, (struct sockaddr*)&g_socketInfo, sizeof(g_socketInfo));
	delete strTemp;
}

void SendResult(CResult res, CvPointArray pArray)
{
	// 사용하지 않음.
	if (!g_bNetwork)
		return;

	Path2D path;
	for (int i = 0; i < (int)pArray.size(); i++)
	{
		CvPoint2D32f ptf;
		ptf.x = (float)pArray[i].x;
		ptf.y = (float)pArray[i].y;
		path.push_back(ptf);
	}
	path = g_PatternMatchKorean.m_PatternRecognition.ResamplingPath(path);

	char *pBuf;
	int nBufSize = path.size() * 9 + 3 + 5;
	int nBufCount = 0;
	pBuf = new char[nBufSize];
	memset(pBuf, 0, nBufSize);

	for (int i = 0; i < 3; i++)			// 이름
		pBuf[i] = res.m_strName[i];

	int nMinX = 640, nMinY = 480, nMaxX = 0, nMaxY = 0; // process 크기관련 480
	for (int i = 0; i < (int)path.size(); i++)
	{
		if (nMinX > path[i].x)				nMinX = (int)path[i].x;
		if (nMinY > path[i].y)				nMinY = (int)path[i].y;
		if (nMaxX < path[i].x)				nMaxX = (int)path[i].x;
		if (nMaxY < path[i].y)				nMaxY = (int)path[i].y;
	}

	nBufCount = 3;

	for (int i = 0; i < path.size(); i++)
	{
		char strtemp[10] = { 0 };
		int x = (int)((path[i].x - nMinX) * 99 / (nMaxX - nMinX));
		int y = (int)((path[i].y - nMinY) * 99 / (nMaxY - nMinY));
		if (y > 99)		y = 99;
		if (x > 99)		x = 99;
		if (x < 0)			x = 0;
		if (y < 0)			y = 0;

		memset(strtemp, 0, 10);
		sprintf_s(strtemp, 10, "%02d", i);
		pBuf[nBufCount++] = 'P';
		pBuf[nBufCount++] = strtemp[0];
		pBuf[nBufCount++] = strtemp[1];

		memset(strtemp, 0, 10);
		sprintf_s(strtemp, 10, "%02d", x);
		pBuf[nBufCount++] = 'X';
		pBuf[nBufCount++] = strtemp[0];
		pBuf[nBufCount++] = strtemp[1];

		memset(strtemp, 0, 10);
		sprintf_s(strtemp, 10, "%02d", y);
		pBuf[nBufCount++] = 'Y';
		pBuf[nBufCount++] = strtemp[0];
		pBuf[nBufCount++] = strtemp[1];

	}

	printf("UDP SEND \"%s\" TO %d.%d.%d.%d(%d)\r\n",
		pBuf,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b1,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b2,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b3,
		g_socketInfo.sin_addr.S_un.S_un_b.s_b4,
		g_sPort);
	int nSendSize = sendto(g_socket, pBuf, nBufSize, 0, (struct sockaddr*)&g_socketInfo, sizeof(g_socketInfo));
	/*	if( nSendSize )
	{
	char strtemp[256] = {0};
	sprintf( strtemp, "SendSize = %d\r\n", nSendSize );
	OutputDebugString( strtemp );
	}*/
	delete pBuf;
}

void initPattern()
{
	// 패턴을 불러오는 함수
	TCHAR path[512];
	GetCurrentDirectory(512, path);  //프로젝트 경로
	char strPath[1024] = { 0 };
	for (int i = 1; i < 34; i++)	// 한글키 로드
	{
		if (i == 2 || i == 5 || i == 9 || i == 11 || i == 14 || i == 21 || i == 23 || i == 25 || i == 27)
			continue;
		sprintf_s(strPath, 1024, "%s\\Pattern\\Korean\\K%02d.xml", path, i);
		string str = strPath;
		g_PatternMatchKorean.AddPattern(str);
		str.clear();
	}
	for (int i = 1; i < 4; i++)	// 제어키 로드
	{
		sprintf_s(strPath, 1024, "%s\\Pattern\\Operation\\O%02d.xml", path, i);
		string str = strPath;
		g_PatternMatchKorean.AddPattern(str);
		str.clear();
	}
	g_PatternMatchKorean.m_PatternRecognition.m_LineSegment.m_arrResult.clear();
}

void initNetwork()
{
	// 네트워크 초기화 함수
	g_bNetwork = 1;
	if (WSAStartup(0x202, &g_wsaData) == SOCKET_ERROR)
	{
		WSACleanup();
		g_bNetwork = 0;
	}

	memset(&g_socketInfo, 0, sizeof(g_socketInfo));
	g_socketInfo.sin_family = AF_INET;
	g_socketInfo.sin_addr.S_un.S_addr = inet_addr(g_strIP);
	g_socketInfo.sin_port = htons(g_sPort);

	g_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset(&g_socketInfoRecv, 0, sizeof(g_socketInfoRecv));
	g_socketInfoRecv.sin_family = AF_INET;
	//	g_socketInfoRecv.sin_addr.S_un.S_addr = inet_addr( g_strIPRecv );
	g_socketInfoRecv.sin_port = htons(g_sPortRecv);
	g_socketInfoRecv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	g_socketRecv = socket(AF_INET, SOCK_DGRAM, 0);
	if (::bind(g_socketRecv, (struct sockaddr*)&g_socketInfoRecv, sizeof(g_socketInfoRecv)) == SOCKET_ERROR)
	{
		closesocket(g_socketRecv);
		WSACleanup();
	}
}

bool initProgram()
{
	readSettingsFromINI();
	initCamera();

	g_imgDepth = cvCreateImage(cvSize(512, 424), IPL_DEPTH_16U, 1); //IPL_DEPTH_16S

	g_imgDepthPrepro = cvCreateImage(cvSize(640, 480), 8, 1);

	g_imgDepthRangedGray = cvCreateImage(cvSize(512, 424), 8, 1); // 최종은 여기로 받아서 보내기.
	g_imgInfra = cvCreateImage(cvSize(512, 424), IPL_DEPTH_16U, 1);
	g_imgColor = cvCreateImage(cvSize(COLOR_WIDTH, COLOR_HEIGHT), 8, 4);
	g_imgBinary = cvCreateImage(cvSize(640, 480), 8, 1);
//	g_imgProcess1 = cvCreateImage(cvSize(COLOR_WIDTH, COLOR_HEIGHT), IPL_DEPTH_8U, 3);
	g_imgProcess2 = cvCreateImage(cvSize(COLOR_WIDTH, 160), IPL_DEPTH_8U, 3);
	//g_pDepth = cvCreateImage(cvSize(640, 480), 8, 1);
	//g_contourImage = cvCreateImage(cvSize(640, 480), 8, 1);
	//g_nDepth = cvCreateImage(cvSize(640, 480),IPL_DEPTH_16U , 1);
//	memset(g_imgProcess1->imageData, 0, g_imgProcess1->imageSize);
	memset(g_imgProcess2->imageData, 0, g_imgProcess2->imageSize);
	memset(g_imgDepthRangedGray->imageData, 0, g_imgDepthRangedGray->imageSize);
	g_memStorage = cvCreateMemStorage();
//	g_memStorage_ = cvCreateMemStorage();

	cvNamedWindow("control", 1);
	cvResizeWindow("control", 640, 300);
	cvCreateTrackbar("InfraMin", "control", &g_uMinInfra, 30000, 0);
	//cvCreateTrackbar("Confidence","control",(int*)&g_uConfidence,9999,setConfidence);
	cvCreateTrackbar("RangeMin", "control", &g_uRangeMin, g_uMaxDepth, 0);
	cvCreateTrackbar("RangeMax", "control", &g_uRangeMax, g_uMaxDepth, 0);
	cvCreateTrackbar("ThMin", "control", &g_uThresholdMin, 255, 0);
	cvCreateTrackbar("ThMax", "control", &g_uThresholdMax, 255, 0);

	g_arr.clear();

	initNetwork();
	g_hThreadRecv = g_hThreadDepth = CreateThread(NULL, 0, ThreadProcRecv, 0, 0, NULL);
	return true;
}

void readSettingsFromINI()
{
	TCHAR path[512];
	GetCurrentDirectory(512, path);  //프로젝트 경로
	strcat_s(path, 512, "\\camera.ini");
	g_uFrameRate = GetPrivateProfileInt(TEXT("CameraSetting"), TEXT("FrameRate"), 30, path);
	g_uMinInfra = GetPrivateProfileInt(TEXT("CameraSetting"), TEXT("MinInfrared "), 30, path);//최소 적외선 설정

	GetCurrentDirectory(512, path);
	strcat_s(path, 512, "\\program.ini");
	g_uRangeMin = GetPrivateProfileInt(TEXT("DepthRange"), TEXT("RangeMin"), 1, path);
	g_uRangeMax = GetPrivateProfileInt(TEXT("DepthRange"), TEXT("RangeMax"), 1, path);
	if (g_uRangeMin == -1)
		g_uRangeMin = g_uMinDepth;// 잘못정의 된 깊이 거리 범위 값에 대한 예외 처리
	if (g_uRangeMax == -1)
		g_uRangeMax = g_uMaxDepth;// 잘못정의 된 깊이 거리 범위 값에 대한 예외 처리

	//g_nDepthThres = GetPrivateProfileInt(TEXT("RecognitionFactor"), TEXT("DepthThres"), 600, path);
	g_PatternMatchKorean.m_PatternRecognition.m_dDegree = GetPrivateProfileInt(TEXT("RecognitionFactor"), TEXT("Degree"), 30, path);
	g_dMinMatchRate = GetPrivateProfileInt(TEXT("RecognitionFactor"), TEXT("MinMatchRate"), 70, path);
	g_PatternMatchKorean.m_nSetLineMode = 0;
	//	g_FingerTracker.m_bVTouchMode = 1;
	g_nTailCut = GetPrivateProfileInt(TEXT("RecognitionFactor"), TEXT("TailCut"), 0, path);

	g_sPort = GetPrivateProfileInt(TEXT("IPSetting"), TEXT("Port"), -1, path);
	GetPrivateProfileString(TEXT("IPSetting"), TEXT("IP"), TEXT("127.0.0.1"), g_strIP, 256, path);

	g_sPortRecv = GetPrivateProfileInt(TEXT("RecvIPSetting"), TEXT("KIICONTROL_PORT"), -1, path);
	g_bStart = !GetPrivateProfileInt(TEXT("RecvIPSetting"), TEXT("KIICONTROL_ENABLE"), 1, path);
	//	GetPrivateProfileString( TEXT("RecvIPSetting"),TEXT("IP"), TEXT("127.0.0.1"), g_strIPRecv, 256, path );

	/*g_FingerTracker.m_rectThres.left = GetPrivateProfileInt(TEXT("Setting"), TEXT("Left"), 160, path);
	g_FingerTracker.m_rectThres.top = GetPrivateProfileInt(TEXT("Setting"), TEXT("Top"), 80, path);
	g_FingerTracker.m_rectThres.right = GetPrivateProfileInt(TEXT("Setting"), TEXT("Right"), 480, path);
	g_FingerTracker.m_rectThres.bottom = GetPrivateProfileInt(TEXT("Setting"), TEXT("Bottom"), 370, path);*/
	g_dMinMatchRate /= 100.0f;
	g_PatternMatchKorean.m_dMinScore = g_dMinMatchRate;
}

void initCamera()
{
	InitKinectV2();//kinect2 초기화
	//return InitDS(true);
}

void getDepthFrame() // 깊이 값 및 컬러값 받기(kinect2 라이브러리를 따름.)
{

	//IplImage* img = cvCreateImage(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), 8, 1);//contour을 따기 위한 이미지.

	IDepthFrame* pDepthFrame = NULL; // 깊이 프래임 초기화
	UINT nBufferSize = 0; // 버퍼 사이즈 초기화

	//g_pColorCoordinates= new ColorSpacePoint[512 * 424];

	IplImage* imgDepth = cvCreateImageHeader(cvSize(512, 424), IPL_DEPTH_16U, 1);// 깊이 이미지의 해더 정의

	HRESULT hr = g_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//최신 깊이 프래임을 저장하기
	while (FAILED(hr))// hr에서 데이터를 받는 것이 실패할 경우
		hr = g_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//최신 깊이 프래임을 저장하기
	if (SUCCEEDED(hr))//hr에서 데이터를 받는 것을 성공할 경우
	{
		hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, (UINT16**)&imgDepth->imageData);//버퍼에서 바로 접근하여 깊이 데이터를 받아옴
		cvCopy(imgDepth, g_imgDepth);//지역이미지변수를 전역 변수 이미지에 저장

		//cvResize(imgDepth, g_imgDepth, CV_INTER_CUBIC);
	}

	cvReleaseImageHeader(&imgDepth); //깊이 이미지 헤더 메모리 해제
	SafeRelease(pDepthFrame); // 깊이 프래임 안전하게 메모리 해제


	IInfraredFrame* pInfraredFrame = NULL; //적외선 프래임 초기화

	IplImage* imgInfra = cvCreateImageHeader(cvSize(512, 424), 16, 1); //적외선 ipl이미지해더 생성
	hr = g_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);//hr에 최신 적외선 프레임 값을 저장
	while (FAILED(hr))//hr에서 데이터를 받는 것을 실패할 경우
		hr = g_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);//hr에 최신 적외선 프레임 값을 저장
	if (SUCCEEDED(hr))//hr에서 데이터를 받는 것을 성공할 경우
	{
		hr = pInfraredFrame->AccessUnderlyingBuffer(&nBufferSize, (UINT16**)&imgInfra->imageData); //적외선으로 받은 데이터 g_imgInfra에 저장
		cvCopy(imgInfra, g_imgInfra);// 지역이미지변수를 전역 이미지 변수에 저장
		//cvResize(imgInfra, g_imgInfra, CV_INTER_CUBIC);
		//if(GetTickCount()>=500 && g_captureIr==false){ //0.5를 기다리고 초기에 ir이미지를 capture하지 않았으면 ir이미지 capture.
		//	cvCopy(imgInfra,g_imgIrFirst);//local 적외선 이미지를 global 적외선 이미지에 저장
		//	g_captureIr=true;// 켑쳐가 되었다는 버튼
		//}
	}

	//cvShowImage("ir First", g_imgInfra);

	cvReleaseImageHeader(&imgInfra);//다쓴 ipl 이미지는 메모리 해제
	SafeRelease(pInfraredFrame); // 안전 메모리 해제
	for (int y = 0; y<424; y++)
	{
		for (int x = 0; x<512; x++)
		{

			unsigned short val = GET2D16U(g_imgInfra, x, y); //ir 이미지에서 적외선의 밝기 값을 val에 저장		

			if (val <g_uMinInfra){ // g_uMinInfra 보다 어두운 값은 0으로 초기화-> 배경 처리
				GET2D16U(g_imgDepth, x, y) = 0;//0으로 초기화-> 배경 처리
			}

		}
	}
}

void getDepthRanged()
{
	for (int y = 0; y<424; y++)
	{
		for (int x = 0; x<512; x++)
		{
			unsigned short val = GET2D16U(g_imgDepth, x, y); //GET2D16S
			if (val && g_uRangeMin <= val &&  val <= g_uRangeMax)
			{
				GET2D8U(g_imgDepthRangedGray, x, y) = 255 - (int)((float)(val - g_uRangeMin) / (g_uRangeMax - g_uRangeMin) * 254);
			}
			else
				GET2D8U(g_imgDepthRangedGray, x, y) = 0;
		}
	}
}

void getBinary()
{
	cvThreshold(g_imgDepthPrepro, g_imgBinary, g_uThresholdMax, 255, CV_THRESH_TOZERO_INV);
	cvThreshold(g_imgBinary, g_imgBinary, g_uThresholdMin, 255, CV_THRESH_BINARY);
}

void processFrame()
{
	//IplImage* imgShowDepthRangedGray = cvCreateImage(cvSize(640, 480), 8, 1);
	IplImage* imgShowCalibration = cvCreateImage(cvSize(640, 480), 8, 3);
	//IplImage* imgShowBinary = cvCreateImage(cvSize(640, 480), 8, 1);

	IplImage* imgShowresult = cvCreateImage(cvSize(640, 480), 8, 3);

	//cvResize(g_imgDepthRangedGray, imgShowDepthRangedGray);
//	cvResize(g_imgBinary, imgShowBinary);

	
	cvMerge(g_imgBinary, g_imgBinary, g_imgBinary, NULL, imgShowresult);

	//sprintf(g_centerXtext, "center.x : %d", g_Center.x);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_centerXtext, cvPoint(190, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_centerYtext, "center.y : %d", g_Center.y);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_centerYtext, cvPoint(250, 440), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//
	//sprintf(g_indexXtext, "index.x : %d", g_index.x);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_indexXtext, cvPoint(320, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_indexYtext, "index.y : %d", g_index.y);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_indexYtext, cvPoint(390, 440), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_thumbXtext, "thumb.x : %d", g_thumbX);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_thumbXtext, cvPoint(460, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_thumbYtext, "thumb.y : %d", g_thumbY);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_thumbYtext, cvPoint(520, 440), &cvFont(1, 1), cvScalar(255, 255, 255));


	cvRectangle(imgShowresult, cvPoint(160, 80), cvPoint(480, 370), cvScalar(255, 255, 255), 3);

	//sprintf(g_clickText, "clicked: %d", g_bVTouch);// 머리 보정 값 출력
	//cvPutText(imgShowresult, g_clickText, cvPoint(0, 440), &cvFont(1, 1), cvScalar(255, 255, 255));

	//cvDrawCircle(imgShowresult, cvPoint(g_arr_result.x, g_arr_result.y), 3, CV_RGB(255, 0, 0), 3);
	
	for (int i = 0; i < g_arr.size(); i++)
	{
		cvCircle(imgShowresult, g_arr[i], 3, cvScalar(255), -1);
	}

	getCenter(g_imgBinary);



	if (g_Center.x != 0 || g_Center.y != 0)
	{
		cvDrawCircle(imgShowresult, cvPoint(g_Center.x, g_Center.y), 3, CV_RGB(0, 0, 255), 3);
		//cvDrawRect(imgShowresult, cvPoint(g_Center.x-100,g_Center.y-50), cvPoint(g_Center.x, g_Center.y+50),CV_RGB(222,222,222),3);
	}
	//Depth이미지를 FingerTracker를 통해 손끝 추출함.

	int avg_x = 0;
	int avg_y = 0;
	int num_point_index = 0;
	int num_point_thumb = 0;
	int indexY = 480;
	int indexX = 0;
	int thumbY = 0;
	int thumbX = 640;



	for (int y = 80; y < 370; y++)
	{

		for (int x = 160; x < 480; x++)
		{
			if (GET2D8U(g_imgBinary, x, y) != 0){
				if (indexY > y){
					indexY = y;
					indexX = x;
					num_point_index++;
				}

				//if (thumbX > x){
				//	thumbX = x;
				//	thumbY = y;
				//	num_point_thumb++;
				//}						//엄지 탐색 알고리즘 추가

				//if (num_point_index == 0) // ROI에서 검지가 감지 안되는 경우
				//{
				//	indexY = 0;
				//	indexX = 0;
				//}

				//if (num_point_thumb == 0) // ROI에서 엄지가 감지 안되는 경우
				//{
				//	thumbX = 0;
				//	thumbY = 0;				//엄지 탐색 알고리즘 추가
				//}
				//avg_x += x;
				//avg_y += y;
				//	num_point += 1;
			}
			/*	else
			{
			g_centerX = 0;
			g_centerY = 0;
			num_point = num_point;
			}*/
		}
	}
	//if (num_point != 0)
	//{
	/*g_centerX = g_Center.x;
	g_centerY = g_Center.y;*/
	/*g_centerX = avg_x/num_point;
	g_centerY = avg_y / num_point;*/


	if (num_point_index == 0) // ROI에서 검지가 감지 안되는 경우
	{
		indexY = 0;
		indexX = 0;
	}

	g_index.x = indexX;
	g_index.y = indexY;


	/*for (int y = (2 * g_index.y + g_Center.y) / 3;y<;y++)
	{

	}*/

	if (g_index.x)
	{
		cvDrawCircle(imgShowresult, cvPoint(g_index.x, g_index.y), 3, CV_RGB(255, 0, 0), 3);
	}

	int angle = 0; // 손의 기울기에 따른 ROI 크기 - 0 : 검지가 중앙을 가리킴, 1 : 검지가 오른쪽을 가리킴, 2 : 검지가 왼쪽을 가리킴 
	if (g_bVTouch == FALSE) // 클릭이 안됐을 때
	{
		if (g_index.x - g_Center.x > 20)
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 70), cvPoint(g_Center.x, g_Center.y + 70), CV_RGB(222, 222, 222), 3);
			angle = 1;
		}
		else if (g_index.x - g_Center.x < -30)
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 10), cvPoint(g_Center.x, g_Center.y + 70), CV_RGB(222, 222, 222), 3);
			angle = 2;
		}
		else
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 50), cvPoint(g_Center.x, g_Center.y + 70), CV_RGB(222, 222, 222), 3);
			angle = 0;
		}


		//int x_roi_range = 0;
		int y_roi_range = 0; // 중심과 엄지 검출 범위
		int x_roi_range = 0;

		switch (angle)
		{
		case 0:
			//x_roi_range = g_Center.x - 100;
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 50;
			break;
		case 1:
			//x_roi_range = g_Center.x - 100;
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 70;
			break;
		case 2:
			//x_roi_range = g_Center.x = 50;
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 10;
			break;
		default:
			break;
		}

		for (int y = y_roi_range; y < g_Center.y + 70; y++)
		{
			for (int x = g_Center.x - 100; x < x_roi_range; x++)
				//for (int y = 80; y < 370; y++)
				//{
				//	for (int x = 160; x<480; x++)
				//	//for (int x = g_Center.x; x > g_Center.x - 100; x--)
			{
				if (GET2D8U(g_imgBinary, x, y) != 0)
					if (thumbX > x)
					{
						thumbX = x;
						thumbY = y;
						num_point_thumb++;
					}

			}
		}

		if (num_point_thumb == 0)
		{
			thumbX = 0;
			thumbY = 0;
		}

		g_thumbX = thumbX;
		g_thumbY = thumbY;

		if (g_thumbX)
		{
			cvDrawCircle(imgShowresult, cvPoint(g_thumbX, g_thumbY), 3, CV_RGB(0, 255, 0), 3);
		}
	}



	if (g_bVTouch == TRUE) // 클릭이 됐을 때
	{
		if (g_index.x - g_Center.x > 20)
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 50), cvPoint(g_Center.x, g_Center.y + 40), CV_RGB(222, 222, 222), 3);
			angle = 1;
		}
		else if (g_index.x - g_Center.x < -30)
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 10), cvPoint(g_Center.x, g_Center.y +40), CV_RGB(222, 222, 222), 3);
			angle = 2;
		}
		else
		{
			cvDrawRect(imgShowresult, cvPoint(g_Center.x - 100, g_Center.y - 30), cvPoint(g_Center.x, g_Center.y + 40), CV_RGB(222, 222, 222), 3);
			angle = 0;
		}


		int y_roi_range = 0; // 중심과 엄지 검출 범위
		int x_roi_range = 0;

		switch (angle)
		{
		case 0:
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 30;
			break;
		case 1:
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 50;
			break;
		case 2:
			x_roi_range = g_Center.x;
			y_roi_range = g_Center.y - 10;
			break;
		default:
			break;
		}

		for (int y = y_roi_range; y < g_Center.y + 40; y++)
		{
			for (int x = g_Center.x - 100; x < x_roi_range; x++)

			{
				if (GET2D8U(g_imgBinary, x, y) != 0)
					if (thumbX > x)
					{
						thumbX = x;
						thumbY = y;
						num_point_thumb++;
					}

			}
		}

		if (num_point_thumb == 0)
		{
			thumbX = 0;
			thumbY = 0;
		}

		g_thumbX = thumbX;
		g_thumbY = thumbY;

		if (g_thumbX)
		{
			cvDrawCircle(imgShowresult, cvPoint(g_thumbX, g_thumbY), 3, CV_RGB(0, 255, 0), 3);
		}
	}
	//if (thumbX == g_index.x)
	//{
	//	g_thumbX = g_centerX;
	//}
	//else
	//g_thumbX = thumbX;
	//g_thumbY = thumbY;
	//
	////	}
	////else
	////{
	////	g_centerX = 0;
	////	g_centerY = 0;
	////}
	//
	//
	//g_index.x = indexX;
	//g_index.y = indexY;



	if (angle == 0)
	{
		g_dist_index = sqrt((g_index.x - g_Center.x)*(g_index.x - g_Center.x) + (g_index.y - g_Center.y)*(g_index.y - g_Center.y)) * 0.66;
		g_dist_thumb = sqrt((g_thumbX - g_Center.x)*(g_thumbX - g_Center.x) + (g_thumbY - g_Center.y)*(g_thumbY - g_Center.y));			//거리 계산 : 클릭 알고리즘에 사용


	}

	if (angle == 1)
	{
		g_dist_index = sqrt((g_index.x - g_Center.x)*(g_index.x - g_Center.x) + (g_index.y - g_Center.y)*(g_index.y - g_Center.y))*0.66;
		g_dist_thumb = sqrt((g_thumbX -g_Center.x)*(g_thumbX - g_Center.x) + (g_thumbY - g_Center.y)*(g_thumbY - g_Center.y));			//거리 계산 : 클릭 알고리즘에 사용
	}

	if (angle == 2)
	{
		//g_dist_index = sqrt((g_index.x - (int)g_Center.x)*(g_index.x - (int)g_Center.y) + (g_index.y - (int)g_Center.y)*(g_index.y - (int)g_Center.y)) *1.6;
		g_dist_index = sqrt(( g_Center.x-g_index.x)*(g_Center.x-g_index.x) + (g_Center.y-g_index.y)*( g_Center.y-g_index.y))*0.66;
		g_dist_thumb = sqrt((g_Center.x-g_thumbX)*(g_Center.x-g_thumbX) + (g_Center.y-g_thumbY)*(g_Center.y-g_thumbY));
	}

	//sprintf(g_dist_index_Text, "center x : %f center ", g_dist_index);
	//cvPutText(imgShowresult, g_dist_index_Text, cvPoint(0, 20), &cvFont(1, 1), CV_RGB(255, 255, 255));

	//sprintf(g_dist_index_Text, "center to index range rate: %f", g_dist_index);
	//cvPutText(imgShowresult, g_dist_index_Text, cvPoint(0, 20), &cvFont(1, 1), CV_RGB(255, 255, 255));
	//
	//sprintf(g_thumbdist, "center to thumb range rate: %f", g_dist_thumb);
	//cvPutText(imgShowresult, g_thumbdist, cvPoint(0, 30), &cvFont(1, 1), CV_RGB(255, 255, 255));


	cvDrawCircle(imgShowresult, g_Center, g_dist_index, CV_RGB(0, 255, 255), 3);


	if (g_dist_index > g_dist_thumb)
	{
		g_arr.push_back(g_index);// 검지 손가락 저장. 원을 이용한 클릭 알고리즘
		g_bVTouch = 1;
	}

	else if (g_dist_index <= g_dist_thumb)
		g_bVTouch = 0;

	//if (g_Center.x!=0 && g_Center.y && g_index.x!=0 && g_index.y!=0)
	//{
	//	cvCircle(g_imgBinary, g_Center, (int)g_dist_index, CV_RGB(34, 45, 56), 3);
	//}



	//cvShowImage("TestOrg", g_imgDepth);
	//cvShowImage("Test", g_imgDepthRangedGray);

	//cvShowImage("세점", imgShowresult);
	cvWaitKey(30);

	if (g_bVTouch || (0 && g_bInputStatus) && g_centerX>0)//이전 버전에서 GetStatus()호출받음
	{
		CvPointArray pArray = g_arr;
		if (pArray.size() > 0)
			SendTipPoint(pArray[pArray.size() - 1], pArray.size());
	}
	else
		SendTipPoint(cvPoint(g_index.x, g_index.y), 0);


	if (checkReady())//이전 버전에서 ChechReady()호출받음.
	{
		
		CResult res = g_PatternMatchKorean.Matching(g_arr);	// 매칭 시작
		g_PatternMatchKorean.DrawStatus(g_imgProcess2); // 여기가 문제가 있을지도...

		if (res.m_strName.size() > 1 && res.m_dScore > g_dMinMatchRate &&
			g_PatternMatchKorean.m_PatternRecognition.m_LineSegment.m_arrResult.size() > 2)	// 결과 보내기
			SendResult(res);
		else if (strstr(res.m_strName.data(), (const char*)"K27") && g_dMinMatchRate - 0.1f < res.m_dScore)	// ㅖ의 경우 인식률이 안좋아서 예외처리 함. But! ㅖ는 이제 제외됨 (ㅕ+ㅣ)
			SendResult(res);
		else
		{
			// 인식 실패
			res.m_strName.clear();
			res.m_strName.push_back('K');
			res.m_strName.push_back('0');
			res.m_strName.push_back('0');
			SendResult(res);
		}



		g_arr.clear();
	}

	cvPutText(imgShowresult, g_strFps, cvPoint(10, 20), &cvFont(1, 1), CV_RGB(255,255,255));

	//cvShowImage("depth", imgShowDepthRangedGray);
	//cvShowImage("Binary", imgShowBinary);
	cvShowImage("인식결과", g_imgProcess2);


	cvShowImage("인식화면", imgShowresult);

	//cvReleaseImage(&imgShowDepthRangedGray);
	//cvReleaseImage(&imgShowBinary);
	cvReleaseImage(&imgShowCalibration);


	cvReleaseImage(&imgShowresult);
}

void exitNetwork()
{
	if (g_bNetwork)
	{
		closesocket(g_socketRecv);
		closesocket(g_socket);
		WSACleanup();
	}
}

void exitProgram()
{
	exitCamera();
	exitNetwork();
	cvDestroyAllWindows();
	cvReleaseImage(&g_imgDepth);
	cvReleaseImage(&g_imgDepthPrepro);
	cvReleaseImage(&g_imgDepthRangedGray);
	cvReleaseImage(&g_imgColor);
	cvReleaseImage(&g_imgBinary);
	cvReleaseImage(&g_imgInfra);// g_imgInfra 메모리 해제
	cvReleaseImage(&g_IRimgBinary);//g_IRimgBinary 이진화된 메모리 해제

	cvReleaseImage(&g_imgProcess2);
	cvReleaseMemStorage(&g_memStorage);
}

void exitCamera()
{
	exitKinectV2(); // 키넥트2 끄기
}

void processKeyEvent(int key)
{
	switch (key)
	{
	case VK_ESCAPE:
		g_bRun = false;
	case '1':
		g_nTimeStart = GetTickCount();
		break;
	case '2':
		g_nTimeEnd = GetTickCount();
		printf("Tick = %lf\r\n", (double)((g_nTimeEnd - g_nTimeStart) / 1000.0f));
		break;
	default:
		break;
	}
}

void InitKinectV2() //kinect2 초기화
{
	HRESULT hr;// 결과 hr 선언

	hr = GetDefaultKinectSensor(&g_pKinectSensor); // 키넥트 센서의 기본값으로 초기화한다.


	if (g_pKinectSensor) // kinect 센서가 ready 상태인지 확인
	{
		// Initialize the Kinect and get the depth reader.
		IDepthFrameSource* pDepthFrameSource = NULL;  // 깊이 프래임을 NULL로 초기화
		IInfraredFrameSource* pInfraredFrameSource = NULL; // 적외선 프래임을 NULL로 초기화
		IColorFrameSource* pColorFrameSource = NULL; // Color 프래임 소스 NULL로 초기화

		hr = g_pKinectSensor->Open(); // kinect를 작동시킴

		if (SUCCEEDED(hr)) // kinect가 작동이 된지 확인
		{
			hr = g_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource); //깊이프레임소스를 hr에 저장
			hr = g_pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);// 적외선 소스를 hr에 저장
			hr = g_pKinectSensor->get_ColorFrameSource(&pColorFrameSource); // color 소스를 hr에 저장
			hr = g_pKinectSensor->get_CoordinateMapper(&g_pCoordinateMapper);
		}

		if (SUCCEEDED(hr))// kinect가 작동이 된지 확인
		{
			hr = pDepthFrameSource->OpenReader(&g_pDepthFrameReader); //깊이 프레임소스의 OpenReader을 hr에 저장
			hr = pInfraredFrameSource->OpenReader(&g_pInfraredFrameReader);//적외선 프레임소스의 OpenReader을 hr에 저장
			hr = pColorFrameSource->OpenReader(&g_pColorFrameReader);// color 프레임에서 OpenReader을 hr에 저장
		}

		SafeRelease(pDepthFrameSource); //깊이프레임소스를 안전하게 메모리 해제
	}

	if (!g_pKinectSensor || FAILED(hr)) //kinect 센서가 ready 상태가 아님
	{
		printf("No ready Kinect found!");
	}
}

void exitKinectV2() //kinect2 카메라 해제.
{
	// done with depth frame reader.
	SafeRelease(g_pDepthFrameReader);//깊이프레임을 읽어오는 메모리 해제
	SafeRelease(g_pInfraredFrameReader);//적외선프레임을 읽어오는 메모리 해제
	SafeRelease(g_pColorFrameReader);// color 이미지 읽어오는 메모리 해제
	SafeRelease(g_pCoordinateMapper);

	if (g_pColorCoordinates)
	{
		delete[] g_pColorCoordinates;
		g_pColorCoordinates = NULL;
	}


	// close the Kinect Sensor.
	if (g_pKinectSensor)
	{
		g_pKinectSensor->Close(); //kinect 센서 전원 끔.
	}

	SafeRelease(g_pKinectSensor); //kinet 센서 메모리 해제
}

BOOL checkReady()
{

	if (!g_bVTouch && g_arr.size() > 0)
	{
		g_arr_pre.clear();
		int nMinX = MAXINT, nMaxX = 0;
		int nMinY = MAXINT, nMaxY = 0;
		for (int i = 0; i < g_arr.size(); i++)
		{
			if (g_arr[i].x < nMinX)		nMinX =g_arr[i].x;
			if (g_arr[i].x > nMaxX)		nMaxX =g_arr[i].x;
			if (g_arr[i].y < nMinY)		nMinY =g_arr[i].y;
			if (g_arr[i].y > nMaxY)		nMaxY =g_arr[i].y;
			g_arr_pre.push_back(g_arr[i]);
		}
		if (nMaxX + nMaxY - nMinX - nMinY < 20)
		{
			g_arr.clear();
			return FALSE;
		}
		return TRUE;
	}
	else
		return FALSE;
}

void getCenter(IplImage* depthImage)
{
	IplImage* depthDist = cvCreateImage(cvSize(640, 480), IPL_DEPTH_32F, 1);
	CvPoint ptTemp;
	double dMin;
	double dMax;
	cvDistTransform(depthImage, depthDist);
	cvMinMaxLoc(depthDist, &dMin, &dMax, &ptTemp, &g_Center);
	cvReleaseImage(&depthDist);

	if (g_Center.x < 160 || g_Center.x>480 || g_Center.y < 80 || g_Center.y>370)
		g_Center = cvPoint(0, 0);
	//cvDrawCircle(g_imgShowresult,g_Center,3,CV_RGB(255,0,0),3);

}