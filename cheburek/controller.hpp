#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/types.hpp>

#define EMPTY_RECT cv::Rect{0,0,0,0}

class Controller
{
    bool is_drawing = false;
    cv::Mat image;
    cv::Mat drawable_layer;
    int ix, iy;
    std::string info;
    std::string status;

    int pixels_per_sm;
    int image_rotation_deg;

    cv::Rect mirror1 = EMPTY_RECT;
    cv::Rect mirror2 = EMPTY_RECT;
    cv::Rect mouse = EMPTY_RECT;

public:
    bool draw_ui();
    void reset_drawing();
    bool mirrors_ready();
    void start(std::string img_name = "cheburek.jpg");
    int imregionalmax(cv::Mat &input, int nLocMax, float threshold, float minDistBtwLocMax, cv::Mat &locations);
    friend void onMouse(int event, int x, int y, int, void* data);
};

void onMouse(int event, int x, int y, int, void* data);
