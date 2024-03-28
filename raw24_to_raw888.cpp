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
#define	 HEIGHT 1080
#define  WIDTH 1920
#define  DRANGE 23.5

double	CP[MAX_CP], CP_raw8[MAX_BIN], CP_raw24[MAX_CP];
double	HIST[MAX_BIN], ACC_HIST[MAX_BIN];
double	LOG_LUM[HEIGHT][WIDTH], LUM8[HEIGHT][WIDTH], LUM24[HEIGHT][WIDTH], RATIO[HEIGHT][WIDTH], RGB[HEIGHT][WIDTH][3];

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
            //cout << L << endl;
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

int raw8_to_rgb(char *raw8_new, char *rgb_final)
{
    unsigned char *ptr;
    ptr = (unsigned char *)raw8_new;
    for(int v = 1; v < 1079; v++)
    {
        for(int h = 1; h < 1919; h++)
        {
            //cout << (int)RGB[v][h - 1][1] << endl;
            if(v % 2 == 0 and h % 2 == 0)
            {
                RGB[v][h][0] = (int)ptr[v * 1920 + h];
                RGB[v][h][1] = (int)(ptr[(v - 1) * 1920 + h] + ptr[(v + 1) * 1920 + h] + ptr[v * 1920 + h + 1] + ptr[v * 1920 + h + 1]) / 4.;
                RGB[v][h][2] = (int)(ptr[(v - 1) * 1920 + h - 1] + ptr[(v + 1) * 1920 + h - 1] + ptr[(v - 1) * 1920 + h + 1] + ptr[(v + 1) * 1920 + h + 1]) / 4.;
            }
            if(v % 2 == 1 and h % 2 == 1)
            {
                RGB[v][h][2] = (int)ptr[v * 1920 + h];
                RGB[v][h][0] = (int)(ptr[(v - 1) * 1920 + h - 1] + ptr[(v + 1) * 1920 + h - 1] + ptr[(v - 1) * 1920 + h + 1] + ptr[(v + 1) * 1920 + h + 1]) / 4.;
                RGB[v][h][1] = (int)(ptr[(v - 1) * 1920 + h] + ptr[(v + 1) * 1920 + h] + ptr[v * 1920 + h + 1] + ptr[v * 1920 + h + 1]) / 4.;
            }
            if(v % 2 == 0 and h % 2 == 1)
            {
                RGB[v][h][1] = (int)ptr[v * 1920 + h];
                RGB[v][h][0] = (int)(ptr[v * 1920 + h - 1] + ptr[v * 1920 + h + 1]) / 2.;
                RGB[v][h][2] = (int)(ptr[(v - 1) * 1920 + h] + ptr[(v + 1) * 1920 + h]) / 2.;
            }
            if(v % 2 == 1 and h % 2 == 0)
            {
                RGB[v][h][1] = (int)ptr[v * 1920 + h];
                RGB[v][h][0] = (int)(ptr[(v - 1) * 1920 + h] + ptr[(v + 1) * 1920 + h]) / 2.;
                RGB[v][h][2] = (int)(ptr[v * 1920 + h - 1] + ptr[v * 1920 + h + 1]) / 2.;
            }
        }
    }
    int r = 0;
    int g = 0;
    int b = 0;
    for(int v = 0; v < 1080; v++)
    {
        for(int h = 0; h < 1920; h++)
        {
            r = (int)RGB[v][h][0];
            g = (int)RGB[v][h][1];
            b = (int)RGB[v][h][2];
            if(r > 255.)
            {
                r = 255.;
            }
            if(r < 0.)
            {
                r = 0.;
            }
            if(g > 255.)
            {
                g = 255.;
            }
            if(g < 0.)
            {
                g = 0.;
            }
            if(b > 255.)
            {
                b = 255.;
            }
            if(b < 0.)
            {
                b = 0.;
            }
            int i = v * 1920 + h;
            rgb_final[3 * i] = b;
            rgb_final[3 * i + 1] = g;
            rgb_final[3 * i + 2] = r;
            //cout << r + b + g << endl;
        }
        //cout << r + b + g << endl;    
    }
    return 0; 
}
int main() 
{
    int height = 1080;
	int width = 1920;
    int res;
    char* raw24 = (char *)malloc(1920*1080*3);
    char* raw8 = (char *)malloc(1920*1080);
    char* rgb_final = (char *)malloc(1920*1080*3);
    int len = 0;
    FILE *fp;
    for(int i = 0; i < 1; i++)
    {
        string str = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20210427_105739/0.raw";
        string str1 = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20210427_105739/1.raw";
        cout << str << endl;
        const char* p = str.data();
        const char* p1 = str1.data();
        res = libcamip_open_file(raw24, p);
        res = tone_mapping(raw24, raw8);
        res = raw8_to_rgb(raw8, rgb_final);
        fp = fopen(p1, "wb");
        len = fwrite(rgb_final, 1920 * 1080 * 3, 1, fp);
        fclose(fp);
    }
    return 0;
}