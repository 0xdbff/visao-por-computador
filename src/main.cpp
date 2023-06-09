#include "Analyser.hpp"

/**
 * Entry point of the application. It opens a video capture from the default
 * camera, processes the video through the `Analyser` class, and returns a
 * status code.
 */
int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        return -1;
    }

    Analyser::processVideo(cap);

    return 0;
}
