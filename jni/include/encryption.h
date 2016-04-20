#ifndef _watermarking__hh
#define _watermarking__hh

#include <time.h>  
#include <stdlib.h> 
#include <stdio.h>  
#include <string.h>
#include <math.h>

#define WMINFO_LENGTH 35  // GPS数据长度
#define WMINFO_WIDTH  40   // a0[]~a9[]元素个数
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

/*
**********************************************************************
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
**********************************************************************
char* EmbeMark(unsigned char* pIamgeBuf,char* pGps,int nIamgeHeight, int nIamgeWidth);

**********************************************************************
* 函数名称：
* WriteWmInfo()
*函数参数：
*  char* fileName  -文件名
*  char* pWminfo   -水印信息
*  double sampleTime -时间戳
*返回值：
*   void
*说明：将当前帧嵌入的GPS信息，记录到文本文件中
**********************************************************************
void WriteWmInfo(char* fileName,char* pWminfo,double sampleTime);


int AnalyzeGPGGA(char* pGps,GPSDATA* data);
void GetWmInfoFromGPGGA(const GPSDATA* data,char* pWmInfo);
int** CreateMeaninglessWMInfor(char* WMInfo);
void ReleaseWMInfo(int** WMInfo);
int* FetchBpointFromImage(unsigned char* pBuf,int nIamgeHeight, int nIamgeWidth);
void ReturnBpointToImage(unsigned char* pBuf,int *pBpiontArry,int nIamgeHeight, int nIamgeWidth);
void ReleaseBpointArry(int* pBpointArry);
void EmbedDCT24(int** WMInfo,int* pBpointArry,int nIamgeHeight,int nIamgeWidth);
void itoa(int i,char*string);



*/

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
/*void fdct_int32(short *const block);
void idct_int32(short *const block);
static  inline  int iclp_char( int x );*/


#endif // vcompress.h

