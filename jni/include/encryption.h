#ifndef _watermarking__hh
#define _watermarking__hh

#include <time.h>  
#include <stdlib.h> 
#include <stdio.h>  
#include <string.h>
#include <math.h>

#define WMINFO_LENGTH 35  // GPS���ݳ���
#define WMINFO_WIDTH  40   // a0[]~a9[]Ԫ�ظ���
#define WMINFO_SEED   10 

typedef struct _GPSDATA GPSDATA;

struct _GPSDATA
{
	struct tm time;//UTCʱ��
	double  latitude;//γ��
	char    lat_hemi;//�ϱ�����
	double  longitude;//����
	char    lon_hemi;//��������
	int     status; //GPS״̬�� 0��ʼ���� 1���㶨λ�� 2���֣� 3��ЧPPS�� 4�̶��⣬ 5����⣬ 6���ڹ��� 7���˹�����̶�ֵ�� 8ģ��ģʽ�� 9WAAS���
	int     sum;//ʹ��������
	double  hdop;//ˮӡ��������
	double  geoHeigth;//����߶�
	char    geoHeightUnit;//����߶ȵ�λ

	double  HeigthDif;//��ƽ����������߶Ȳ�
	char    HeightDifUnit;//��ƽ����������߶Ȳλ
};

/*
**********************************************************************
* �������ƣ�
* EmbeMark()
*����������
*  unsigned char *pIamgeBuf  -ͼƬ���ݿ�
*  char* pGps        -GPS���ݴ���GPGGA��ʽ��
*  int nIamgeHeight  -ͼƬ�߶�
*  int nIamgeWidth   -ͼƬ���
*����ֵ��
*   falseΪʧ��,trueΪ�ɹ�
*˵��������һ��ͼ�����ݿ⣬�������,��GPGGA��ʽ��GPS��Ƕ�뵽����
**********************************************************************
char* EmbeMark(unsigned char* pIamgeBuf,char* pGps,int nIamgeHeight, int nIamgeWidth);

**********************************************************************
* �������ƣ�
* WriteWmInfo()
*����������
*  char* fileName  -�ļ���
*  char* pWminfo   -ˮӡ��Ϣ
*  double sampleTime -ʱ���
*����ֵ��
*   void
*˵��������ǰ֡Ƕ���GPS��Ϣ����¼���ı��ļ���
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

