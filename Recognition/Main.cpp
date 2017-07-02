#include <WinSock2.h>
#include "stdafx.h"
#include "Main.h"


template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease) //kinect2 �޸𸮸� �����ϰ� �����ϴ� �Լ�
{
	if (pInterfaceToRelease != NULL)
	{

		pInterfaceToRelease->Release(); // kinect2 �� interface�� release.
		pInterfaceToRelease = NULL;
	}
}

IKinectSensor*          g_pKinectSensor = 0; //kinect2 ����
IDepthFrameReader*      g_pDepthFrameReader;//���� �� �д� ���� ����
IInfraredFrameReader*	g_pInfraredFrameReader; // ���ܼ� ���� �д� ���� ����
IColorFrameReader*		g_pColorFrameReader;// color ���� ���� ���� ����

ICoordinateMapper*		g_pCoordinateMapper = 0; // mapping �ϴ� �Լ�
ColorSpacePoint*		g_pColorCoordinates = 0; // ���� ��ǥ


DWORD WINAPI ThreadProcRecv(LPVOID lParam)
{
	// KII-Control���� UDP����� �޴� �κ�. 
	// ���� ������ Recv�Լ��� ���� ������ ��ٸ��� �����̶� Thread�� ������ ������ �޵��� ����
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
		if (!strcmp(pBuf, "START"))		// START��� ���� ����
			g_bStart = 1;
		if (!strcmp(pBuf, "STOP"))		// STOP�̶�� ���� ����
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
	initPattern();		// ���� �ҷ���

	// Helpâ ����
	IplImage *pHelp = cvLoadImage("HelpKorean3.png");
	cvShowImage("Help", pHelp);

	//processFrame();
	while (g_bRun)
	{
		if (!g_bStart)	// KII-Control���� �����ϴ� �κ�
		{
			processKeyEvent(cvWaitKey(5));
			continue;
		}

		int t = GetTickCount();

		getDepthFrame();
		getDepthRanged();

		///////////depth image ����ȭ/////////////////
		IplImage* img = cvCreateImage(cvSize(512, 424), 8, 1);//contour�� ���� ���� �̹���.
		IplImage* imgMask = cvCreateImage(cvSize(512, 424), 8, 1);//contour�� ������ ����ũ�� �ӽ������ϴ� �̹���.

		//cvResize(g_imgDepthto8, g_imgDepthRangedGray, CV_INTER_CUBIC);

		// 
		cvCopy(g_imgDepthRangedGray, img);//-- ���� ū ���� �� ��ü�� ���� ������ ��� ����

		CvSeq* contours = 0;	//�ܰ���.
		cvFindContours(g_imgDepthRangedGray, g_memStorage, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//�ܰ��� ����
		CvSeq* select_contour = 0;
		float max_area = 0;

		while (contours)
		{
			float area = abs(cvContourArea(contours)); //����
			if (area>max_area)		//������ �ִ� �̹��� ũ�⺸�� �˻��� ũ�Ⱑ ũ�� �ش� �̹����� �ִ� �̹����� ����
			{
				max_area = area;   //�ִ� �̹��� ũ�� ����
				select_contour = contours;
			}

			contours = contours->h_next;
		}

		cvZero(imgMask); //imgMask�� 0���� �ʱ�ȭ
		cvDrawContours(imgMask, select_contour, cvScalar(255), cvScalar(255), 0, CV_FILLED);//imgMask�� �߶� �̹��� ����

		cvClearMemStorage(g_memStorage); //�ܰ��� ���� ������ ������ ����(������ �޸� ���� ����)

		cvZero(g_imgDepthRangedGray);
		cvCopy(img, g_imgDepthRangedGray, imgMask);//img���� imgMask�� �ش��ϴ� �κ��� imgProcess�� ����--//
		// ���⼭ ����ũ �Ҷ� ���Ƿ� ���� ���� �̹����� ����ŷ �̹����� �����ؼ� �ٽ� g_imgDepthRangedGray��
		// �ֽ��ϴ�. ����� �ٽ� g_imgDepthRangedGray�� �־����� �̴ϴ�. �׸��� �װ�
		// �������� g_imgDepthPrepro 640 480 �̹����� �ٽ� �������� �ؼ� g_imgDepthPrepro �� ���ϴ�.
		cvResize(g_imgDepthRangedGray, g_imgDepthPrepro, CV_INTER_CUBIC);

		cvReleaseImage(&img);//�̹��� ���� �޸� ����
		cvReleaseImage(&imgMask);//�̹��� ���� �޸� ����
		////////////////////////////
		//cvShowImage("16���� ��ó���� �̹���", g_imgDepthPrepro);
		//getDepthRanged();

		getBinary();
		//cvShowImage("���̳ʸ� ", g_imgBinary);
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
	// UDP�� ���� KII-GUI�� ����� ������ (K01, K02 ��)
	if (!g_bNetwork)		// ��Ʈ��ũ ���� �Լ��� �ʱ�ȭ ���� ���� ������ ����.
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
	// UDP����� �̿��� �ճ� ���� ���� (���� ����: P01X02Y03)
	if (!g_bNetwork)
		return;

	// �ճ� ��ġ�� 0~99 ������ �� ������.
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
	// ������� ����.
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

	for (int i = 0; i < 3; i++)			// �̸�
		pBuf[i] = res.m_strName[i];

	int nMinX = 640, nMinY = 480, nMaxX = 0, nMaxY = 0; // process ũ����� 480
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
	// ������ �ҷ����� �Լ�
	TCHAR path[512];
	GetCurrentDirectory(512, path);  //������Ʈ ���
	char strPath[1024] = { 0 };
	for (int i = 1; i < 34; i++)	// �ѱ�Ű �ε�
	{
		if (i == 2 || i == 5 || i == 9 || i == 11 || i == 14 || i == 21 || i == 23 || i == 25 || i == 27)
			continue;
		sprintf_s(strPath, 1024, "%s\\Pattern\\Korean\\K%02d.xml", path, i);
		string str = strPath;
		g_PatternMatchKorean.AddPattern(str);
		str.clear();
	}
	for (int i = 1; i < 4; i++)	// ����Ű �ε�
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
	// ��Ʈ��ũ �ʱ�ȭ �Լ�
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

	g_imgDepthRangedGray = cvCreateImage(cvSize(512, 424), 8, 1); // ������ ����� �޾Ƽ� ������.
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
	GetCurrentDirectory(512, path);  //������Ʈ ���
	strcat_s(path, 512, "\\camera.ini");
	g_uFrameRate = GetPrivateProfileInt(TEXT("CameraSetting"), TEXT("FrameRate"), 30, path);
	g_uMinInfra = GetPrivateProfileInt(TEXT("CameraSetting"), TEXT("MinInfrared "), 30, path);//�ּ� ���ܼ� ����

	GetCurrentDirectory(512, path);
	strcat_s(path, 512, "\\program.ini");
	g_uRangeMin = GetPrivateProfileInt(TEXT("DepthRange"), TEXT("RangeMin"), 1, path);
	g_uRangeMax = GetPrivateProfileInt(TEXT("DepthRange"), TEXT("RangeMax"), 1, path);
	if (g_uRangeMin == -1)
		g_uRangeMin = g_uMinDepth;// �߸����� �� ���� �Ÿ� ���� ���� ���� ���� ó��
	if (g_uRangeMax == -1)
		g_uRangeMax = g_uMaxDepth;// �߸����� �� ���� �Ÿ� ���� ���� ���� ���� ó��

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
	InitKinectV2();//kinect2 �ʱ�ȭ
	//return InitDS(true);
}

void getDepthFrame() // ���� �� �� �÷��� �ޱ�(kinect2 ���̺귯���� ����.)
{

	//IplImage* img = cvCreateImage(cvSize(DEPTH_WIDTH, DEPTH_HEIGHT), 8, 1);//contour�� ���� ���� �̹���.

	IDepthFrame* pDepthFrame = NULL; // ���� ������ �ʱ�ȭ
	UINT nBufferSize = 0; // ���� ������ �ʱ�ȭ

	//g_pColorCoordinates= new ColorSpacePoint[512 * 424];

	IplImage* imgDepth = cvCreateImageHeader(cvSize(512, 424), IPL_DEPTH_16U, 1);// ���� �̹����� �ش� ����

	HRESULT hr = g_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//�ֽ� ���� �������� �����ϱ�
	while (FAILED(hr))// hr���� �����͸� �޴� ���� ������ ���
		hr = g_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);//�ֽ� ���� �������� �����ϱ�
	if (SUCCEEDED(hr))//hr���� �����͸� �޴� ���� ������ ���
	{
		hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, (UINT16**)&imgDepth->imageData);//���ۿ��� �ٷ� �����Ͽ� ���� �����͸� �޾ƿ�
		cvCopy(imgDepth, g_imgDepth);//�����̹��������� ���� ���� �̹����� ����

		//cvResize(imgDepth, g_imgDepth, CV_INTER_CUBIC);
	}

	cvReleaseImageHeader(&imgDepth); //���� �̹��� ��� �޸� ����
	SafeRelease(pDepthFrame); // ���� ������ �����ϰ� �޸� ����


	IInfraredFrame* pInfraredFrame = NULL; //���ܼ� ������ �ʱ�ȭ

	IplImage* imgInfra = cvCreateImageHeader(cvSize(512, 424), 16, 1); //���ܼ� ipl�̹����ش� ����
	hr = g_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);//hr�� �ֽ� ���ܼ� ������ ���� ����
	while (FAILED(hr))//hr���� �����͸� �޴� ���� ������ ���
		hr = g_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);//hr�� �ֽ� ���ܼ� ������ ���� ����
	if (SUCCEEDED(hr))//hr���� �����͸� �޴� ���� ������ ���
	{
		hr = pInfraredFrame->AccessUnderlyingBuffer(&nBufferSize, (UINT16**)&imgInfra->imageData); //���ܼ����� ���� ������ g_imgInfra�� ����
		cvCopy(imgInfra, g_imgInfra);// �����̹��������� ���� �̹��� ������ ����
		//cvResize(imgInfra, g_imgInfra, CV_INTER_CUBIC);
		//if(GetTickCount()>=500 && g_captureIr==false){ //0.5�� ��ٸ��� �ʱ⿡ ir�̹����� capture���� �ʾ����� ir�̹��� capture.
		//	cvCopy(imgInfra,g_imgIrFirst);//local ���ܼ� �̹����� global ���ܼ� �̹����� ����
		//	g_captureIr=true;// ���İ� �Ǿ��ٴ� ��ư
		//}
	}

	//cvShowImage("ir First", g_imgInfra);

	cvReleaseImageHeader(&imgInfra);//�پ� ipl �̹����� �޸� ����
	SafeRelease(pInfraredFrame); // ���� �޸� ����
	for (int y = 0; y<424; y++)
	{
		for (int x = 0; x<512; x++)
		{

			unsigned short val = GET2D16U(g_imgInfra, x, y); //ir �̹������� ���ܼ��� ��� ���� val�� ����		

			if (val <g_uMinInfra){ // g_uMinInfra ���� ��ο� ���� 0���� �ʱ�ȭ-> ��� ó��
				GET2D16U(g_imgDepth, x, y) = 0;//0���� �ʱ�ȭ-> ��� ó��
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

	//sprintf(g_centerXtext, "center.x : %d", g_Center.x);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_centerXtext, cvPoint(190, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_centerYtext, "center.y : %d", g_Center.y);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_centerYtext, cvPoint(250, 440), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//
	//sprintf(g_indexXtext, "index.x : %d", g_index.x);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_indexXtext, cvPoint(320, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_indexYtext, "index.y : %d", g_index.y);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_indexYtext, cvPoint(390, 440), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_thumbXtext, "thumb.x : %d", g_thumbX);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_thumbXtext, cvPoint(460, 400), &cvFont(1, 1), cvScalar(255, 255, 255));
	//
	//sprintf(g_thumbYtext, "thumb.y : %d", g_thumbY);// �Ӹ� ���� �� ���
	//cvPutText(imgShowresult, g_thumbYtext, cvPoint(520, 440), &cvFont(1, 1), cvScalar(255, 255, 255));


	cvRectangle(imgShowresult, cvPoint(160, 80), cvPoint(480, 370), cvScalar(255, 255, 255), 3);

	//sprintf(g_clickText, "clicked: %d", g_bVTouch);// �Ӹ� ���� �� ���
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
	//Depth�̹����� FingerTracker�� ���� �ճ� ������.

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
				//}						//���� Ž�� �˰��� �߰�

				//if (num_point_index == 0) // ROI���� ������ ���� �ȵǴ� ���
				//{
				//	indexY = 0;
				//	indexX = 0;
				//}

				//if (num_point_thumb == 0) // ROI���� ������ ���� �ȵǴ� ���
				//{
				//	thumbX = 0;
				//	thumbY = 0;				//���� Ž�� �˰��� �߰�
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


	if (num_point_index == 0) // ROI���� ������ ���� �ȵǴ� ���
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

	int angle = 0; // ���� ���⿡ ���� ROI ũ�� - 0 : ������ �߾��� ����Ŵ, 1 : ������ �������� ����Ŵ, 2 : ������ ������ ����Ŵ 
	if (g_bVTouch == FALSE) // Ŭ���� �ȵ��� ��
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
		int y_roi_range = 0; // �߽ɰ� ���� ���� ����
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



	if (g_bVTouch == TRUE) // Ŭ���� ���� ��
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


		int y_roi_range = 0; // �߽ɰ� ���� ���� ����
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
		g_dist_thumb = sqrt((g_thumbX - g_Center.x)*(g_thumbX - g_Center.x) + (g_thumbY - g_Center.y)*(g_thumbY - g_Center.y));			//�Ÿ� ��� : Ŭ�� �˰��� ���


	}

	if (angle == 1)
	{
		g_dist_index = sqrt((g_index.x - g_Center.x)*(g_index.x - g_Center.x) + (g_index.y - g_Center.y)*(g_index.y - g_Center.y))*0.66;
		g_dist_thumb = sqrt((g_thumbX -g_Center.x)*(g_thumbX - g_Center.x) + (g_thumbY - g_Center.y)*(g_thumbY - g_Center.y));			//�Ÿ� ��� : Ŭ�� �˰��� ���
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
		g_arr.push_back(g_index);// ���� �հ��� ����. ���� �̿��� Ŭ�� �˰���
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

	//cvShowImage("����", imgShowresult);
	cvWaitKey(30);

	if (g_bVTouch || (0 && g_bInputStatus) && g_centerX>0)//���� �������� GetStatus()ȣ�����
	{
		CvPointArray pArray = g_arr;
		if (pArray.size() > 0)
			SendTipPoint(pArray[pArray.size() - 1], pArray.size());
	}
	else
		SendTipPoint(cvPoint(g_index.x, g_index.y), 0);


	if (checkReady())//���� �������� ChechReady()ȣ�����.
	{
		
		CResult res = g_PatternMatchKorean.Matching(g_arr);	// ��Ī ����
		g_PatternMatchKorean.DrawStatus(g_imgProcess2); // ���Ⱑ ������ ��������...

		if (res.m_strName.size() > 1 && res.m_dScore > g_dMinMatchRate &&
			g_PatternMatchKorean.m_PatternRecognition.m_LineSegment.m_arrResult.size() > 2)	// ��� ������
			SendResult(res);
		else if (strstr(res.m_strName.data(), (const char*)"K27") && g_dMinMatchRate - 0.1f < res.m_dScore)	// ���� ��� �νķ��� �����Ƽ� ����ó�� ��. But! �ƴ� ���� ���ܵ� (��+��)
			SendResult(res);
		else
		{
			// �ν� ����
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
	cvShowImage("�νİ��", g_imgProcess2);


	cvShowImage("�ν�ȭ��", imgShowresult);

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
	cvReleaseImage(&g_imgInfra);// g_imgInfra �޸� ����
	cvReleaseImage(&g_IRimgBinary);//g_IRimgBinary ����ȭ�� �޸� ����

	cvReleaseImage(&g_imgProcess2);
	cvReleaseMemStorage(&g_memStorage);
}

void exitCamera()
{
	exitKinectV2(); // Ű��Ʈ2 ����
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

void InitKinectV2() //kinect2 �ʱ�ȭ
{
	HRESULT hr;// ��� hr ����

	hr = GetDefaultKinectSensor(&g_pKinectSensor); // Ű��Ʈ ������ �⺻������ �ʱ�ȭ�Ѵ�.


	if (g_pKinectSensor) // kinect ������ ready �������� Ȯ��
	{
		// Initialize the Kinect and get the depth reader.
		IDepthFrameSource* pDepthFrameSource = NULL;  // ���� �������� NULL�� �ʱ�ȭ
		IInfraredFrameSource* pInfraredFrameSource = NULL; // ���ܼ� �������� NULL�� �ʱ�ȭ
		IColorFrameSource* pColorFrameSource = NULL; // Color ������ �ҽ� NULL�� �ʱ�ȭ

		hr = g_pKinectSensor->Open(); // kinect�� �۵���Ŵ

		if (SUCCEEDED(hr)) // kinect�� �۵��� ���� Ȯ��
		{
			hr = g_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource); //���������Ӽҽ��� hr�� ����
			hr = g_pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);// ���ܼ� �ҽ��� hr�� ����
			hr = g_pKinectSensor->get_ColorFrameSource(&pColorFrameSource); // color �ҽ��� hr�� ����
			hr = g_pKinectSensor->get_CoordinateMapper(&g_pCoordinateMapper);
		}

		if (SUCCEEDED(hr))// kinect�� �۵��� ���� Ȯ��
		{
			hr = pDepthFrameSource->OpenReader(&g_pDepthFrameReader); //���� �����Ӽҽ��� OpenReader�� hr�� ����
			hr = pInfraredFrameSource->OpenReader(&g_pInfraredFrameReader);//���ܼ� �����Ӽҽ��� OpenReader�� hr�� ����
			hr = pColorFrameSource->OpenReader(&g_pColorFrameReader);// color �����ӿ��� OpenReader�� hr�� ����
		}

		SafeRelease(pDepthFrameSource); //���������Ӽҽ��� �����ϰ� �޸� ����
	}

	if (!g_pKinectSensor || FAILED(hr)) //kinect ������ ready ���°� �ƴ�
	{
		printf("No ready Kinect found!");
	}
}

void exitKinectV2() //kinect2 ī�޶� ����.
{
	// done with depth frame reader.
	SafeRelease(g_pDepthFrameReader);//������������ �о���� �޸� ����
	SafeRelease(g_pInfraredFrameReader);//���ܼ��������� �о���� �޸� ����
	SafeRelease(g_pColorFrameReader);// color �̹��� �о���� �޸� ����
	SafeRelease(g_pCoordinateMapper);

	if (g_pColorCoordinates)
	{
		delete[] g_pColorCoordinates;
		g_pColorCoordinates = NULL;
	}


	// close the Kinect Sensor.
	if (g_pKinectSensor)
	{
		g_pKinectSensor->Close(); //kinect ���� ���� ��.
	}

	SafeRelease(g_pKinectSensor); //kinet ���� �޸� ����
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