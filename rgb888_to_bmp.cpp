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

#pragma pack(push, 1)
struct BMPFileHeader
{
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
};

struct BMPInfoHeader
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t x_pels_per_meter;
    int32_t y_pels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;
};

#pragma pack(pop)

bool write_bmp(const char* filename, uint8_t* data, int32_t width, int32_t height)
{
    BMPFileHeader file_header = { 0 };
    BMPInfoHeader info_header = { 0 };
    std::ofstream ofs(filename, std::ios::binary);
    if(!ofs)
    {
        cout << "111111111111111111" << endl;
        return false;
    }
    file_header.type = 0x4D42;
    file_header.size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + width * height * 3;
    file_header.offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));

    info_header.size = sizeof(BMPInfoHeader);
    info_header.width = width;
    info_header.height = height;
    info_header.planes = 1;
    info_header.bit_count = 24;
    info_header.size_image = width * height * 3;
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    int32_t row_size = ((width * 3 + 3) / 4) * 4;
    uint8_t* row_data = new uint8_t[row_size];
    for(int32_t y = height - 1; y >= 0; --y)
    {
        for(int32_t x = 0; x < width; ++x)
        {
            //cout << (int)data[(y * width + x) * 3 + 0] << endl;
            row_data[x * 3 + 2] = (int)data[(y * width + x) * 3 + 2];
            row_data[x * 3 + 1] = (int)data[(y * width + x) * 3 + 1];
            row_data[x * 3 + 0] = (int)data[(y * width + x) * 3 + 0];
        }
        ofs.write(reinterpret_cast<char*>(row_data), row_size);
    }
    delete[] row_data;
    ofs.close();
    return true;

}

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

int main() 
{
    int height = 1080;
	int width = 1920;
    int res;
    char* rgb_final = (char *)malloc(1920*1080*3);
    int len = 0;
    FILE *fp;
    for(int i = 0; i < 1; i++)
    {
        string str = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20210427_105739/1.raw";
        string str1 = "/home/zujieliu/share/tu_cam_viewer_release_v4.6.2/20210427_105739/1.bmp";
        cout << str << endl;
        const char* p = str.data();
        const char* p1 = str1.data();
        res = libcamip_open_file(rgb_final, p);     
        //uint8_t* data = static_cast<uint8_t>(rgb_final);
        uint8_t* data = new uint8_t[1920 * 1080 * 3];
        for(int32_t y = 0; y < 1080; ++y)
        {
            for(int32_t x = 0; x < 1920; ++x)
            {
                int32_t index = (y * 1920 + x) * 3;
                //cout << rgb_final[index + 0] << endl;
                data[index + 0] = rgb_final[index + 0];
                data[index + 1] = rgb_final[index + 1];
                data[index + 2] = rgb_final[index + 2];
            }
        }
        res = write_bmp(p1, data, width, height);
    }
    return 0;
}
