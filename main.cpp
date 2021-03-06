// Sketch of one way to do a scaling study
#include <iostream>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
#include "tbb/pipeline.h"
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string>

#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>

#ifdef HAVE_CUDA

#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudaimgproc.hpp"

#endif


#if defined(__APPLE__) && defined(__MACH__)

#include <cpuid.h>

#define CPUID(INFO, LEAF, SUBLEAF) __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])

#define GETCPU(CPU) {                              \
        uint32_t CPUInfo[4];                           \
        CPUID(CPUInfo, 1, 0);                          \
        /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */ \
        if ( (CPUInfo[3] & (1 << 9)) == 0) {           \
          CPU = -1;  /* no APIC on chip */             \
        }                                              \
        else {                                         \
          CPU = (unsigned)CPUInfo[1] >> 24;                    \
        }                                              \
        if (CPU < 0) CPU = 0;                          \
      }
#else

#define GETCPU(CPU) {                                   \
        int _cpu, _status;                              \
        _status = syscall(SYS_getcpu, &_cpu, NULL, NULL); \
        if (_status != 0)                                \
            CPU = 0;                                    \
        else                                            \
            CPU = _cpu;                                  \
      }

#endif

using namespace tbb;
using namespace cv;
using namespace std;

int getdir (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) 
    {
        if (dirp->d_type & DT_REG)
        {
            string name = dir+"/"+dirp->d_name;
            files.push_back(string(name));
        }
    }
    closedir(dp);
    return 0;
}


void findContourParallel(vector<string> &files) {
    int idx=0;
    int writeIdx = 0;
    RNG rng(12345);
    
    //namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    parallel_pipeline( /*max_number_of_live_token=*/16,       
        make_filter<void, Mat>(
            filter::serial,
            [&](flow_control& fc)-> Mat
            {
            	int cpu;
            	Mat image;
            	
    			GETCPU(cpu);
				if (files[idx] != *files.end())
				{
                    //cout << "CPU " << cpu  << " filter 0 checking " << files[idx] << endl;				
				    image = imread(files[idx], CV_LOAD_IMAGE_COLOR); 
                    //cout << "CPU " << cpu << " filter 0 Done " << endl;
                    idx++;
				    return image;
				}
				else
				{
				    fc.stop();
                    return image;
				}
            }    
        ) &
        make_filter<Mat,Mat>(
            filter::parallel,
            [&](Mat image)
            {
				int cpu;
				Mat grayImg, binaryImg;
				
				GETCPU(cpu);

                //cout << "CPU " << cpu << " filter 1 gray " << endl;

                cvtColor(image, grayImg, CV_RGB2GRAY);
                threshold(grayImg, binaryImg, 128, 255, THRESH_BINARY_INV);

                //cout << "CPU " << cpu << " filter 1 Done " << endl;
            	return binaryImg;
			} 
        ) &
        make_filter<Mat,Mat>(
            filter::parallel,
            [&](Mat image)
            {
				int cpu;
				vector<vector<Point> > contours;
				vector<Vec4i> hierarchy;
				Mat drawing = Mat::zeros( image.size(), CV_8UC3 );
				
				GETCPU(cpu);
                findContours( image, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

                for( int i = 0; i< contours.size(); i++ )
                {
                    Scalar color = Scalar(rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255) );
                    drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
                }

                
                //cout << "CPU " << cpu << " filter 2 Done " << endl;
            	return drawing;
			} 
        ) &  
        make_filter<Mat,void>(
            filter::parallel,
            [&](Mat image) 
            {
				int cpu;

                GETCPU(cpu);
				string name = string("./output/cup")+ to_string(idx) + string("_parallel.png");
				idx++;
				//cout << "CPU " << cpu << " filter 3 write " << name << endl;
				imwrite(name , image);
                //cout << "CPU " << cpu << " filter 3 Done " << endl;
			}
        )
    );
}

void findContourSerial(vector<string> &files) {
    int idx=0;
    Mat image, grayImg, binaryImg;
    RNG rng(12345);

    tbb::tick_count::interval_t t0_threshold((double)0), t0_gray((double)0), t0_findContour((double)0);;
    
    //namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    for (vector<string>::iterator it = files.begin() ; it != files.end(); ++it)
    {
	    string name = string("./output/cup")+ to_string(idx++) + string("_serial.png");    
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        Mat drawing;
        tbb::tick_count t0;
        
        image = imread(*it, CV_LOAD_IMAGE_COLOR); 

        t0 = tbb::tick_count::now();
        cvtColor(image, grayImg, CV_RGB2GRAY);
        t0_gray += (tbb::tick_count::now()-t0);

        t0 = tbb::tick_count::now();
        threshold(grayImg, binaryImg, 128, 255, THRESH_BINARY_INV);
        t0_threshold += (tbb::tick_count::now() - t0);
        
        drawing = Mat::zeros( binaryImg.size(), CV_8UC3 );

        t0 = tbb::tick_count::now();
        findContours( binaryImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
        t0_findContour += (tbb::tick_count::now()-t0);
        
        for( int i = 0; i< contours.size(); i++ )
        {
            Scalar color = Scalar(rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255) );
            drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
        }

        
        imwrite(name , drawing);
    }

    cout << "   gray takes " << t0_gray.seconds() << endl;
    cout << "   threshold takes " << t0_threshold.seconds() << endl;   
    cout << "   findContour takes " << t0_findContour.seconds() << endl;
}

#ifdef HAVE_CUDA
void findContourSerialCuda(vector<string> &files) {
    int idx=0;
    cv::cuda::GpuMat imgCuda, grayCuda, binaryCuda;
    tbb::tick_count::interval_t t0_upload((double)0), 
                                t0_threshold((double)0), 
                                t0_gray((double)0),
                                t0_findContour((double)0);

    printf("CUDA Device count = %d\n", cuda::getCudaEnabledDeviceCount());

    RNG rng(12345);
    //namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    for (vector<string>::iterator it = files.begin() ; it != files.end(); ++it)
    {
	    string name = string("./output/cup")+ to_string(idx++) + string("_cuda.png");    
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        Mat drawing;
        tbb::tick_count t0;

        t0 = tbb::tick_count::now();
        imgCuda.upload(imread(*it, CV_LOAD_IMAGE_COLOR));
        t0_upload += (tbb::tick_count::now() - t0);

        t0 = tbb::tick_count::now();
        cuda::cvtColor(imgCuda, grayCuda, CV_RGB2GRAY);
        t0_gray += (tbb::tick_count::now() - t0);

        t0 = tbb::tick_count::now();
        cuda::threshold(grayCuda, binaryCuda, 128, 255, THRESH_BINARY_INV);
        t0_threshold += (tbb::tick_count::now() - t0);

        t0 = tbb::tick_count::now();
        findContours( Mat(binaryCuda), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
        t0_findContour += (tbb::tick_count::now() - t0);        

        drawing = Mat::zeros( binaryCuda.size(), CV_8UC3 );
        
        for( int i = 0; i< contours.size(); i++ )
        {
            Scalar color = Scalar(rng.uniform(0,255), rng.uniform(0,255), rng.uniform(0,255) );
            drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
        }

        
        imwrite(name , drawing);

    }

    cout << "   upload takes " << t0_upload.seconds() << endl;
    cout << "   gray takes " << t0_gray.seconds() << endl;
    cout << "   threshold takes " << t0_threshold.seconds() << endl;
    cout << "   findContour takes " << t0_findContour.seconds() << endl;
    
}
#endif

int main() 
{
	vector<string> imgNameList;
	getdir("./cup", imgNameList);
	cout << "img count = " << imgNameList.size() << endl;

    tbb::tick_count t0 = tbb::tick_count::now();
    findContourParallel(imgNameList);  
    tbb::tick_count t1 = tbb::tick_count::now();
    cout << "findContourParallel takes " << (t1 - t0).seconds() << endl;

    t0 = tbb::tick_count::now();
    findContourSerial(imgNameList); 
    t1 = tbb::tick_count::now();
    cout << "findContourSerial takes " << (t1 - t0).seconds() << endl;

#ifdef HAVE_CUDA    
    t0 = tbb::tick_count::now();
    findContourSerialCuda(imgNameList); 
    t1 = tbb::tick_count::now();
    cout << "findContourSerialCuda takes " << (t1 - t0).seconds() << endl;
#endif

    return 0;
}


