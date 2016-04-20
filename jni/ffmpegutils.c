/*
 * Copyright 2011 - Churn Labs, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is mostly based off of the FFMPEG tutorial:
 * http://dranger.com/ffmpeg/
 * With a few updates to support Android output mechanisms and to update
 * places where the APIs have shifted.
 */

#include <errno.h>
#include <sys/types.h>	
#include <sys/stat.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <linux/fb.h>
#include <jni.h>
#include <syslog.h>
#include <android/log.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>


#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define WMINFO_LENGTH 35
#define WMINFO_WIDTH  40
#define WMINFO_SEED   10

typedef struct _GPSDATA GPSDATA;






struct _GPSDATA
{
	struct tm time;//UTC时间
	double  latitude;//纬度
	char    lat_hemi;//南北半球
	double  longitude;//经度
	char    lon_hemi;//东西半球
	int     status; //GPS状态， 0初始化， 1单点定位， 2码差分， 3无效PPS， 4固定解， 5浮点解， 6正在估算 7，人工输入固定值， 8模拟模式， 9WAAS查分
	int     sum;//使用卫星数
	double  hdop;//水印精度因子
	double  geoHeigth;//地理高度
	char    geoHeightUnit;//地理高度单位

	double  HeigthDif;//海平面与椭球面高度差
	char    HeightDifUnit;//海平面与椭球面高度差单位
};

/***********************************************************************
* 函数名称：
* EmbeMark()
*函数参数：
*  unsigned char *pIamgeBuf  -图片数据块
*  char* pGps        -GPS数据串（GPGGA格式）
*  int nIamgeHeight  -图片高度
*  int nIamgeWidth   -图片宽度
*返回值：
*   false为失败,true为成功
*说明：给定一个图像数据库，及其宽、高,将GPGGA格式的GPS串嵌入到里面
***********************************************************************/
char* EmbeMark(unsigned char* pIamgeBuf,int nIamgeHeight, int nIamgeWidth,int* time,double* gps);

/***********************************************************************
* 函数名称：
* WriteWmInfo()
*函数参数：
*  char* fileName  -文件名
*  char* pWminfo   -水印信息
*  double sampleTime -时间戳
*返回值：
*   void
*说明：将当前帧嵌入的GPS信息，记录到文本文件中
***********************************************************************/



void GetWmInfoFromGPGGA(int* Date, double* pGps, char* pWmInfo);
int** CreateMeaninglessWMInfor(char* WMInfo);
void ReleaseWMInfo(int** WMInfo);
int* FetchBpointFromImage(unsigned char* pBuf,int nIamgeHeight, int nIamgeWidth);
void ReturnBpointToImage(unsigned char* pBuf,int *pBpiontArry,int nIamgeHeight, int nIamgeWidth);
void ReleaseBpointArry(int* pBpointArry);
void EmbedDCT24(int** WMInfo,int* pBpointArry,int nIamgeHeight,int nIamgeWidth);


int markflag=7;
int marks=6;

////////////////////////DCT
#define USE_ACCURATE_ROUNDING

#define RIGHT_SHIFT(x, shft)  ((x) >> (shft))

#ifdef USE_ACCURATE_ROUNDING
#define ONE ((int) 1)
#define DESCALE(x, n)  RIGHT_SHIFT((x) + (ONE << ((n) - 1)), n)
#else
#define DESCALE(x, n)  RIGHT_SHIFT(x, n)
#endif

#define CONST_BITS  13
#define PASS1_BITS  2

#define FIX_0_298631336  ((int)  2446)	/* FIX(0.298631336) */
#define FIX_0_390180644  ((int)  3196)	/* FIX(0.390180644) */
#define FIX_0_541196100  ((int)  4433)	/* FIX(0.541196100) */
#define FIX_0_765366865  ((int)  6270)	/* FIX(0.765366865) */
#define FIX_0_899976223  ((int)  7373)	/* FIX(0.899976223) */
#define FIX_1_175875602  ((int)  9633)	/* FIX(1.175875602) */
#define FIX_1_501321110  ((int) 12299)	/* FIX(1.501321110) */
#define FIX_1_847759065  ((int) 15137)	/* FIX(1.847759065) */
#define FIX_1_961570560  ((int) 16069)	/* FIX(1.961570560) */
#define FIX_2_053119869  ((int) 16819)	/* FIX(2.053119869) */
#define FIX_2_562915447  ((int) 20995)	/* FIX(2.562915447) */
#define FIX_3_072711026  ((int) 25172)	/* FIX(3.072711026) */

#define W1 2841					/* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676					/* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408					/* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609					/* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108					/* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565					/* 2048*sqrt(2)*cos(7*pi/16) */

/* function pointer */

/*
 * Perform an integer forward DCT on one block of samples.
 */
void fdct_int32(short *const block);
void idct_int32(short *const block);
static  inline  int iclp_char( int x );



#define  LOG_TAG    "FfmpegUtils"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)

struct fimc_buffer {
    unsigned char *start;
    size_t  length;
};

static int fd = -1;
struct fimc_buffer *buffers=NULL;
struct v4l2_buffer v4l2_buf;
static int bufnum = 1;
static int mwidth,mheight;

char* wmInfo[WMINFO_LENGTH] = {0};

char* EmbeMark(unsigned char* pIamgeBuf,int nIamgeHeight, int nIamgeWidth,int* time,double* gps)
{
	GetWmInfoFromGPGGA(time, gps, wmInfo);
	int** WMInfoArry = CreateMeaninglessWMInfor(wmInfo);
	int* pBArry = FetchBpointFromImage(pIamgeBuf,nIamgeHeight,nIamgeWidth);
	EmbedDCT24(WMInfoArry,pBArry,nIamgeHeight,nIamgeWidth);
	ReturnBpointToImage(pIamgeBuf,pBArry,nIamgeHeight,nIamgeWidth);
	ReleaseBpointArry(pBArry);
	ReleaseWMInfo(WMInfoArry);
	return wmInfo;
}

// 将GPSDATA数据写入pWmInfo
void GetWmInfoFromGPGGA(int* Date, double* pGps, char* pWmInfo)
{
	//时间
	char* p = pWmInfo;

	char year[20];
	sprintf(year, "%02d", Date[0]);

	strncpy(p,year,2);
 	p += 2;

	char month[20];
	sprintf(month, "%02d", Date[1]);
	strncpy(p,month,2);
	p += 2;

	char day[20];
	sprintf(day, "%02d", Date[2]);
	strncpy(p,day,2);
	p += 2;

	char hour[20];
	sprintf(hour, "%02d", Date[3]);
	strncpy(p,hour,2);
	p += 2;

	char minute[20];
	sprintf(minute, "%02d", Date[4]);
	strncpy(p,minute,2);
	p += 2;

	char second[20];
	sprintf(second, "%02d", Date[5]);
	strncpy(p,second,2);
	p += 2;

	//经度
	char longitude[20];
	sprintf(longitude,"%010.6f", pGps[0]);
	strncpy(p,longitude,3);
	p += 3;
	strncpy(p,longitude + 4,6);
	p += 6;

	//纬度
	char latitude[20];
	sprintf(latitude,"%09.6f", pGps[1]);

	strncpy(p,latitude,2);
	p += 2;
	strncpy(p,latitude + 3,6);
	p += 6;

	//高程
	if (pGps[2] >= 0)
	{
		strncpy(p++,"0",1);
	}
	else
	{
		strncpy(p++,"1",1);
	}

	char elevation[20];
	sprintf(elevation,"%06.1f",fabs(pGps[2]));

	strncpy(p,elevation,4);
	p += 4;
	strncpy(p++,elevation + 5,1);

	*p = '\0';
}


int** CreateMeaninglessWMInfor(char* WMInfo)
{
	int ** WMInfoArray = NULL;

	int a0[]={0,0,1,1,1,0,0,1,1,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,0,1,1};
	int a1[]={0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,0,1,1,1,0,0,1,0,0,0,0,1,0,1,0,0,1,1,0,1,1,0,0,1,0};
	int a2[]={1,0,1,0,0,1,1,0,0,1,0,0,1,0,1,1,0,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,0,1,1,0,0,1,1,0};
	int a3[]={0,0,0,0,1,0,1,0,1,1,0,1,1,0,1,0,0,0,0,1,0,1,0,1,0,0,1,0,1,0,1,0,0,0,0,1,0,0,1,1};
	int a4[]={1,1,1,0,1,0,1,0,1,1,0,0,0,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,1,0,0,0,0};
	int a5[]={0,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,0,0,1,1,0,0,0,1,0,1,0,1,1};
	int a6[]={1,0,1,0,0,0,0,1,0,1,0,1,1,0,0,0,0,1,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,1,1,1,0,0};
	int a7[]={1,0,0,0,1,0,0,1,0,0,0,0,1,1,0,0,1,1,1,1,1,1,1,1,0,1,1,0,1,1,0,0,0,0,1,0,0,0,0,0};
	int a8[]={0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,1,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,1,0,0};
	int a9[]={0,0,0,1,0,1,1,1,1,1,0,0,1,1,0,1,0,1,1,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,1,0};

    int i,j;
	if (WMInfoArray == NULL)
	{
		WMInfoArray = (int**)malloc(WMINFO_LENGTH * sizeof(int));
		for (j = 0;j < WMINFO_LENGTH;j++)
		{
			WMInfoArray[j] = (int*)malloc(WMINFO_WIDTH * sizeof(int));
	//		memset(WMInfoArray[j],0,WMINFO_WIDTH * sizeof(int));
		}
	}

	for(i = 0; i < WMINFO_LENGTH; i++)
	{
		switch(WMInfo[i])
		{
		case '0':
			memcpy(WMInfoArray[i],a0,WMINFO_WIDTH*sizeof(int));
			break;
		case '1':
			memcpy(WMInfoArray[i],a1,WMINFO_WIDTH*sizeof(int));
			break;
		case '2':
			memcpy(WMInfoArray[i],a2,WMINFO_WIDTH*sizeof(int));
			break;
		case '3':
			memcpy(WMInfoArray[i],a3,WMINFO_WIDTH*sizeof(int));
			break;
		case '4':
			memcpy(WMInfoArray[i],a4,WMINFO_WIDTH*sizeof(int));
			break;
		case '5':
			memcpy(WMInfoArray[i],a5,WMINFO_WIDTH*sizeof(int));
			break;
		case '6':
			memcpy(WMInfoArray[i],a6,WMINFO_WIDTH*sizeof(int));
			break;
		case '7':
			memcpy(WMInfoArray[i],a7,WMINFO_WIDTH*sizeof(int));
			break;
		case '8':
			memcpy(WMInfoArray[i],a8,WMINFO_WIDTH*sizeof(int));
			break;
		case '9':
			memcpy(WMInfoArray[i],a9,WMINFO_WIDTH*sizeof(int));
			break;
		}
	}

	return WMInfoArray;
}

void ReleaseWMInfo(int** WMInfo)
{
	if (WMInfo != NULL)
	{   int i;
		for ( i = 0;i<WMINFO_LENGTH;i++)
		{
			free(WMInfo[i]);
		}
		free(WMInfo);
		WMInfo = NULL;
	}
}

int* FetchBpointFromImage(unsigned char* pBuf,int nIamgeHeight, int nIamgeWidth)
{
	int lenth = nIamgeHeight*nIamgeWidth*sizeof(int);
	int *pBpiontArry = (int* )malloc(lenth);

    int i,j;
	for (i=0;i<nIamgeHeight;i++)
	{
		for(j=0; j<nIamgeWidth; j++)
		{
			int b = pBuf[3*i*nIamgeWidth+3*j+0];

			pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j] = b;
		}
	}

	return pBpiontArry;
}

void ReturnBpointToImage(unsigned char* pBuf,int *pBpiontArry,int nIamgeHeight, int nIamgeWidth)
{
    int i,j;
	for( i=0; i<nIamgeHeight; i++)
	{
		for( j=0; j<nIamgeWidth; j++)
		{
			if (pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j] < 0)
				pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j] = 0;
			if (pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j] > 255)
				pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j] = 255;

			pBuf[3*i*nIamgeWidth+3*j+0] = pBpiontArry[(nIamgeHeight - i - 1)*nIamgeWidth+j];
		}
	}
}

void ReleaseBpointArry(int* pBpointArry)
{
	if (pBpointArry!=NULL)
	{
		free(pBpointArry);
		pBpointArry=NULL;
	}
}

void EmbedDCT24(int** WMInfo,int* pBpointArry,int nIamgeHeight,int nIamgeWidth)
{
	//-----------------------
	int nDelta = 40;
	int mDctBlock = 8;
	int nXBlock = mDctBlock;
	int mWMNum = 10;
	int nMaxXBlock = (nIamgeHeight * nIamgeWidth) / (mWMNum * WMINFO_WIDTH*WMINFO_LENGTH * mDctBlock);
	if (nMaxXBlock > nXBlock)
		nXBlock = nMaxXBlock;
	//-----------------------
	// 将图像分成8*8块，进行离散余弦变换
	//	CTransform CTran;
	int m = 0;
	int n = 0;
	int jOri = 0;

	short Data88[64];
	int i, j, bi, bj;
	int akp, WMInforBit;

	for (i = 0; i < nIamgeHeight-mDctBlock; i+=mDctBlock)
	{
		for (j = jOri; j < nIamgeWidth-nXBlock; j+=nXBlock)
		{
			for (bi = 0; bi < mDctBlock; bi++)
			{
				for (bj = 0; bj < mDctBlock; bj++)
				{
					Data88[bi*8+bj] = pBpointArry[(i+bi)*nIamgeWidth+(j+bj)];
				}
			}
			//			CTran.fdct_2D(Data88, CTran.GetTwoIndex(8), CTran.GetTwoIndex(8));
			fdct_int32(Data88);
			//----------------
			akp = (int)(Data88[0] / nDelta + 0.5);
			WMInforBit = WMInfo[m][n];
			n++;
			if (m == WMINFO_LENGTH-1 && n == WMINFO_WIDTH-1)
			{
				m = 0;
				n = 0;
			}
			if (n == WMINFO_WIDTH-1)
			{
				m++;
				n = 0;
			}
			if ((akp+WMInforBit) & 1 == 1)
				Data88[0] = ((double)akp - 0.5) * nDelta;
			else
				Data88[0] = ((double)akp + 0.5) * nDelta;
			//----------------
			//			CTran.fidct_2D(Data88, CTran.GetTwoIndex(8), CTran.GetTwoIndex(8));
			idct_int32(Data88);
			for (bi = 0; bi < mDctBlock; bi++)
			{
				for ( bj = 0; bj < mDctBlock; bj++)
				{
					pBpointArry[(i+bi)*nIamgeWidth+(j+bj)] = Data88[bi*mDctBlock+bj];
				}
			}
			//----------------
			/*	delete[] Data88; Data88 = NULL;*/
		}
	}
}


void fdct_int32(short *const block)
{
	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z1, z2, z3, z4, z5;
	short *blkptr;
	int *dataptr;
	int data[64];
	int i;

	/* Pass 1: process rows. */
	/* Note results are scaled up by sqrt(8) compared to a true DCT; */
	/* furthermore, we scale the results by 2**PASS1_BITS. */

	dataptr = data;
	blkptr = block;
	for (i = 0; i < 8; i++) {
		tmp0 = blkptr[0] + blkptr[7];
		tmp7 = blkptr[0] - blkptr[7];
		tmp1 = blkptr[1] + blkptr[6];
		tmp6 = blkptr[1] - blkptr[6];
		tmp2 = blkptr[2] + blkptr[5];
		tmp5 = blkptr[2] - blkptr[5];
		tmp3 = blkptr[3] + blkptr[4];
		tmp4 = blkptr[3] - blkptr[4];

		/* Even part per LL&M figure 1 --- note that published figure is faulty;
		 * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
		 */

		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = (tmp10 + tmp11) << PASS1_BITS;
		dataptr[4] = (tmp10 - tmp11) << PASS1_BITS;

		z1 = (tmp12 + tmp13) * FIX_0_541196100;
		dataptr[2] =
			DESCALE(z1 + tmp13 * FIX_0_765366865, CONST_BITS - PASS1_BITS);
		dataptr[6] =
			DESCALE(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS - PASS1_BITS);

		/* Odd part per figure 8 --- note paper omits factor of sqrt(2).
		 * cK represents cos(K*pi/16).
		 * i0..i3 in the paper are tmp4..tmp7 here.
		 */

		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = (z3 + z4) * FIX_1_175875602;	/* sqrt(2) * c3 */

		tmp4 *= FIX_0_298631336;	/* sqrt(2) * (-c1+c3+c5-c7) */
		tmp5 *= FIX_2_053119869;	/* sqrt(2) * ( c1+c3-c5+c7) */
		tmp6 *= FIX_3_072711026;	/* sqrt(2) * ( c1+c3+c5-c7) */
		tmp7 *= FIX_1_501321110;	/* sqrt(2) * ( c1+c3-c5-c7) */
		z1 *= -FIX_0_899976223;	/* sqrt(2) * (c7-c3) */
		z2 *= -FIX_2_562915447;	/* sqrt(2) * (-c1-c3) */
		z3 *= -FIX_1_961570560;	/* sqrt(2) * (-c3-c5) */
		z4 *= -FIX_0_390180644;	/* sqrt(2) * (c5-c3) */

		z3 += z5;
		z4 += z5;

		dataptr[7] = DESCALE(tmp4 + z1 + z3, CONST_BITS - PASS1_BITS);
		dataptr[5] = DESCALE(tmp5 + z2 + z4, CONST_BITS - PASS1_BITS);
		dataptr[3] = DESCALE(tmp6 + z2 + z3, CONST_BITS - PASS1_BITS);
		dataptr[1] = DESCALE(tmp7 + z1 + z4, CONST_BITS - PASS1_BITS);

		dataptr += 8;			/* advance pointer to next row */
		blkptr += 8;
	}

	/* Pass 2: process columns.
	 * We remove the PASS1_BITS scaling, but leave the results scaled up
	 * by an overall factor of 8.
	 */

	dataptr = data;
	for (i = 0; i < 8; i++) {
		tmp0 = dataptr[0] + dataptr[56];
		tmp7 = dataptr[0] - dataptr[56];
		tmp1 = dataptr[8] + dataptr[48];
		tmp6 = dataptr[8] - dataptr[48];
		tmp2 = dataptr[16] + dataptr[40];
		tmp5 = dataptr[16] - dataptr[40];
		tmp3 = dataptr[24] + dataptr[32];
		tmp4 = dataptr[24] - dataptr[32];

		/* Even part per LL&M figure 1 --- note that published figure is faulty;
		 * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
		 */

		tmp10 = tmp0 + tmp3;
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;

		dataptr[0] = DESCALE(tmp10 + tmp11, PASS1_BITS);
		dataptr[32] = DESCALE(tmp10 - tmp11, PASS1_BITS);

		z1 = (tmp12 + tmp13) * FIX_0_541196100;
		dataptr[16] =
			DESCALE(z1 + tmp13 * FIX_0_765366865, CONST_BITS + PASS1_BITS);
		dataptr[48] =
			DESCALE(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS + PASS1_BITS);

		/* Odd part per figure 8 --- note paper omits factor of sqrt(2).
		 * cK represents cos(K*pi/16).
		 * i0..i3 in the paper are tmp4..tmp7 here.
		 */

		z1 = tmp4 + tmp7;
		z2 = tmp5 + tmp6;
		z3 = tmp4 + tmp6;
		z4 = tmp5 + tmp7;
		z5 = (z3 + z4) * FIX_1_175875602;	/* sqrt(2) * c3 */

		tmp4 *= FIX_0_298631336;	/* sqrt(2) * (-c1+c3+c5-c7) */
		tmp5 *= FIX_2_053119869;	/* sqrt(2) * ( c1+c3-c5+c7) */
		tmp6 *= FIX_3_072711026;	/* sqrt(2) * ( c1+c3+c5-c7) */
		tmp7 *= FIX_1_501321110;	/* sqrt(2) * ( c1+c3-c5-c7) */
		z1 *= -FIX_0_899976223;	/* sqrt(2) * (c7-c3) */
		z2 *= -FIX_2_562915447;	/* sqrt(2) * (-c1-c3) */
		z3 *= -FIX_1_961570560;	/* sqrt(2) * (-c3-c5) */
		z4 *= -FIX_0_390180644;	/* sqrt(2) * (c5-c3) */

		z3 += z5;
		z4 += z5;

		dataptr[56] = DESCALE(tmp4 + z1 + z3, CONST_BITS + PASS1_BITS);
		dataptr[40] = DESCALE(tmp5 + z2 + z4, CONST_BITS + PASS1_BITS);
		dataptr[24] = DESCALE(tmp6 + z2 + z3, CONST_BITS + PASS1_BITS);
		dataptr[8] = DESCALE(tmp7 + z1 + z4, CONST_BITS + PASS1_BITS);

		dataptr++;				/* advance pointer to next column */
	}
	/* descale */
	for (i = 0; i < 64; i++)
		block[i] = (short int) DESCALE(data[i], 3);
}

/* private data
 * Initialized by idct_int32_init so it's mostly RO data,
 * doesn't hurt thread safety */

static  inline  int iclp_char( int x )
{
    return ( (x & ~255) ? (-x)>>31 & 255 : x );
}

void idct_int32(short *const block)
{

	/*
	 * idct_int32_init() must be called before the first call to this
	 * function!
	 */


	short *blk;
	int i;
	long X0, X1, X2, X3, X4, X5, X6, X7, X8;


	for (i = 0; i < 8; i++)		/* idct rows */
	{
		blk = block + (i << 3);
		if (!
			((X1 = blk[4] << 11) | (X2 = blk[6]) | (X3 = blk[2]) | (X4 =
																	blk[1]) |
			 (X5 = blk[7]) | (X6 = blk[5]) | (X7 = blk[3]))) {
			blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] =
				blk[7] = blk[0] << 3;
			continue;
		}

		X0 = (blk[0] << 11) + 128;	/* for proper rounding in the fourth stage  */

		/* first stage  */
		X8 = W7 * (X4 + X5);
		X4 = X8 + (W1 - W7) * X4;
		X5 = X8 - (W1 + W7) * X5;
		X8 = W3 * (X6 + X7);
		X6 = X8 - (W3 - W5) * X6;
		X7 = X8 - (W3 + W5) * X7;

		/* second stage  */
		X8 = X0 + X1;
		X0 -= X1;
		X1 = W6 * (X3 + X2);
		X2 = X1 - (W2 + W6) * X2;
		X3 = X1 + (W2 - W6) * X3;
		X1 = X4 + X6;
		X4 -= X6;
		X6 = X5 + X7;
		X5 -= X7;

		/* third stage  */
		X7 = X8 + X3;
		X8 -= X3;
		X3 = X0 + X2;
		X0 -= X2;
		X2 = (181 * (X4 + X5) + 128) >> 8;
		X4 = (181 * (X4 - X5) + 128) >> 8;

		/* fourth stage  */

		blk[0] = (short) ((X7 + X1) >> 8);
		blk[1] = (short) ((X3 + X2) >> 8);
		blk[2] = (short) ((X0 + X4) >> 8);
		blk[3] = (short) ((X8 + X6) >> 8);
		blk[4] = (short) ((X8 - X6) >> 8);
		blk[5] = (short) ((X0 - X4) >> 8);
		blk[6] = (short) ((X3 - X2) >> 8);
		blk[7] = (short) ((X7 - X1) >> 8);

	}							/* end for ( i = 0; i < 8; ++i ) IDCT-rows */



	for (i = 0; i < 8; i++)		/* idct columns */
	{
		blk = block + i;
		/* shortcut  */
		if (!
			((X1 = (blk[8 * 4] << 8)) | (X2 = blk[8 * 6]) | (X3 =
															 blk[8 *
																 2]) | (X4 =
																		blk[8 *
																			1])
			 | (X5 = blk[8 * 7]) | (X6 = blk[8 * 5]) | (X7 = blk[8 * 3]))) {
			blk[8 * 0] = blk[8 * 1] = blk[8 * 2] = blk[8 * 3] = blk[8 * 4] =
				blk[8 * 5] = blk[8 * 6] = blk[8 * 7] =
				iclp_char((blk[8 * 0] + 32) >> 6);
			continue;
		}

		X0 = (blk[8 * 0] << 8) + 8192;

		/* first stage  */
		X8 = W7 * (X4 + X5) + 4;
		X4 = (X8 + (W1 - W7) * X4) >> 3;
		X5 = (X8 - (W1 + W7) * X5) >> 3;
		X8 = W3 * (X6 + X7) + 4;
		X6 = (X8 - (W3 - W5) * X6) >> 3;
		X7 = (X8 - (W3 + W5) * X7) >> 3;

		/* second stage  */
		X8 = X0 + X1;
		X0 -= X1;
		X1 = W6 * (X3 + X2) + 4;
		X2 = (X1 - (W2 + W6) * X2) >> 3;
		X3 = (X1 + (W2 - W6) * X3) >> 3;
		X1 = X4 + X6;
		X4 -= X6;
		X6 = X5 + X7;
		X5 -= X7;

		/* third stage  */
		X7 = X8 + X3;
		X8 -= X3;
		X3 = X0 + X2;
		X0 -= X2;
		X2 = (181 * (X4 + X5) + 128) >> 8;
		X4 = (181 * (X4 - X5) + 128) >> 8;

		/* fourth stage  */
		blk[8 * 0] = iclp_char((X7 + X1)>> 14);
		blk[8 * 1] = iclp_char((X3 + X2)>> 14);
		blk[8 * 2] = iclp_char((X0 + X4) >> 14);
		blk[8 * 3] = iclp_char((X8 + X6)>> 14 );
		blk[8 * 4] = iclp_char((X8 - X6)>> 14);
		blk[8 * 5] = iclp_char((X0 - X4)>> 14 );
		blk[8 * 6] = iclp_char((X3 - X2)>> 14);
		blk[8 * 7] = iclp_char((X7 - X1)>> 14);
	}

// 	for (i = 0; i < 64; i++)
// 		block[i] = (short) blk[i];

// 	for (i = 0; i < 8; i++)
// 		for (j = 0; j < 8; j++)
// 		{
// 			block[j * 8 + i] = blk[j * 8 + i];
// 		}

}								/* end function idct_int32(block) */










/*
 *open usb camera device
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_open(JNIEnv * env, jclass obj, jint devid)
{
	LOGI("%s\n",__func__);
	char *devname;
	switch(devid){
		case 0:
			devname = "/dev/video0";
			break;
		case 4:
			devname = "/dev/video4";
			break;
		default:
			devname = "/dev/video4";
			break;
	}
	fd = open(devname, O_RDWR, 0);
	if (fd<0)
		LOGE("%s ++++ open error\n",devname);
	return fd;
}



/*
 * init device
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_init(JNIEnv * env, jclass obj, jint width, jint height,jint numbuf)
{
	LOGI("%s\n",__func__);
	int ret;
	int i;
	bufnum = numbuf;
	mwidth = width;
	mheight = height;
	struct v4l2_format fmt;	
	struct v4l2_capability cap;



    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        LOGE("%d :VIDIOC_QUERYCAP failed\n",__LINE__);
        return -1;
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("%d : no capture devices\n",__LINE__);
        return -1;
    }



	memset( &fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;					
	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
	{
		LOGE("++++%d : set format failed\n",__LINE__);
		return -1;
	}


/*	struct v4l2_streamparm setfps;
    //set fps
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = 30;
	ioctl(fd, VIDIOC_S_PARM, setfps);*/

    struct v4l2_requestbuffers req;
    req.count = numbuf;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;




    ret = ioctl(fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("++++%d : VIDIOC_REQBUFS failed\n",__LINE__);
        return -1;
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers) {
        LOGE ("++++%d Out of memory\n",__LINE__);
		return -1;
    }

	for(i = 0; i< bufnum; ++i) {
		memset(&v4l2_buf, 0, sizeof(v4l2_buf));
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.index = i;
		ret = ioctl(fd , VIDIOC_QUERYBUF, &v4l2_buf);
		if(ret < 0) {
		   LOGE("+++%d : VIDIOC_QUERYBUF failed\n",__LINE__);
		   return -1;
		}
		buffers[i].length = v4l2_buf.length;
		if ((buffers[i].start = (char *)mmap(0, v4l2_buf.length,
		                                     PROT_READ | PROT_WRITE, MAP_SHARED,
		                                     fd, v4l2_buf.m.offset)) < 0) {
		     LOGE("%d : mmap() failed",__LINE__);
		     return -1;
		}
	}
	return 0;
}





/*
 *stream on
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_streamon(JNIEnv * env, jclass obj)
{
	LOGI("%s\n",__func__);
	int i;
	int ret;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	for(i = 0; i< bufnum; ++i) {
		memset(&v4l2_buf, 0, sizeof(v4l2_buf));
		v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.index = i;
		ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
		if (ret < 0) {
		    LOGE("%d : VIDIOC_QBUF failed\n",__LINE__);
		    return ret;
		}
	}

    ret = ioctl(fd, VIDIOC_STREAMON, &type);


    if (ret < 0) {
        LOGE("%d : VIDIOC_STREAMON failed\n",__LINE__);
        return ret;
    }
	return 0;
}
/*
 *get one frame data
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_dqbuf(JNIEnv * env, jclass obj,const jbyteArray videodata)
{
	//LOGI("%s\n",__func__);
    int ret;

	jbyte *data = (jbyte*)(*env)->GetByteArrayElements(env, videodata, 0);
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;





    ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("%s : VIDIOC_DQBUF failed, dropped frame\n",__func__);
        return ret;
    }

	memcpy(data,buffers[v4l2_buf.index].start,buffers[v4l2_buf.index].length);
	(*env)->ReleaseByteArrayElements(env, videodata, data, 0);
	return v4l2_buf.index;
}

/*
* yuv to rgb565
*/
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_yuvtorgb(JNIEnv * env, jclass obj,const jbyteArray yuvdata, jbyteArray rgbdata,const jint dwidth,const jint dheight)
{
	//LOGI("%s\n",__func__);
	jbyte *ydata = (jbyte*)(*env)->GetByteArrayElements(env, yuvdata, 0);
	jbyte *rdata = (jbyte*)(*env)->GetByteArrayElements(env, rgbdata, 0);
	AVFrame * rpicture=NULL;
	AVFrame * ypicture=NULL;
	struct SwsContext *swsctx = NULL;
	rpicture=avcodec_alloc_frame();
	ypicture=avcodec_alloc_frame();
	avpicture_fill((AVPicture *) rpicture, (uint8_t *)rdata, PIX_FMT_RGB565,dwidth,dheight);
	avpicture_fill((AVPicture *) ypicture, (uint8_t *)ydata, AV_PIX_FMT_YUYV422,mwidth,mheight);
	swsctx = sws_getContext(mwidth,mheight, AV_PIX_FMT_YUYV422,	dwidth, dheight,PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);
	sws_scale(swsctx,(const uint8_t* const*)ypicture->data,ypicture->linesize,0,mheight,rpicture->data,rpicture->linesize);
	sws_freeContext(swsctx);
	av_free(rpicture);
	av_free(ypicture);
	(*env)->ReleaseByteArrayElements(env, yuvdata, ydata, 0);
	(*env)->ReleaseByteArrayElements(env, rgbdata, rdata, 0);
	return 0;
}



/*
 *put in frame buffer to queue
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_qbuf(JNIEnv * env, jclass obj,jint index)
{
	//LOGI("%s\n",__func__);

    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("%s : VIDIOC_QBUF failed\n",__func__);
        return ret;
    }
    return 0;
}

AVFormatContext* pFormatCtx;
AVOutputFormat* fmt;
AVStream* video_st;

AVCodecContext *pCodecCtx= NULL;
AVPacket avpkt;
FILE * video_file;
char index1[35]={};
unsigned char *outbuf=NULL;
unsigned char *yuv420buf=NULL;
unsigned char *rgb24buf=NULL;
static int outsize=0;

/*
* encording init
*/
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_videoinit(JNIEnv * env, jclass obj,jbyteArray filename)
{
	LOGI("%s",__func__);

	jbyte *filedir =(*env)->GetByteArrayElements(env, filename, 0);

	LOGD("%s",filedir);

	//纠正传输送的文件名

	int i=0;
	index1[0]=(char)filedir[0];

	LOGE("1");

	do
	{
		i++;
		index1[i]=(char)filedir[i];
	}
	while(index1[i]!= 'i');

	index1[i+1]='\0';
	LOGD("%s",index1);


	(*env)->ReleaseByteArrayElements(env, filename, filedir, 0);

	AVCodec * pCodec=NULL;
	av_register_all();

	pFormatCtx = avformat_alloc_context();
	fmt = av_guess_format(NULL, index1, NULL);
	pFormatCtx->oformat = fmt;

	//LOGD("%s",video_file);

	if (avio_open(&pFormatCtx->pb,index1, AVIO_FLAG_READ_WRITE) < 0)
		{
		LOGE("output file  open failure");
		return -1;
		}

	LOGD("avio_open\n");
//
  	video_st = av_new_stream(pFormatCtx, 0);
  	if (video_st==NULL)
  	{
  		LOGD("new_stream open failure");
  		return -1;
  	}

  	pCodecCtx = video_st->codec;
  	pCodecCtx->codec_id = fmt->video_codec;
  	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    /* put sample parameters */
    pCodecCtx->bit_rate = 6000000;
    /* resolution must be a multiple of two */
    pCodecCtx->width = mwidth;
    pCodecCtx->height = mheight;
    /* frames per second */
    pCodecCtx->time_base= (AVRational){1,15};
    pCodecCtx->gop_size = 1;
    pCodecCtx->max_b_frames=0;
    av_dump_format(pFormatCtx, 0,index1, 1);
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if(pCodec == NULL) {
		LOGD("cannot find correct encode\n");
		return -1;
	}
    /* open it */
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("encode can't be opened\n");
        return -1;
    }
    avformat_write_header(pFormatCtx,NULL);
	outsize = mwidth * mheight*3;
	outbuf = malloc(outsize*sizeof(char));
	yuv420buf = malloc(outsize*sizeof(char));
	rgb24buf = malloc(outsize*sizeof(char));

	return 1;
}



JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_videostart(JNIEnv * env, jclass obj,jbyteArray yuvdata,jintArray time,jdoubleArray gps)
{
	LOGI("%s\n",__func__);
	int frameFinished=0,size=0;
	jbyte *ydata = (jbyte*)(*env)->GetByteArrayElements(env, yuvdata, 0);
	AVFrame * yuv420pframe=NULL;
	AVFrame * yuv422frame=NULL;
	AVFrame * rgb24frame=NULL;


	markflag=markflag+1;
	struct SwsContext *swsctx = NULL;
	struct SwsContext *sws1 = NULL;
	struct SwsContext *swsctx2 = NULL;
	if(markflag<=marks)
	{

		yuv420pframe=avcodec_alloc_frame();
		yuv422frame=avcodec_alloc_frame();
		avpicture_fill((AVPicture *) yuv420pframe, (uint8_t *)yuv420buf,AV_PIX_FMT_YUV420P,mwidth,mheight);
		avpicture_fill((AVPicture *) yuv422frame, (uint8_t *)ydata, AV_PIX_FMT_YUYV422,mwidth,mheight);
		swsctx2 = sws_getContext(mwidth,mheight, AV_PIX_FMT_YUYV422,mwidth, mheight,AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		sws_scale(swsctx2,(const uint8_t* const*)yuv422frame->data,yuv422frame->linesize,0,mheight,yuv420pframe->data,yuv420pframe->linesize);
	}

	if(markflag>marks)
	{
		yuv422frame=avcodec_alloc_frame();;
		rgb24frame=avcodec_alloc_frame();
		avpicture_fill((AVPicture *) rgb24frame, (uint8_t *)rgb24buf,AV_PIX_FMT_RGB24,mwidth,mheight);
		avpicture_fill((AVPicture *) yuv422frame, (uint8_t *)ydata,AV_PIX_FMT_YUYV422,mwidth,mheight);
		swsctx = sws_getContext(mwidth,mheight,AV_PIX_FMT_YUYV422,mwidth,mheight,AV_PIX_FMT_RGB24,SWS_BICUBIC,NULL, NULL, NULL);
		sws_scale(swsctx,(const uint8_t* const*)yuv422frame->data,yuv422frame->linesize,0,mheight,rgb24frame->data,rgb24frame->linesize);


		markflag=0;

		jint *ytime = (jint*)(*env)->GetIntArrayElements(env, time, 0);
		jdouble *ygps = (jdouble*)(*env)->GetDoubleArrayElements(env, gps, 0);

		EmbeMark((unsigned char*)rgb24buf,mheight,mwidth,ytime,ygps);

		(*env)->ReleaseIntArrayElements(env, time, ytime, 0);
		(*env)->ReleaseDoubleArrayElements(env, gps, ygps, 0);

		LOGD("%d",ygps[0]);
		LOGD("%f",ytime[0]);

		rgb24buf=(uint8_t *)rgb24buf;

		yuv420pframe=avcodec_alloc_frame();
		rgb24frame=avcodec_alloc_frame();
		avpicture_fill((AVPicture *) yuv420pframe, (uint8_t *)yuv420buf,AV_PIX_FMT_YUV420P,mwidth,mheight);
		avpicture_fill((AVPicture *) rgb24frame, (uint8_t *)rgb24buf,AV_PIX_FMT_RGB24,mwidth,mheight);
		sws1 = sws_getContext(mwidth,mheight,AV_PIX_FMT_RGB24,mwidth, mheight,AV_PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);
		sws_scale(sws1,(const uint8_t* const*)rgb24frame->data,rgb24frame->linesize,0,mheight,yuv420pframe->data,yuv420pframe->linesize);
	}


	av_init_packet(&avpkt);
	size = avcodec_encode_video2(pCodecCtx, &avpkt, yuv420pframe, &frameFinished);
	if (size < 0) {
		LOGE("encode_error!\n");
		return -1;
	}
	if(frameFinished){
		avpkt.stream_index = video_st->index;
		size = av_write_frame(pFormatCtx, &avpkt);
		av_free_packet(&avpkt);
	}

	(*env)->ReleaseByteArrayElements(env, yuvdata, ydata, 0);

	sws_freeContext(swsctx2);
	sws_freeContext(swsctx);
	av_free(yuv422frame);
	sws_freeContext(sws1);
	av_free(yuv420pframe);
	av_free(rgb24frame);
}

JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_videoclose(JNIEnv * env, jclass obj)
{
	LOGI("%s\n",__func__);
	av_write_trailer(pFormatCtx);
	avcodec_close(video_st->codec);

	free(outbuf);

	avio_close(pFormatCtx->pb);
	LOGD("avio_close\n");
	avformat_free_context(pFormatCtx);

}

/*
 *streamoff
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_streamoff(JNIEnv * env, jclass obj,jint index)
{
	LOGI("%s\n",__func__);
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("%s : VIDIOC_STREAMOFF failed\n",__func__);
        return ret;
    }

    return 0;
}
/*
 *release
 */
JNIEXPORT jint JNICALL Java_com_example_giscamera_Ffmpeg_release(JNIEnv * env, jclass obj)
{
	LOGI("%s\n",__func__);
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;
	int i;
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("%s : VIDIOC_STREAMOFF failed\n",__func__);
        return ret;
    }

    for (i = 0; i < bufnum; i++) {
       ret = munmap(buffers[i].start, buffers[i].length);
		if (ret < 0) {
		    LOGE("%s : release failed\n",__func__);
		    return ret;
    	}
	}
	free (buffers);
	close(fd);
	return 0;
}
