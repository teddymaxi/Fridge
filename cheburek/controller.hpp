#include <iostream>
#include <opencv2/opencv.hpp>

class Controller
{
    bool is_drawing = false;
    cv::Mat image;
    cv::Mat drawable_layer;
    int ix, iy;
    bool mode = true;

public:
    void draw_ui();
    void start();
    int imregionalmax(cv::Mat &input, int nLocMax, float threshold, float minDistBtwLocMax, cv::Mat &locations);
    friend void onMouse(int event, int x, int y, int, void* data);
};

void onMouse(int event, int x, int y, int, void* data);
