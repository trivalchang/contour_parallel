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
    # ./contour
the program reads the image files under cup and outputs the contour under output.

### Performance 

    Compared to serial version, TBB version is about 40% faster on dual core MacBook Pro and 3~4 times faster on quad core ubuntu 

### Known issue

    Due to thread safe issue, imshow() in make_filter causes execption
