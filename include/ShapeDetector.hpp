#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

#define CIRCLE_DETECTION_PARAM1 40
#define CIRCLE_DETECTION_PARAM2 10
#define MIN_RADIUS 30
#define MAX_RADIUS 600
#define OCTAGON_APPROXIMATION_PARAM 0.02
#define OCTAGON_CIRCULARITY_THRESHOLD 0.65

using namespace std;

/**
 * @brief This class provides functionality to detect red and blue colors in a
 * given image.
 */
class ShapeDetector {
   public:
    /**
     * @brief Removes small components from the image based on their area.
     *
     * @param img The input image.
     * @param minComponentArea The minimum area of the components to keep.
     * @param morphSize The size of the structuring element used for
     * morphological operations.
     * @return cv::Mat The image after removing small components.
     */
    static cv::Mat removeSmallComponents(const cv::Mat& img,
                                         double minComponentArea = 200.0,
                                         int morphSize = 4);
    /**
     * @brief Detects circles in the input image.
     *
     * @param img The input image.
     * @return std::vector<std::pair<cv::Vec3f, cv::Point2f>> A vector of pairs,
     * each consisting of a circle and its centroid.
     */
    static vector<pair<cv::Vec3f, cv::Point2f>> detectCircles(
        const cv::Mat& img);

    /**
     * @brief Detects octagons in the input image.
     *
     * @param img The input image.
     * @param minPerimeter The minimum perimeter of the octagons to detect.
     * @return std::vector<std::vector<cv::Point>> A vector of detected
     * octagons, each represented by a vector of its vertices.
     */
    static vector<vector<cv::Point>> detectOctagons(const cv::Mat& img,
                                                    double minPerimeter = 50.0);
    /**
     * @brief Detects squares in the input image.
     *
     * @param img The input image.
     * @return std::vector<std::pair<std::vector<cv::Point>, cv::Point2f>> A
     * vector of pairs, each consisting of a square and its centroid.
     */
    static vector<pair<vector<cv::Point>, cv::Point2f>> detectSquares(
        const cv::Mat& img);
};
