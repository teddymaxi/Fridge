#include "controller.hpp"
#include <cmath>
using namespace cv;

void Controller::start()
{
    image = imread("test.png");
    if(!image.data)
    {
        std::cerr << "No data!\n";
        return;
    }
    drawable_layer = Mat(Size(image.cols, image.rows), image.type());

    namedWindow("Cheburek", WINDOW_AUTOSIZE);
    setMouseCallback("Cheburek", &onMouse, this);

    while(waitKey(10) != 'q')
        draw_ui();

    destroyAllWindows();
}

void Controller::draw_ui()
{
    auto ui = image.clone();
    //bitwise_not(ui, ui);
    //GaussianBlur(ui, ui, {0,0}, 4, 4);
    Mat locs = ui.clone();
    cvtColor(locs, locs, ColorConversionCodes::COLOR_BGR2GRAY);
    imregionalmax(locs, 9, 60., 40, ui);

    addWeighted(ui, 1, drawable_layer, 1, 0, ui);
    putText(ui, "Status" + std::to_string(rand()), {0, 60}, FONT_HERSHEY_SIMPLEX, 2.0, {255,255,255}, 2);

    imshow("Cheburek", ui);
}

void onMouse(int event, int x, int y, int, void* data)
{
    auto *control = static_cast<Controller*>(data);

    if(event == MouseEventTypes::EVENT_LBUTTONDOWN)
    {
        control->is_drawing = true;
        control->ix = x;
        control->iy = y;
    }
    else if(event == MouseEventTypes::EVENT_LBUTTONUP)
    {
        control->is_drawing = false;
        if(control->mode)
        {
            rectangle(control->drawable_layer, {control->ix, control->iy}, {x,y}, {0,255,0, 0});
        }
        else
        {
            circle(control->drawable_layer, {x,y}, 5, {0,0,255});
        }
        //std::cout << x << ", " << y << std::endl;
    }
}

int Controller::imregionalmax(Mat &input, int nLocMax, float threshold, float minDistBtwLocMax, Mat &locations)
{
    Mat scratch = input.clone();
    int nFoundLocMax = 0;
    for (int i = 0; i < nLocMax; i++) {
        Point location;
        double maxVal;
        minMaxLoc(scratch, NULL, &maxVal, NULL, &location);
        if (maxVal > threshold) {
            //std::cout << maxVal << std::endl;
            nFoundLocMax += 1;
            int row = location.y;
            int col = location.x;
            circle(locations, {col, row}, 50, {255,255,255,255}, 2);
            circle(scratch, location, minDistBtwLocMax, {0,0,0}, -1);
        } else {
            break;
        }
    }
    return nFoundLocMax;
}
