#include <iostream>
#include <vector>
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
    int mirror1_tilt = 45; // DEG
    cv::Rect mirror2 = EMPTY_RECT;
    int mirror2_tilt = 45; // DEG
    cv::Rect mouse = EMPTY_RECT;

    bool model_completed = false;

public:
    bool draw_ui();
    void reset_drawing();
    bool mirrors_ready();
    void start(std::string img_name = "cheburek.jpg");
    std::vector<cv::Point> imregionalmax(cv::Mat &input, int nLocMax, float threshold, float thres_max, float minDistBtwLocMax);
    void adjust_brightness_contrast(cv::Mat &input, float brightness, float contrast);
    void mark_maxima(cv::Mat &loc_img, const std::vector<cv::Point> &points);
    friend void onMouse(int event, int x, int y, int, void* data);
};

void onMouse(int event, int x, int y, int, void* data);
