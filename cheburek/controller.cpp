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

    mirror1_tilt = MIRROR_1_TILT;
    mirror2_tilt = MIRROR_2_TILT;

    if(IMAGE_ROTATION != 0)
    {
        rotate(image, image, IMAGE_ROTATION);
    }
    
    reset_drawing();

    info = "Mark first mirror";

    namedWindow(MAIN_WINDOW, WINDOW_NORMAL);
    setMouseCallback(MAIN_WINDOW, &onMouse, this);

    while(draw_ui());

    destroyAllWindows();
}

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

        if(!model_completed)
        {
            std::vector<Point> p_xy;
            std::vector<Point> p_xz_up;
            std::vector<Point> p_xz_down;
            std::vector<Point3i> spheres;

            for(auto &p : maxima)
            {
                // SETUP 3 PLANES
                if(mouse.contains(p))
                {
                    p.x -= mouse.x;
                    p.y -= mouse.y;
                    p.y = mouse.height - p.y;

                    p_xy.emplace_back(p);
                }
                else if(mirror1.contains(p)) // Upper
                {
                    p.x -= mirror1.x;
                    p.y -= mirror1.y;
                    p.y = mirror1.height - p.y;

                    p.y = p.y * tan(mirror1_tilt * CV_PI / 180.);
                    p_xz_up.emplace_back(p);
                }
                else if(mirror2.contains(p)) // Lower
                {
                    p.x -= mirror2.x;
                    p.y -= mirror2.y;

                    p.y = p.y * tan(mirror2_tilt * CV_PI / 180.);
                    p_xz_down.emplace_back(p);
                }

                // SEARCH FOR INTERSECTIONS WITHIN RADIUS
                // We'll work with the points on the flat plane
                // #TODO: Quadratic complexity, might optimize later

                for(auto &p : p_xy)
                {
                    // #TODO merge ?
                    for(auto &u : p_xz_up)
                    {
                        if(abs(u.x - p.x) < CYLINDER_RADIUS * 2)
                        {
                            spheres.emplace_back((u.x + p.x)/2, p.y, u.y);
                        }
                    }
                    for(auto &l : p_xz_down)
                    {
                        if(abs(l.x - p.x) < CYLINDER_RADIUS * 2)
                        {
                            spheres.emplace_back((l.x + p.x)/2, p.y, l.y);
                        }
                    }
                }

                std::cout << "Found spheres: " << std::endl;
                for(auto &p : spheres)
                {
                    std::cout << p << std::endl;
                }
            }

            model_completed = true;
        }

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
            control->info = "Mark 2nd mirror";
        }
        else if(control->mirror2 == EMPTY_RECT)
        {
            control->mirror2 = Rect(Point{control->ix, control->iy}, Point{x, y});
            control->info = "Mark mouse";
        }
        else if(control->mouse == EMPTY_RECT)
        {
            control->mouse = Rect(Point{control->ix, control->iy}, Point{x, y});
            control->info = "Ready";
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
    model_completed = false;

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
