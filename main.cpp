#include <iostream>
#include <opencv2/opencv.hpp>
#include <ostream>

#define RED_LOWER_BOUND1 cv::Scalar(0, 130, 80)
#define RED_UPPER_BOUND1 cv::Scalar(10, 255, 255)
#define RED_LOWER_BOUND2 cv::Scalar(165, 130, 80)
#define RED_UPPER_BOUND2 cv::Scalar(180, 255, 255)

#define BLUE_LOWER_BOUND cv::Scalar(104, 110, 80)
#define BLUE_UPPER_BOUND cv::Scalar(124, 255, 255)

#define CIRCLE_DETECTION_PARAM1 30
#define CIRCLE_DETECTION_PARAM2 20
#define MIN_RADIUS 20
#define MAX_RADIUS 600
#define OCTAGON_APPROXIMATION_PARAM 0.02
#define OCTAGON_CIRCULARITY_THRESHOLD 0.65

class ColorDetector {
   public:
    static cv::Mat detectRed(const cv::Mat& img) {
        cv::Mat hsv;
        cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

        cv::Mat mask1, mask2;
        cv::inRange(hsv, RED_LOWER_BOUND1, RED_UPPER_BOUND1, mask1);
        cv::inRange(hsv, RED_LOWER_BOUND2, RED_UPPER_BOUND2, mask2);
        cv::Mat mask = mask1 | mask2;

        // Add noise reduction
        cv::Mat kernel =
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1),
                         2);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1),
                         2);

        return mask;
    }

    static cv::Mat detectBlue(const cv::Mat& img) {
        cv::Mat hsv;
        cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

        cv::Mat mask;
        cv::inRange(hsv, BLUE_LOWER_BOUND, BLUE_UPPER_BOUND, mask);

        // Add noise reduction
        cv::Mat kernel =
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel, cv::Point(-1, -1),
                         2);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1),
                         2);

        return mask;
    }
};

class ShapeDetector {
   public:
    static std::vector<cv::Vec3f> detectCircles(const cv::Mat& img) {
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(img, circles, cv::HOUGH_GRADIENT, 1, img.rows / 8,
                         CIRCLE_DETECTION_PARAM1, CIRCLE_DETECTION_PARAM2,
                         MIN_RADIUS, MAX_RADIUS);

        std::vector<cv::Vec3f> filtered_circles;
        for (const auto& circle : circles) {
            cv::Mat circleMask = cv::Mat::zeros(img.rows, img.cols, CV_8U);
            cv::circle(circleMask, cv::Point(circle[0], circle[1]), circle[2],
                       cv::Scalar(255), -1);

            double expectedArea = CV_PI * circle[2] * circle[2];
            double actualArea = cv::countNonZero(img & circleMask);

            if (actualArea / expectedArea > 0.4) {
                filtered_circles.push_back(circle);
            }
        }

        return filtered_circles;
    }

    static std::vector<std::vector<cv::Point>> detectOctagons(
        const cv::Mat& img) {
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(img, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point>> octagons;
        for (const auto& contour : contours) {
            std::vector<cv::Point> approx;
            cv::approxPolyDP(
                contour, approx,
                cv::arcLength(contour, true) * OCTAGON_APPROXIMATION_PARAM,
                true);

            if (approx.size() == 8) {
                double maxEdgeLength = 0.0;
                double minEdgeLength = std::numeric_limits<double>::max();
                for (int i = 0; i < approx.size(); i++) {
                    cv::Point diff =
                        approx[(i + 1) % approx.size()] - approx[i];
                    double edgeLength =
                        cv::sqrt(diff.x * diff.x + diff.y * diff.y);
                    minEdgeLength = std::min(minEdgeLength, edgeLength);
                    maxEdgeLength = std::max(maxEdgeLength, edgeLength);
                }
                double edgeLengthRatio = minEdgeLength / maxEdgeLength;
                if (edgeLengthRatio < 0.8) {  // adjust this threshold as needed
                    continue;  // discard the contour if the edge lengths vary
                               // too much
                }

                octagons.push_back(approx);
            }
        }

        return octagons;
    }
};

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        return -1;
    }

    const double overlapThreshold = 0.1;

    cv::Mat frame;
    while (cap.read(frame)) {
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // Split the image into H, S, and V channels
        std::vector<cv::Mat> hsv_channels;
        cv::split(hsv, hsv_channels);
        // Apply histogram equalization to the V channel
        cv::equalizeHist(hsv_channels[2], hsv_channels[2]);
        // Merge the H, S, and V channels back into a single image
        cv::merge(hsv_channels, hsv);
        // Convert back to BGR color space for color detection
        cv::cvtColor(hsv, frame, cv::COLOR_HSV2BGR);

        cv::Mat redMask = ColorDetector::detectRed(frame);
        cv::Mat blueMask = ColorDetector::detectBlue(frame);
        cv::Mat colorMask = redMask | blueMask;

        std::vector<cv::Vec3f> circles =
            ShapeDetector::detectCircles(colorMask);
        std::vector<std::vector<cv::Point>> octagons =
            ShapeDetector::detectOctagons(redMask);

        for (const auto& circle : circles) {
            if (circle[2] < 20 || circle[2] > 500) continue;

            cv::Rect circleRect(circle[0] - circle[2], circle[1] - circle[2],
                                2 * circle[2], 2 * circle[2]);
            for (const auto& octagon : octagons) {
                cv::Rect octagonRect = cv::boundingRect(octagon);
                double intersectionArea = (circleRect & octagonRect).area();
                double unionArea =
                    circleRect.area() + octagonRect.area() - intersectionArea;
                double overlap = intersectionArea / unionArea;
                if (overlap > overlapThreshold) {
                    continue;  // skip this circle if it overlaps too much with
                               // an octagon
                }
            }

            cv::circle(frame, cv::Point(circle[0], circle[1]), circle[2],
                       cv::Scalar(0, 255, 0), 3, cv::LINE_AA);
            cv::putText(frame, "Circle", cv::Point(circle[0], circle[1]),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(255, 255, 255), 2);
        }

        for (const auto& octagon : octagons) {
            cv::polylines(frame, octagon, true, cv::Scalar(0, 255, 0), 3,
                          cv::LINE_AA);

            // Calculate the center of the contour
            cv::Moments M = cv::moments(octagon);
            int cX = static_cast<int>(M.m10 / M.m00);
            int cY = static_cast<int>(M.m01 / M.m00);

            cv::putText(frame, "Octagon", cv::Point(cX, cY),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(255, 255, 255), 2);
        }

        cv::imshow("frame", frame);
        cv::imshow("color", colorMask);

        int key = cv::waitKey(30);
        if (key == 'x' || key == 'X') {
            std::cout << "Exiting program..." << std::endl;
            break;
        }
    }

    cv::destroyAllWindows();

    return 0;
}
