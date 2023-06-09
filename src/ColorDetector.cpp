/**
 * @brief This class provides functionality to detect red and blue colors in a
 * given image.
 */
#include "ColorDetector.hpp"

/**
 * @brief Detects the red color in the input image.
 *
 * @param img The input image.
 * @return cv::Mat The mask of the detected red color.
 */
cv::Mat ColorDetector::detectRed(const cv::Mat& img) {
    cv::Mat hsv;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask1, mask2;
    cv::inRange(hsv, RED_LOWER_BOUND1, RED_UPPER_BOUND1, mask1);
    cv::inRange(hsv, RED_LOWER_BOUND2, RED_UPPER_BOUND2, mask2);
    cv::Mat mask = mask1 | mask2;

    // Add noise reduction
    cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

    return mask;
}

/**
 * @brief Detects the blue color in the input image.
 *
 * @param img The input image.
 * @return cv::Mat The mask of the detected blue color.
 */
cv::Mat ColorDetector::detectBlue(const cv::Mat& img) {
    cv::Mat hsv;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask;
    cv::inRange(hsv, BLUE_LOWER_BOUND, BLUE_UPPER_BOUND, mask);

    // Add noise reduction
    cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

    return mask;
}
