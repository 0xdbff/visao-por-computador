#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#define RED_LOWER_BOUND1 cv::Scalar(0, 130, 80)
#define RED_UPPER_BOUND1 cv::Scalar(10, 255, 255)
#define RED_LOWER_BOUND2 cv::Scalar(165, 130, 80)
#define RED_UPPER_BOUND2 cv::Scalar(180, 255, 255)

#define BLUE_LOWER_BOUND cv::Scalar(104, 110, 80)
#define BLUE_UPPER_BOUND cv::Scalar(124, 255, 255)

using namespace std;

/**
 * @brief This class provides functionality to detect red and blue colors in a
 * given image.
 */
class ColorDetector {
   public:
    /**
     * @brief Detects the red color in the input image.
     *
     * @param img The input image.
     * @return cv::Mat The mask of the detected red color.
     */
    static cv::Mat detectRed(const cv::Mat& img);
    /**
     * @brief Detects the blue color in the input image.
     *
     * @param img The input image.
     * @return cv::Mat The mask of the detected blue color.
     */
    static cv::Mat detectBlue(const cv::Mat& img);
};
