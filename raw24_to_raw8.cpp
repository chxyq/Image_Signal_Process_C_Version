#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#include <dirent.h>
#include <ctime>
#include <cmath>
//#include <io.h>
#include <stdio.h>
////#include <opencv/cv.h>
//#include <opencv/highgui.h>
//#include <opencv2/core/core_c.h>
//#include <opencv2/highgui/highgui_c.h>
//#include <opencv2/imgproc/imgproc_c.h>

using namespace std;
//using namespace cv;
#define  MAX_CP   1504
#define  MAX_BIN  376
#define	 HEIGHT 2160
#define  WIDTH 3840
#define  DRANGE 23.5

double	CP[MAX_CP], CP_raw8[MAX_BIN], CP_raw24[MAX_CP];
double	HIST[MAX_BIN], ACC_HIST[MAX_BIN];
double	LOG_LUM[HEIGHT][WIDTH], LUM8[HEIGHT][WIDTH], LUM24[HEIGHT][WIDTH], RATIO[HEIGHT][WIDTH];

static string  getCurrentTimeStr()
{
    time_t t = time(NULL);
    char ch[64] = {0};
    strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));     //年-月-日 时-分-秒
    return ch;
}

int libcamip_open_file(char *camimg_ptr,const char *file)
{
	FILE *fp;
	int len = 0;
	if(camimg_ptr == NULL || file == NULL)
		return 1;
	fp = fopen(file,"rb");
	if(NULL == fp)
	{
		fclose(fp);
		return 1;
	}
	//len = fread(camimg_ptr->imageData,1,camimg_ptr->imageSize,fp);
    // camimg_ptr = (char *)malloc(1920*1080*3);
	len = fread(camimg_ptr, 1920*1080*3, 1, fp);
    //img = ptr;
	fclose(fp);
	return 0;
}

int tone_mapping(char *raw24, char *raw8)
{
    int res;
    int height = 1080;
    int width = 1920;
    double step = DRANGE / MAX_CP;
    unsigned char *ptr;
    for( int bin = 0; bin < MAX_BIN; bin++)
    {
        HIST[bin] = 0.;
        ACC_HIST[bin] = 0.;
    }
    ptr = (unsigned char *)raw24;
    int p;
    int index1 = 0;
    for(int v = 0; v < 1080; v++)
        for(int h = 0; h < 1920; h++)
        {
            p = ptr[ v * 3 * 1920 + h * 3] | ptr[ v * 3 * 1920 + h * 3 + 1] << 8 | ptr[ v * 3 * 1920 + h * 3 + 2] << 16;
            //p = (p < 1) ? 0. : p;
            double L = (p < 1.) ? 0. : log(p) / log(2.);
            L = (L > DRANGE) ? DRANGE : L;
            LOG_LUM[v][h] = L;
            index1 = int(L * 375. / DRANGE);
            HIST[index1] += 1.;
        }
    ACC_HIST[0] = HIST[0];
    for(int i = 1; i < MAX_BIN; i++)
    {
        if(i >= 2 && i <= (MAX_BIN -2))
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        else
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        
    }
    double base[5] = {0.005, 0.01, 0.5, 0.9999, 0.99999};
    base[0] = (double)((ACC_HIST[(MAX_BIN-1)] * base[0]) + 0.5);
    base[1] = (double)((ACC_HIST[(MAX_BIN-1)] * base[1]) + 0.5);
    base[2] = (double)((ACC_HIST[(MAX_BIN-1)] * base[2]) + 0.5);
    base[3] = (double)((ACC_HIST[(MAX_BIN-1)] * base[3]) + 0.5);
    base[4] = (double)((ACC_HIST[(MAX_BIN-1)] * base[4]) + 0.5);
    double	bin_step = DRANGE / MAX_BIN;
    int pos;
    double	i_base[5];
    double	o_base[5] =   {0.0, 4.0, 130.0, 245.0, 250.0};
    for (int j = 0; j < 5; j++)
    {
        pos = 0;
        for(int i = 0; i < MAX_BIN; i++)
        {
            pos = i;
            if(ACC_HIST[i] >= base[j])
            {
                break;
            }
        }
        i_base[j] = pos * bin_step;
    }
    double	i_L, i_H;
	double	o_L, o_H;
    o_base[3] = i_base[3] / DRANGE * 250.0;
    o_base[4] = i_base[4] / DRANGE * 250.0;
    if(i_base[2] <= 10.)
    {
        o_base[2] = 40.;
    }
    else
    {
        o_base[2] = (i_base[2] - 10.) * 10. + 40.;
    }
    if(o_base[2] >= 130.)
    {
        o_base[2] = 130.;
    }
    
    for(int i = 0; i < MAX_CP; i++) CP[i] = 0.;
    for(int i = 0; i < MAX_CP; i++)
    {
        double in_pos = i * step;
        in_pos = (i_base[0] > in_pos) ? i_base[0] : in_pos;
        in_pos = (i_base[5 - 1] < in_pos) ? i_base[5 - 1] : in_pos;
        for(int j = 0; j < 5; j++)
        {
            i_H = i_base[j];
            o_H = o_base[j];
            i_L = ((j - 1) < 1) ? i_base[0] : i_base[j - 1];
            o_L = ((j - 1) < 1) ? o_base[0] : o_base[j - 1];
            if(in_pos < i_base[j])
            {
                break;
            }
        }
        double ratio = (in_pos - i_L) / (i_H - i_L + 0.00000001);
        CP[i] = int(o_L + (o_H - o_L) * ratio);
    }
    int index2 = 0;
    int nor_log_lum = 0;
    for(int v = 0; v < 1080; v++)
    {
        for(int h = 0; h < 1920; h++)
        {
            index2 = (int)(LOG_LUM[v][h] / step);
            index2 = (index2 >= MAX_CP) ? MAX_CP -1 : index2;
            nor_log_lum = CP[index2];
            //nor_log_lum = (int)((CP[index2] + ((LOG_LUM[v][h] - (index2 * step)) / step)) + 0.5);
            if(nor_log_lum > 255.)
            {
                nor_log_lum = 255.;
            }
            if(nor_log_lum < 0.)
            {
                nor_log_lum = 0.;
            }
            int i = v * 1920 + h;
            raw8[i] = nor_log_lum;
        }
    }
    return 0;
}

int raw8_to_raw24(char *raw8, char *raw24_new)
{
    int res;
    int height = 1080;
    int width = 1920;
    unsigned char *ptr;
    for( int bin = 0; bin < MAX_BIN; bin++)
    {
        HIST[bin] = 0.;
        ACC_HIST[bin] = 0.;
    }
    ptr = (unsigned char *)raw8;
    int p;
    int index1 = 0;
    for(int v = 0; v < 1080; v++)
        for(int h = 0; h < 1920; h++)
        {
            p = ptr[v * 1920 + h];
            //p = (p < 1) ? 0. : p;
            int L = p;
            LUM8[v][h] = L;
            HIST[L] += 1.;
        }
    ACC_HIST[0] = HIST[0];
    for(int i = 1; i < MAX_BIN; i++)
    {
        if(i >= 2 && i <= (MAX_BIN -2))
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        else
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        
    }
    double base[5] = {0.005, 0.01, 0.5, 0.9999, 0.99999};
    base[0] = (double)((ACC_HIST[(MAX_BIN-1)] * base[0]) + 0.5);
    base[1] = (double)((ACC_HIST[(MAX_BIN-1)] * base[1]) + 0.5);
    base[2] = (double)((ACC_HIST[(MAX_BIN-1)] * base[2]) + 0.5);
    base[3] = (double)((ACC_HIST[(MAX_BIN-1)] * base[3]) + 0.5);
    base[4] = (double)((ACC_HIST[(MAX_BIN-1)] * base[4]) + 0.5);
    int pos;
    double	i_base[5];
    double	o_base[5] =   {7.0, 8.0, 13.0, 21.5, 23.0};
    for (int j = 0; j < 5; j++)
    {
        pos = 0;
        for(int i = 0; i < MAX_BIN; i++)
        {
            pos = i;
            if(ACC_HIST[i] >= base[j])
            {
                break;
            }
        }
        i_base[j] = pos;
    }
    double	i_L, i_H;
	double	o_L, o_H;
    o_base[3] = i_base[3] / 255 * 23.0;
    o_base[4] = i_base[4] / 255 * 23.0;
    if(i_base[2] < 40.)
    {
        o_base[2] = 10.;
    }
    else
    {
        o_base[2] = (i_base[2] - 40.) * 0.1 + 10.;
    }
    if(o_base[2] > 19.)
    {
        o_base[2] = 19.;
    }
    for(int i = 0; i < 256; i++) CP_raw8[i] = 0.;
    for(int i = 0; i < 256; i++)
    {
        if(i < i_base[0])
        {
            CP_raw8[i] = o_base[0];
        }
        else if (i < i_base[1])
        {
            double ratio = (i - i_base[0]) / (i_base[1] - i_base[0] + 0.00000001);
            CP_raw8[i] = o_base[0] + (o_base[1] - o_base[0]) * ratio;
        }
        else if (i < i_base[2])
        {
            double ratio = (i - i_base[1]) / (i_base[2] - i_base[1] + 0.00000001);
            CP_raw8[i] = o_base[1] + (o_base[2] - o_base[1]) * ratio;
        }
        else if (i < i_base[3])
        {
            double ratio = (i - i_base[2]) / (i_base[3] - i_base[2] + 0.00000001);
            CP_raw8[i] = o_base[2] + (o_base[3] - o_base[2]) * ratio;
        }
        else if (i < i_base[4])
        {
            double ratio = (i - i_base[3]) / (i_base[4] - i_base[3] + 0.00000001);
            CP_raw8[i] = o_base[3] + (o_base[4] - o_base[3]) * ratio;
        }
        else
        {
            CP_raw8[i] = o_base[4];
        }        
    }
    int index2 = 0;
    float nor_log_lum = 0;
    int index3 = 0;
    for( int bin = 0; bin < MAX_BIN; bin++)
    {
        HIST[bin] = 0.;
        ACC_HIST[bin] = 0.;
    }
    for(int v = 0; v < 1080; v++)
    {
        for(int h = 0; h < 1920; h++)
        {
            index2 = (int)(LUM8[v][h]);
            nor_log_lum = CP_raw8[index2];
            LUM24[v][h] = nor_log_lum;
            index3 = int(nor_log_lum * 255. / DRANGE);
            HIST[index3] += 1;
            //nor_log_lum = (int)((CP[index2] + ((LOG_LUM[v][h] - (index2 * step)) / step)) + 0.5);
            int i = v * 1920 + h;
            raw24_new[i] = nor_log_lum;
        }
    }
    return 0;
}

int raw24_to_raw8(char *raw24_new, char *raw8_new)
{
    int height = 1080;
    int width = 1920;
    double step = DRANGE / MAX_CP;
    unsigned char *ptr;
    ptr = (unsigned char *)raw24_new;
    float p;
    int index1 = 0;
    ACC_HIST[0] = HIST[0];
    for(int i = 1; i < MAX_BIN; i++)
    {
        if(i >= 2 && i <= (MAX_BIN -2))
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        else
        {
            ACC_HIST[i] = ACC_HIST[i - 1] + HIST[i];
        }
        
    }
    double base[5] = {0.005, 0.01, 0.5, 0.9999, 0.99999};
    base[0] = (double)((ACC_HIST[(MAX_BIN-1)] * base[0]) + 0.5);
    base[1] = (double)((ACC_HIST[(MAX_BIN-1)] * base[1]) + 0.5);
    base[2] = (double)((ACC_HIST[(MAX_BIN-1)] * base[2]) + 0.5);
    base[3] = (double)((ACC_HIST[(MAX_BIN-1)] * base[3]) + 0.5);
    base[4] = (double)((ACC_HIST[(MAX_BIN-1)] * base[4]) + 0.5);
    double	bin_step = DRANGE / MAX_BIN;
    int pos;
    double	i_base[5];
    double	o_base[5] =   {0.0, 4.0, 130.0, 245.0, 250.0};
    for (int j = 0; j < 5; j++)
    {
        pos = 0;
        for(int i = 0; i < MAX_BIN; i++)
        {
            pos = i;
            if(ACC_HIST[i] >= base[j])
            {
                break;
            }
        }
        i_base[j] = pos * bin_step;
    }
    double	i_L, i_H;
	double	o_L, o_H;
    o_base[3] = i_base[3] / DRANGE * 250.0;
    o_base[4] = i_base[4] / DRANGE * 250.0;
    if(i_base[2] <= 10.)
    {
        o_base[2] = 40.;
    }
    else
    {
        o_base[2] = (i_base[2] - 10.) * 10. + 40.;
    }
    if(o_base[2] >= 130.)
    {
        o_base[2] = 130.;
    }
    for(int i = 0; i < MAX_CP; i++) CP_raw24[i] = 0.;
    for(int i = 0; i < MAX_CP; i++)
    {
        double in_pos = i * step;
        in_pos = (i_base[0] > in_pos) ? i_base[0] : in_pos;
        in_pos = (i_base[5 - 1] < in_pos) ? i_base[5 - 1] : in_pos;
        for(int j = 0; j < 5; j++)
        {
            i_H = i_base[j];
            o_H = o_base[j];
            i_L = ((j - 1) < 1) ? i_base[0] : i_base[j - 1];
            o_L = ((j - 1) < 1) ? o_base[0] : o_base[j - 1];
            if(in_pos < i_base[j])
            {
                break;
            }
        }
        double ratio = (in_pos - i_L) / (i_H - i_L + 0.00000001);
        CP_raw24[i] = int(o_L + (o_H - o_L) * ratio);
    }
    int index2 = 0;
    int nor_log_lum = 0;
    for(int v = 0; v < 1080; v++)
    {
        for(int h = 0; h < 1920; h++)
        {
            index2 = (int)(LUM24[v][h] / step);
            index2 = (index2 >= MAX_CP) ? MAX_CP -1 : index2;
            nor_log_lum = CP_raw24[index2];
            //nor_log_lum = (int)((CP[index2] + ((LOG_LUM[v][h] - (index2 * step)) / step)) + 0.5);
            if(nor_log_lum > 255.)
            {
                nor_log_lum = 255.;
            }
            if(nor_log_lum < 0.)
            {
                nor_log_lum = 0.;
            }
            int i = v * 1920 + h;
            raw8_new[i] = nor_log_lum;
        }
    }
    return 0;
}
/*int raw8_to_rgb(char *raw8_new, IplImage *rgb_final)
{
    IplImage *rgb;
    rgb = cvCreateImage(cvSize(1080,1920),8,3);
    memcpy(rgb->imageData, raw8_new ,1080*1920*3);
    cvCvtColor(rgb , rgb_final , CV_BGR2RGB);
    cvReleaseImage(&rgb);
    return 0; 
}*/
int main() 
{
    int height = 1080;
	int width = 1920;
    int res;
    //double rgb[1080][1920];
    //res = raw8_to_rgb();
    //struct _finddata_t fileinfo;
    //string in_path;
    //string in_name;
    char* raw24 = (char *)malloc(1920*1080*3);
    char* raw8 = (char *)malloc(1920*1080);
    //char* raw24_new = (char *)malloc(1920*1080);
    //char* raw8_new = (char *)malloc(1920*1080);
    //IplImage* rgb_final = (IplImage *)malloc(1920*1080*3);
    //IplImage *rgb_final;
    int len = 0;
    FILE *fp;
    //string filepath ="/home/tusimple/workspace/20210128/2021-01-28-13-14-49-452099/camera4_4/161181";
    //string filepath1 ="/home/tusimple/workspace/20210128/2021-01-28-13-14-49-452099/raw8/1611811";
    for(int i = 0; i < 2; i++)
    {
        int c = 0;
        //c = 112415+ i * 20;
        string str = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20201106_174129/0.raw";
        string str1 = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20201106_174129/1.raw";
        cout << str << endl;
        const char* p = str.data();
        const char* p1 = str1.data();
        res = libcamip_open_file(raw24, p);
        res = tone_mapping(raw24, raw8);
        //res = raw8_to_rgb(raw8, rgb_final);
        //cv2::imwrite("/home/tusimple/newdisk4t/workspace/20210330/cppdemo/8.jpg", rgb_final);
        //cvSaveImage("/home/tusimple/newdisk4t/workspace/20210330/cppdemo/8.jpg", rgb_final);
        fp = fopen(p1, "wb");
        len = fwrite(raw8, 1920 * 1080, 1, fp);
        fclose(fp);
    }
    //IplImage *rgb_final;
    //clock_t start = clock();
    //res = raw8_to_raw24(raw8, raw24_new);
    //res = raw24_to_raw8(raw24_new, raw8_new);
    //res = raw8_to_rgb(raw8_new, rgb_final);
    //cvSaveImage("/home/tusimple/workspace/20210115/2021-01-15-13-01-55-573904/1/8.jpg", rgb_final);
    //clock_t end = clock();
    //cout << double(end - start) / CLOCKS_PER_SEC << endl;
    //cout << 0;
    //cout << log(2.71828183);
    //fclose(fp);
    return 0;
}