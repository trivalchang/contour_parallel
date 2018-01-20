# contour_parallel
The purpose of this program is to demostrate how to combin intel TBB pipeline and OpenCV to draw the contours of images. It also compares the performance of parallel and serial version of the smae program


### Prerequisites
OpenCV 3.3 and Intel TBB package is required

macOS
    
    please refer to http://macappstore.org/tbb/
    basically, just execute "brew install tbb" on your terminal

ubuntu

### Installing
    # cd contour_parallel
    # cmake .
    # make
    
### Running the program
	C++ version
    # ./contour
    
    python version
    # python contour.py
the program reads the image files under cup and outputs the contour under output.

### Performance 

	on dual-core Macbook Pro
	C++ TBB parallel - 0.11 sec
	C++ serial - 0.22 sec
	Python serial - 0.27 sec

### Known issue

    Due to thread safe issue, imshow() in make_filter causes execption
