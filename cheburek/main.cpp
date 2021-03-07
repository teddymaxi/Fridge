#include "controller.hpp"
#include "config.hpp"

int main(int argc, char** argv)
{
    Controller controller;
    if(argc > 1)
    {
        controller.start(std::string(argv[1]));
    }
    else
    {
        controller.start();
    }
    

    return 0;
}
