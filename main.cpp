#include <opencv2/opencv.hpp>

std::pair<std::vector<cv::Rect>, cv::Mat> detectRedRegions(const cv::Mat& image) {
    std::vector<cv::Rect> redRegions;

    // Convert the image to the HSV color space
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    // Threshold the HSV image for only red colors
    cv::Mat mask1, mask2;
    cv::inRange(hsv, cv::Scalar(0, 140, 90), cv::Scalar(6, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(164, 140, 90), cv::Scalar(180, 255, 255), mask2);

    // Combine the above two masks to get our final mask
    cv::Mat mask = mask1 | mask2;

    // Perform morphological operations
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
    cv::erode(mask, mask, element);
    cv::dilate(mask, mask, element);

    // Find contours in the thresholded image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);

    // Filter contours based on size
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        double area = cv::contourArea(contour);

        if (area > 1000 && area < 50000) {  // Adjust these values as necessary
            redRegions.push_back(rect);
        }
    }

    return std::make_pair(redRegions, mask);
}

std::string classifyTrafficSign(const cv::Mat& signImage) {
    // Convert to grayscale
    cv::Mat gray;
    cv::cvtColor(signImage, gray, cv::COLOR_BGR2GRAY);

    // Blur the image
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);

    // Use Canny edge detection
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    // Find contours in the edge image
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Filter contours based on size and shape
    for (const auto& contour : contours) {
        // Approximate the contour to a polygon
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, cv::arcLength(contour, true) * 0.02, true);

        // Calculate the area and perimeter of the contour
        double area = cv::contourArea(contour);
        double perimeter = cv::arcLength(contour, true);

        // Calculate circularity
        double circularity = 4 * CV_PI * area / (perimeter * perimeter);

        // If the polygon is close to circular, it might be a 'No Entry' or 'Stop' sign
        if (circularity > 0.85) {
            // Check for the presence of a horizontal line for 'No Entry' sign
            std::vector<cv::Vec4i> lines;
            cv::HoughLinesP(edges, lines, 1, CV_PI/180, 50, 50, 10);
            int count_horizontal_lines = 0;
            for (const auto& line : lines) {
                if (abs(line[1] - line[3]) < 20) {
                    count_horizontal_lines++;
                }
            }
            // If more than a threshold number of horizontal lines are found, it's likely a 'No Entry' sign
            if (count_horizontal_lines > 1) { // You may have to define this constant
                return "No Entry";
            }

            // Check for an octagonal shape for 'Stop' sign
            if (approx.size() == 8) {
                return "Stop";
            }
        }
    }
    // If no sign is detected, return an empty string
    return "";
}

int main() {
    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        return -1;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame;

        // Detect red regions
        auto [redRegions, mask] = detectRedRegions(frame);

        cv::imshow("Mask", mask);

        // Classify each region
        for (const cv::Rect& region : redRegions) {
            cv::Mat regionImage = frame(region);
            std::string label = classifyTrafficSign(regionImage);

            // Draw bounding box and label only if a sign is detected
            if (!label.empty()) {
                cv::rectangle(frame, region, cv::Scalar(0, 255, 0));
                cv::putText(frame, label, region.tl(), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255));
            }
        }

        // Display the frame
        cv::imshow("Analysis", frame);

        if (cv::waitKey(30) >= 0) {
            break;
        }
    }

    cv::destroyWindow("Mask");

    return 0;
}
