#include "controller.hpp"
#include "config.hpp"
#include "opencv2/imgcodecs.hpp"
#include <cmath>
using namespace cv;

void Controller::start(std::string img_name)
{
    image = imread(img_name);
    if(!image.data)
    {
        std::cerr << "No data!\n";
        return;
    }
    pixels_per_sm = PIXELS_PER_SM;
    image_rotation_deg = IMAGE_ROTATION;
    
    reset_drawing();

    info = "Mark first mirror";

    namedWindow(MAIN_WINDOW, WINDOW_NORMAL);
    setMouseCallback(MAIN_WINDOW, &onMouse, this);

    while(draw_ui());

    destroyAllWindows();
}

bool kek = false;

bool Controller::draw_ui()
{
    auto key = waitKey(10);
    if(key == 'r' || key == 'R')
        reset_drawing();

    auto ui = image.clone();
#if INVERT
    bitwise_not(ui, ui);
#endif
#if BLUR
    GaussianBlur(ui, ui, {0,0}, 4, 4);
#endif
    Mat locs = ui.clone();
    cvtColor(locs, locs, ColorConversionCodes::COLOR_BGR2GRAY);
    if(mirrors_ready())
    {
        auto maxima = imregionalmax(locs, NUM_MAX, 60., 40);
        status = "Found " + std::to_string(maxima.size()) + " points.";

        mark_maxima(ui, maxima);        
    }

    addWeighted(ui, 1, drawable_layer, 1, 0, ui);
#if DRAW_TEXT
    putText(ui, info + " " + status, {0, 60}, FONT_HERSHEY_SIMPLEX, TEXT_SIZE, {255,255,255}, 2);
#endif
    imshow(MAIN_WINDOW, ui);

    return key != 'q';
}

inline Controller* get(void* data)
{
    return static_cast<Controller*>(data);
}

void onMouse(int event, int x, int y, int, void* data)
{
    auto *control = static_cast<Controller*>(data);

    if(control->mirrors_ready())
    {
        return;
    }

    if(event == MouseEventTypes::EVENT_LBUTTONDOWN)
    {
        control->is_drawing = true;
        control->ix = x;
        control->iy = y;
    }
    else if(event == MouseEventTypes::EVENT_LBUTTONUP)
    {
        control->is_drawing = false;
        rectangle(control->drawable_layer, {control->ix, control->iy}, {x,y}, RECT_COLOR);

        if(control->mirror1 == EMPTY_RECT)
        {
            control->mirror1 = Rect(Point{control->ix, control->iy}, Point{x, y});
            control->info = "Mirror 1 set";
        }
        else if(control->mirror2 == EMPTY_RECT)
        {
            control->mirror2 = Rect(Point{control->ix, control->iy}, Point{x, y});
            control->info = "Mirror 2 set";
        }
        else if(control->mouse == EMPTY_RECT)
        {
            control->mouse = Rect(Point{control->ix, control->iy}, Point{x, y});
            control->info = "Mouse set. Ready";
        }
    }
}

std::vector<Point> Controller::imregionalmax(Mat &input, int nLocMax, float threshold, float minDistBtwLocMax)
{
    std::vector<Point> locations;
    Mat scratch = input.clone();
    Mat mask = scratch.clone();
    rectangle(mask, Rect{0,0, mask.size[1]-1, mask.size[0]-1}, {0,0,0,255}, -1);
    mask = mask.reshape(1);
    rectangle(mask, mirror1, WHITE, -1);
    rectangle(mask, mirror2, WHITE, -1);
    rectangle(mask, mouse, WHITE, -1);
    
    for (int i = 0; i < nLocMax; i++) {
        Point location;
        double maxVal;
        minMaxLoc(scratch, NULL, &maxVal, NULL, &location, mask);
        if (maxVal > threshold) {
            locations.emplace_back(location);
            circle(scratch, location, minDistBtwLocMax, {0,0,0}, -1);
        } else {
            break;
        }
    }
    return locations;
}

void Controller::mark_maxima(Mat &loc_img, const std::vector<Point> &points)
{
    for(auto &point : points)
    {
        circle(loc_img, point, POINT_SIZE, POINT_COLOR, -2);
    }
}

void Controller::reset_drawing()
{
    drawable_layer = Mat(Size(image.cols, image.rows), image.type());
    for(int y = 0; y < drawable_layer.rows; y++) {
        for(int x = 0; x < drawable_layer.cols; x++) {
            for(int c = 0; c < drawable_layer.channels(); c++) {
                drawable_layer.at<Vec3b>(y,x)[c] =
                    0;
                }
            }
    }

    mirror2 = mirror1 = mouse = EMPTY_RECT;

    info = "Mark first mirror";
}

bool Controller::mirrors_ready()
{
    return mirror1 != EMPTY_RECT && mirror2 != EMPTY_RECT && mouse != EMPTY_RECT;
}

void Controller::adjust_brightness_contrast(cv::Mat &input, float brightness, float contrast)
{
    for(int y = 0; y < image.rows; y++) {
        for(int x = 0; x < image.cols; x++) {
            for(int c = 0; c < image.channels(); c++) {
                image.at<Vec3b>(y,x)[c] =
                    saturate_cast<uchar>(contrast*image.at<Vec3b>(y,x)[c] + brightness);
                }
            }
    }
}
