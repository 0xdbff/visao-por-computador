#include <opencv2/opencv.hpp>

std::vector<cv::Rect> detectTrafficSigns(const cv::Mat& image) {
    std::vector<cv::Rect> signs;

    // Convert the image to grayscale
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    // Threshold the image
    cv::Mat thresh;
    cv::threshold(gray, thresh, 100, 255, cv::THRESH_BINARY);

    // Find contours in the thresholded image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    // Filter contours based on size and shape
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double aspectRatio = static_cast<double>(rect.width) / rect.height;

        if (cv::contourArea(contour) > 1000 && aspectRatio > 0.7 &&
            aspectRatio < 1.3) {
            signs.push_back(rect);
        }
    }

    return signs;
}
std::string classifyTrafficSign(const cv::Mat& signImage) {
    // Convert the image to the HSV color space
    cv::Mat hsv;
    cv::cvtColor(signImage, hsv, cv::COLOR_BGR2HSV);

    // Threshold the HSV image to get only red colors
    cv::Mat mask1, mask2;
    cv::inRange(hsv, cv::Scalar(0, 70, 50), cv::Scalar(10, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(170, 70, 50), cv::Scalar(180, 255, 255), mask2);

    // Combine the above two masks to get our final mask
    cv::Mat mask = mask1 | mask2;

    // Find contours in the mask
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    // Filter contours based on size and shape
    for (const auto& contour : contours) {
        // Approximate the contour to a polygon
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, cv::arcLength(contour, true) * 0.02,
                         true);

        // If the polygon has 8 sides, it might be a stop sign
        if (approx.size() == 8) {
            return "Stop";
        }
    }

    // If no stop sign is detected, return an empty string
    return "Unrecognized";
}


int main() {
    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        return -1;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame;

        // // Detect traffic signs
        std::vector<cv::Rect> signs = detectTrafficSigns(frame);

        // Classify each sign
        for (const cv::Rect& sign : signs) {
            cv::Mat signImage = frame(sign);
            std::string label = classifyTrafficSign(signImage);

            // Draw bounding box and label
            cv::rectangle(frame, sign, cv::Scalar(0, 255, 0));
            cv::putText(frame, label, sign.tl(), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                        cv::Scalar(255, 255, 255));
        }

        // Display the frame
        cv::imshow("Analysis", frame);

        if (cv::waitKey(30) >= 0) {
            break;
        }
    }
}
