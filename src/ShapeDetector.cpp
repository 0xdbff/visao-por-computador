/**
 * @brief This class provides functionality to detect different shapes in a
 * given image.
 */
#include "ShapeDetector.hpp"

/**
 * @brief Removes small components from the image based on their area.
 *
 * @param img The input image.
 * @param minComponentArea The minimum area of the components to keep.
 * @param morphSize The size of the structuring element used for
 * morphological operations.
 * @return cv::Mat The image after removing small components.
 */
cv::Mat ShapeDetector::removeSmallComponents(const cv::Mat& img,
                                             double minComponentArea,
                                             int morphSize) {
    cv::Mat imgProcessed;

    // Use morphological closing to close small holes in the image
    cv::Mat element = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(2 * morphSize + 1, 2 * morphSize + 1),
        cv::Point(morphSize, morphSize));
    cv::morphologyEx(img, imgProcessed, cv::MORPH_CLOSE, element);

    // Find connected components
    cv::Mat labels;
    int numComponents = cv::connectedComponents(imgProcessed, labels, 4);

    cv::Mat mask = cv::Mat::zeros(img.size(), CV_8UC1);

    // For each component, if its area is large enough, draw it on the mask
    for (int i = 1; i < numComponents; i++) {
        cv::Mat component = (labels == i);

        double area = cv::countNonZero(component);

        // Skip if component area is less than threshold
        if (area < minComponentArea) {
            continue;
        }

        mask = mask | component;
    }

    return mask;
}

/**
 * @brief Detects circles in the input image.
 *
 * @param img The input image.
 * @return std::vector<std::pair<cv::Vec3f, cv::Point2f>> A vector of pairs,
 * each consisting of a circle and its centroid.
 */
vector<pair<cv::Vec3f, cv::Point2f>> ShapeDetector::detectCircles(
    const cv::Mat& img) {
    vector<cv::Vec3f> circles;
    cv::HoughCircles(img, circles, cv::HOUGH_GRADIENT, 1, img.rows / 8,
                     CIRCLE_DETECTION_PARAM1, CIRCLE_DETECTION_PARAM2,
                     MIN_RADIUS, MAX_RADIUS);

    vector<pair<cv::Vec3f, cv::Point2f>> result;

    for (const auto& circle : circles) {
        if (circle[2] < 30 || circle[2] > 500) {
            continue;
        }
        cv::Mat circleMask = cv::Mat::zeros(img.rows, img.cols, CV_8U);
        cv::circle(circleMask, cv::Point(circle[0], circle[1]), circle[2],
                   cv::Scalar(255), -1);

        cv::Mat maskedImg;
        img.copyTo(maskedImg, circleMask);

        double expectedArea = CV_PI * circle[2] * circle[2];
        double actualArea =
            cv::countNonZero(maskedImg);  // count the number of white
                                          // pixels inside the circle

        if (actualArea / expectedArea > 0.6 &&
            actualArea / expectedArea < 0.84) {
            cout << actualArea / expectedArea << endl;
            cv::Moments mu = cv::moments(
                maskedImg,
                false);  // compute the moments of the masked binary image
            cv::Point2f mc =
                cv::Point2f(static_cast<float>(mu.m10 / (mu.m00 + 1e-5)),
                            static_cast<float>(mu.m01 / (mu.m00 + 1e-5)));
            result.emplace_back(circle, mc);
        }
    }

    return result;
}

/**
 * @brief Detects octagons in the input image.
 *
 * @param img The input image.
 * @param minPerimeter The minimum perimeter of the octagons to detect.
 * @return std::vector<std::vector<cv::Point>> A vector of detected
 * octagons, each represented by a vector of its vertices.
 */
vector<vector<cv::Point>> ShapeDetector::detectOctagons(const cv::Mat& img,
                                                        double minPerimeter) {
    vector<vector<cv::Point>> contours;
    cv::findContours(img, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    vector<vector<cv::Point>> octagons;
    for (const auto& contour : contours) {
        double contourLength = cv::arcLength(contour, true);

        // Skip contour if it has less than minPerimeter length
        if (contourLength < minPerimeter) {
            continue;
        }

        vector<cv::Point> approx;
        cv::approxPolyDP(
            contour, approx,
            cv::arcLength(contour, true) * OCTAGON_APPROXIMATION_PARAM, true);

        if (approx.size() == 8) {
            double maxEdgeLength = 0.0;
            double minEdgeLength = numeric_limits<double>::max();
            for (int i = 0; i < approx.size(); i++) {
                cv::Point diff = approx[(i + 1) % approx.size()] - approx[i];
                double edgeLength = cv::sqrt(diff.x * diff.x + diff.y * diff.y);
                minEdgeLength = min(minEdgeLength, edgeLength);
                maxEdgeLength = max(maxEdgeLength, edgeLength);
            }
            double edgeLengthRatio = minEdgeLength / maxEdgeLength;
            // Do not match if the contours are not approx equal in length.
            if (edgeLengthRatio < 0.8) {
                continue;
            }

            octagons.push_back(approx);
        }
    }

    return octagons;
}

/**
 * @brief Detects squares in the input image.
 *
 * @param img The input image.
 * @return std::vector<std::pair<std::vector<cv::Point>, cv::Point2f>> A
 * vector of pairs, each consisting of a square and its centroid.
 */
vector<pair<vector<cv::Point>, cv::Point2f>> ShapeDetector::detectSquares(
    const cv::Mat& img) {
    vector<vector<cv::Point>> contours;
    cv::findContours(img.clone(), contours, cv::RETR_TREE,
                     cv::CHAIN_APPROX_SIMPLE);

    vector<pair<vector<cv::Point>, cv::Point2f>> squares;

    for (const auto& contour : contours) {
        vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, cv::arcLength(contour, true) * 0.02,
                         true);

        if (approx.size() == 4 && fabs(cv::contourArea(approx)) > 1000) {
            array<double, 4> sides = {cv::norm(approx[1] - approx[0]),
                                      cv::norm(approx[2] - approx[1]),
                                      cv::norm(approx[3] - approx[2]),
                                      cv::norm(approx[0] - approx[3])};

            double max_side = *max_element(sides.begin(), sides.end());
            double min_side = *min_element(sides.begin(), sides.end());

            if (max_side <= 1.2 * min_side) {
                cv::Moments m = cv::moments(contour, false);
                squares.emplace_back(
                    approx,
                    cv::Point2f(static_cast<float>(m.m10 / (m.m00 + 1e-5)),
                                static_cast<float>(m.m01 / (m.m00 + 1e-5))));
            }
        }
    }

    return squares;
}
