#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#include "ColorDetector.hpp"
#include "ShapeDetector.hpp"

using namespace std;

/**
 * Class `Analyser` is responsible for processing video streams and performing
 * color and shape detection. It can identify blue circles, red circles,
 * octagons, and squares in the frames of a video stream.
 */
class Analyser {
   public:
    /**
     * The `processVideo` function reads frames from a video source, performs
     * color and shape detection, and processes each frame accordingly.
     *
     * @param cap - A reference to a cv::VideoCapture object representing the
     * video source.
     */
    static void processVideo(cv::VideoCapture& cap);
};
