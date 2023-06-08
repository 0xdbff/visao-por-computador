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
#define OCTAGON_APPROXIMATION_PARAM 0.04
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
    static std::vector<std::pair<cv::Vec3f, cv::Point2f>> detectCircles(
        const cv::Mat& img) {
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(img, circles, cv::HOUGH_GRADIENT, 1, img.rows / 8,
                         CIRCLE_DETECTION_PARAM1, CIRCLE_DETECTION_PARAM2,
                         MIN_RADIUS, MAX_RADIUS);

        std::vector<std::pair<cv::Vec3f, cv::Point2f>> result;

        for (const auto& circle : circles) {
            cv::Mat circleMask = cv::Mat::zeros(img.rows, img.cols, CV_8U);
            cv::circle(circleMask, cv::Point(circle[0], circle[1]), circle[2],
                       cv::Scalar(255), -1);

            cv::Mat maskedImg;
            img.copyTo(maskedImg,
                       circleMask);  // mask the binary image with the circle

            double expectedArea = CV_PI * circle[2] * circle[2];
            double actualArea =
                cv::countNonZero(maskedImg);  // count the number of white
                                              // pixels inside the circle

            if (actualArea / expectedArea > 0.48) {
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
                // Do not match if the contours are not aprox equal in lenght.
                if (edgeLengthRatio < 0.7) {
                    continue;
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

        // Send blue mask to detect circles, and mass center
        std::vector<std::pair<cv::Vec3f, cv::Point2f>> blueCircles =
            ShapeDetector::detectCircles(blueMask);
        std::vector<std::pair<cv::Vec3f, cv::Point2f>> redCircles =
            ShapeDetector::detectCircles(redMask);
        std::vector<std::vector<cv::Point>> octagons =
            ShapeDetector::detectOctagons(redMask);

        for (const auto& bcircle : blueCircles) {
            cv::Vec3f circle = bcircle.first;
            cv::Point2f center_of_mass = bcircle.second;

            if (circle[2] < 10 || circle[2] > 500) {
                continue;
            }

            cv::circle(frame,
                       cv::Point(static_cast<int>(circle[0]),
                                 static_cast<int>(circle[1])),
                       static_cast<int>(circle[2]), cv::Scalar(0, 255, 0), 3,
                       cv::LINE_AA);

            double delta =
                cv::Point2f(circle[0], circle[1]).x - center_of_mass.x;

            std::string circle_center = "(" + std::to_string((int)circle[0]) +
                                        ", " + std::to_string((int)circle[1]) +
                                        ")";
            cv::putText(frame, circle_center,
                        cv::Point(static_cast<int>(circle[0] - 40),
                                  static_cast<int>(circle[1] + 20)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0x00, 0x00, 0x00), 2);

            if (center_of_mass.x > circle[0]) {
                cv::putText(frame, "Turn Left",
                            cv::Point(static_cast<int>(circle[0] - 40),
                                      static_cast<int>(circle[1] - 20)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5,
                            cv::Scalar(40, 255, 50), 2);

                std::cout << "TurnRight\tDetected blue circle: center: ("
                          << circle[0] << ", " << circle[1]
                          << "), center of Mass: (" << center_of_mass.x << ", "
                          << center_of_mass.y << "), delta: " << delta
                          << std::endl;
            } else {
                cv::putText(frame, "Turn Right",
                            cv::Point(static_cast<int>(circle[0] - 40),
                                      static_cast<int>(circle[1] - 20)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5,
                            cv::Scalar(40, 255, 50), 2);

                std::cout << "TurnLeft\tDetected blue circle: center: ("
                          << circle[0] << ", " << circle[1]
                          << "), center of Mass: (" << center_of_mass.x << ", "
                          << center_of_mass.y << "), delta: " << delta
                          << std::endl;
            }
        }

        for (const auto& rcircle : redCircles) {
            cv::Vec3f circle = rcircle.first;

            if (circle[2] < 10 || circle[2] > 500) {
                continue;
            }

            cv::circle(frame,
                       cv::Point(static_cast<int>(circle[0]),
                                 static_cast<int>(circle[1])),
                       static_cast<int>(circle[2]), cv::Scalar(0, 255, 0), 3,
                       cv::LINE_AA);

            std::cout << "STOP\t\tDetected red circle: center: (" << circle[0]
                      << ", " << circle[1] << std::endl;

            std::string circle_center = "(" + std::to_string((int)circle[0]) +
                                        ", " + std::to_string((int)circle[1]) +
                                        ")";
            cv::putText(frame, circle_center,
                        cv::Point(static_cast<int>(circle[0] - 40),
                                  static_cast<int>(circle[1] + 20)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0x00, 0x00, 0x00), 2);

            cv::putText(frame, "Stop",
                        cv::Point(static_cast<int>(circle[0] - 40),
                                  static_cast<int>(circle[1] - 20)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(40, 255, 50),
                        2);
        }

        for (const auto& octagon : octagons) {
            cv::polylines(frame, octagon, true, cv::Scalar(0, 255, 0), 3,
                          cv::LINE_AA);

            // Calculate the center of the contour
            cv::Moments M = cv::moments(octagon);
            int cX = static_cast<int>(M.m10 / M.m00);
            int cY = static_cast<int>(M.m01 / M.m00);

            std::string octagon_center =
                "(" + std::to_string(cX) + ", " + std::to_string(cY) + ")";
            cv::putText(frame, octagon_center, cv::Point(cX - 40, cY + 20),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(255, 255, 255), 2);

            cv::putText(frame, "Stop", cv::Point(cX, cY),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(255, 255, 255), 2);
        }

        cv::imshow("binary", colorMask);
        cv::imshow("Analyser", frame);

        int key = cv::waitKey(30);
        if (key == 'x' || key == 'X') {
            std::cout << "Exiting program..." << std::endl;
            break;
        }
    }

    cv::destroyAllWindows();

    return 0;
}
