#include <iostream>
#include <opencv2/opencv.hpp>

std::pair<std::vector<cv::Rect>, cv::Mat> detectRedRegions(
    const cv::Mat& image) {
    std::vector<cv::Rect> redRegions;

    // Convert the image to the HSV color space
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    // Threshold the HSV image for only red colors
    cv::Mat mask1, mask2;
    cv::inRange(hsv, cv::Scalar(0, 140, 90), cv::Scalar(6, 255, 255), mask1);
    cv::inRange(hsv, cv::Scalar(164, 140, 90), cv::Scalar(180, 255, 255),
                mask2);

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

std::pair<std::vector<cv::Vec3f>, cv::Mat> detectBlueCircles(
    const cv::Mat& image) {
    std::vector<cv::Vec3f> circles;

    // Convert the image to the HSV color space
    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    // Threshold the HSV image for only blue colors
    cv::Mat mask;
    cv::inRange(hsv, cv::Scalar(100, 50, 50), cv::Scalar(140, 255, 255), mask);

    // Perform morphological operations
    cv::Mat element =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
    cv::erode(mask, mask, element);
    cv::dilate(mask, mask, element);

    // Apply Hough Circle Transform to find circles
    cv::HoughCircles(mask, circles, cv::HOUGH_GRADIENT, 1,
                     mask.rows / 8,  // Change this value as needed
                     100, 30,        // Change the two thresholds as needed
                     0, 0);          // Change the min and max radius as needed

    return std::make_pair(circles, mask);
}

std::map<std::string, cv::Mat> loadTemplates() {
    std::map<std::string, cv::Mat> templates;
    std::string signs[] = {"ArrowLeft", "ArrowRight", "Car",
                           "Forbidden", "Highway",    "Stop"};
    for (const auto& sign : signs) {
        cv::Mat templateImage =
            cv::imread("../sinais/" + sign + ".jpg", cv::IMREAD_GRAYSCALE);

        std::cout << "load template for: " << sign << std::endl;

        if (templateImage.empty()) {
            std::cout << "Failed to load template for: " << sign << std::endl;
            continue;
        }

        cv::Mat mask = templateImage.clone();
        cv::threshold(
            mask, mask, 1, 255,
            cv::THRESH_BINARY);  // Any value greater than 1 becomes 255
        templates[sign] = templateImage;
        templates[sign + "_mask"] =
            mask;  // Store the mask with a key postfixed with "_mask"
    }
    return templates;
}

std::string matchTemplateWithSigns(const cv::Mat& signImage,
                                   std::map<std::string, cv::Mat>& templates) {
    std::string bestMatch;
    double maxMatch = 0;
    for (const auto& temp : templates) {
        // Skip the masks in the templates map
        if (temp.first.find("_mask") != std::string::npos) {
            continue;
        }
        cv::Mat result;
        cv::Mat mask =
            templates[temp.first + "_mask"];  // Get the corresponding mask
        cv::matchTemplate(signImage, temp.second, result, cv::TM_CCORR_NORMED,
                          mask);
        double minVal, maxVal;
        cv::minMaxLoc(result, &minVal, &maxVal);

        std::cout << "Match for " << temp.first << ": " << maxVal << std::endl;

        if (maxVal > maxMatch) {
            maxMatch = maxVal;
            bestMatch = temp.first;
        }
    }
    if (maxMatch > 0.75) {  // 0.75 is a threshold, you can adjust it according
        return bestMatch;
    } else {
        return "";
    }
}

std::string classifyTrafficSign(const cv::Mat& signImage,
                                std::map<std::string, cv::Mat>& templates) {
    cv::Mat signImageGray = signImage.clone();
    cv::cvtColor(signImageGray, signImageGray, cv::COLOR_BGR2GRAY);
    return matchTemplateWithSigns(signImageGray, templates);
}

int main() {
    std::cout << "Starting the program..." << std::endl;

    cv::VideoCapture cap(0);

    if (!cap.isOpened()) {
        std::cerr << "Error: Video capture is not opened." << std::endl;
        return -1;
    }

    std::cout << "Loading templates..." << std::endl;
    std::map<std::string, cv::Mat> templates = loadTemplates();

    cv::Mat frame;

    while (true) {
        cap >> frame;

        auto [redRegions, redMask] = detectRedRegions(frame);
        auto [blueCircles, blueMask] = detectBlueCircles(frame);

        cv::Mat combinedMask = redMask | blueMask;
        cv::cvtColor(combinedMask, combinedMask, cv::COLOR_GRAY2BGR);

        for (const cv::Rect& region : redRegions) {
            cv::Mat regionImage = frame(region);

            if (regionImage.rows >= 300 && regionImage.cols >= 400) {
                std::string label = classifyTrafficSign(regionImage, templates);

                if (!label.empty()) {
                    std::cout << "Red Match Found: " << label << " at "
                              << region.x << "," << region.y << std::endl;
                    cv::rectangle(frame, region, cv::Scalar(0, 255, 0));
                    cv::putText(frame,
                                label + " at " + std::to_string(region.x) +
                                    "," + std::to_string(region.y),
                                region.tl(), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                                cv::Scalar(255, 255, 255));
                }
            }
        }

        for (const cv::Vec3f& circle : blueCircles) {
            cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
            int radius = cvRound(circle[2]);
            cv::circle(frame, center, radius, cv::Scalar(255, 0, 0), 3, 8, 0);

            cv::Mat regionImage = frame(cv::Rect(
                center.x - radius, center.y - radius, 2 * radius, 2 * radius));

            if (!regionImage.empty() && regionImage.rows >= 100 &&
                regionImage.cols >= 100) {
                std::string label = classifyTrafficSign(regionImage, templates);

                if (!label.empty()) {
                    std::cout << "Blue Match Found: " << label << " at "
                              << center.x << "," << center.y << std::endl;
                    cv::putText(frame,
                                label + " at " + std::to_string(center.x) +
                                    "," + std::to_string(center.y),
                                center, cv::FONT_HERSHEY_SIMPLEX, 1.0,
                                cv::Scalar(255, 255, 255));
                }
            }
        }

        cv::imshow("Analysis", frame);
        cv::imshow("Mask", combinedMask);

        int key = cv::waitKey(30);
        if (key == 'x' || key == 'X') {
            std::cout << "Exiting program..." << std::endl;
            break;
        }
    }

    cv::destroyAllWindows();

    return 0;
}
