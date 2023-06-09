#include <algorithm>
#include <cmath>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <utility>
#include <vector>

using namespace std;

/**
 * @brief Structure to represent an image with pixel data and dimensions.
 */
struct Image {
    vector<vector<vector<uint8_t>>> data;  // Image pixel data
    int height, width;  // Image dimensions (height and width)

    /**
     * @brief Constructor to initialize an Image object.
     * @param data The pixel data of the image.
     * @param height The height of the image.
     * @param width The width of the image.
     */
    Image(vector<vector<vector<uint8_t>>> data, int height, int width)
        : data(std::move(data)), height(height), width(width) {}
};

/**
 * @brief Structure to represent a point with x and y coordinates.
 */
struct Point {
    int x, y;  // x and y coordinates of a point
};

/**
 * @brief Structure to represent a circle with center and radius.
 */
struct Circle {
    Point center;   // Center point of the circle
    double radius;  // Radius of the circle
};

/**
 * @brief Convert a cv::Mat object to an Image object.
 *
 * @param mat The cv::Mat object to convert.
 * @return The converted Image object.
 */
Image matToImage(cv::Mat mat) {
    int height = mat.rows;
    int width = mat.cols;
    vector<vector<vector<uint8_t>>> data(
        height, vector<vector<uint8_t>>(width, vector<uint8_t>(3)));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            cv::Vec3b intensity = mat.at<cv::Vec3b>(i, j);
            for (int c = 0; c < 3; c++) {
                data[i][j][c] = intensity.val[c];
            }
        }
    }

    return {data, height, width};
}

/**
 * @brief Convert an Image object to a cv::Mat object.
 *
 * @param img The Image object to convert.
 * @return The converted cv::Mat object.
 */
cv::Mat imageToMat(Image img) {
    cv::Mat mat(img.height, img.width, CV_8UC3);

    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            for (int c = 0; c < 3; c++)
                mat.at<cv::Vec3b>(i, j)[c] = img.data[i][j][c];
        }
    }

    return mat;
}

/**
 * @brief Convert an RGB image to the HSV color space.
 *
 * @param rgb The RGB image to convert.
 * @return The HSV image.
 */
Image convertToHSV(Image rgb) {
    Image hsv = rgb;
    for (int i = 0; i < hsv.height; ++i) {
        for (int j = 0; j < hsv.width; ++j) {
            double r = rgb.data[i][j][0] / 255.0;
            double g = rgb.data[i][j][1] / 255.0;
            double b = rgb.data[i][j][2] / 255.0;

            double max_val = max({r, g, b}), min_val = min({r, g, b});
            double diff = max_val - min_val;

            if (max_val == min_val) {
                hsv.data[i][j][0] = 0;
            } else if (max_val == r) {
                hsv.data[i][j][0] =
                    static_cast<uint8_t>(60 * fmod(((g - b) / diff), 6));
            } else if (max_val == g) {
                hsv.data[i][j][0] =
                    static_cast<uint8_t>(60 * (((b - r) / diff) + 2));
            } else if (max_val == b) {
                hsv.data[i][j][0] =
                    static_cast<uint8_t>(60 * (((r - g) / diff) + 4));
            }

            // Saturation
            hsv.data[i][j][1] =
                (max_val == 0) ? 0
                               : static_cast<uint8_t>((diff / max_val) * 0xff);

            // Value
            hsv.data[i][j][2] = static_cast<uint8_t>(max_val * 0xff);
        }
    }
    return hsv;
}

/**
 * @brief Create a binary mask by thresholding an HSV image.
 *
 * @param hsv The input HSV image.
 * @param lower The lower threshold values for each channel (H, S, V).
 * @param upper The upper threshold values for each channel (H, S, V).
 * @return The binary mask image.
 */
Image inRange(Image hsv, vector<uint8_t> lower, vector<uint8_t> upper) {
    Image mask = hsv;
    for (int i = 0; i < mask.height; ++i) {
        for (int j = 0; j < mask.width; ++j) {
            if (hsv.data[i][j][0] >= lower[0] &&
                hsv.data[i][j][0] <= upper[0] &&
                hsv.data[i][j][1] >= lower[1] &&
                hsv.data[i][j][1] <= upper[1] &&
                hsv.data[i][j][2] >= lower[2] &&
                hsv.data[i][j][2] <= upper[2]) {
                mask.data[i][j] = {0xff, 0xff, 0xff};
            } else {
                mask.data[i][j] = {0, 0, 0};  // black
            }
        }
    }
    return mask;
}

/**
 * @brief Get the segmented image based on the HSV thresholding.
 *
 * @param inputMat The input image in cv::Mat format.
 * @param lower The lower threshold values for each channel (H, S, V).
 * @param upper The upper threshold values for each channel (H, S, V).
 * @return The segmented image in cv::Mat format.
 */
cv::Mat getHSVSegmentedImage(cv::Mat& inputMat, vector<uint8_t>& lower,
                             vector<uint8_t>& upper) {
    Image img = matToImage(inputMat);
    Image hsv = convertToHSV(img);
    Image mask = inRange(hsv, lower, upper);
    return imageToMat(mask);
}

/**
 * @brief Perform erosion operation on an image.
 *
 * @param img The image to apply the operation on.
 * @param kernelSize The size of the kernel to be used for the operation. This
 * must be an odd number.
 * @return The eroded image.
 * @throws std::invalid_argument If the kernel size is not odd.
 */
static inline Image erode(Image img, int kernelSize) {
    if (kernelSize % 2 == 0) {
        throw invalid_argument("Kernel size must be odd!");
    }
    // Create a new image with same size but filled with 255 (white color)
    vector<vector<vector<uint8_t>>> newData(
        img.height,
        vector<vector<uint8_t>>(img.width, vector<uint8_t>(3, 255)));
    Image erodedImage(newData, img.height, img.width);

    // Radius of the kernel
    int radius = kernelSize / 2;

    for (int i = radius; i < img.height - radius; i++) {
        for (int j = radius; j < img.width - radius; j++) {
            uint8_t minVal = 255;

            // Find the minimum pixel value in the neighborhood
            for (int x = -radius; x <= radius; x++) {
                for (int y = -radius; y <= radius; y++) {
                    minVal = min(
                        minVal,
                        img.data[i + x][j + y][0]);  // Assuming grayscale image
                }
            }

            // Set the minimum value to the corresponding pixel in the eroded
            // image
            erodedImage.data[i][j][0] = minVal;
            erodedImage.data[i][j][1] = minVal;
            erodedImage.data[i][j][2] = minVal;
        }
    }

    return erodedImage;
}

/**
 * @brief Perform dilation operation on an image.
 *
 * @param img The image to apply the operation on.
 * @param kernelSize The size of the kernel to be used for the operation. This
 * must be an odd number.
 * @return The dilated image.
 * @throws std::invalid_argument If the kernel size is not odd.
 */
static inline Image dilate(Image img, int kernelSize) {
    if (kernelSize % 2 == 0) {
        throw invalid_argument("Kernel size must be odd!");
    }
    // Create a new image with same size but filled with 0 (black color)
    vector<vector<vector<uint8_t>>> newData(
        img.height, vector<vector<uint8_t>>(img.width, vector<uint8_t>(3, 0)));
    Image dilatedImage(newData, img.height, img.width);

    // Radius of the kernel
    int radius = kernelSize / 2;

    for (int i = radius; i < img.height - radius; i++) {
        for (int j = radius; j < img.width - radius; j++) {
            uint8_t maxVal = 0;

            // Find the maximum pixel value in the neighborhood
            for (int x = -radius; x <= radius; x++) {
                for (int y = -radius; y <= radius; y++) {
                    maxVal = max(
                        maxVal,
                        img.data[i + x][j + y][0]);  // Assuming grayscale image
                }
            }

            // Set the maximum value to the corresponding pixel in the dilated
            // image
            dilatedImage.data[i][j][0] = maxVal;
            dilatedImage.data[i][j][1] = maxVal;
            dilatedImage.data[i][j][2] = maxVal;
        }
    }

    return dilatedImage;
}

/**
 * @brief Perform the morphological operation 'Opening' on an image.
 * Opening is the dilation of the erosion of an image. It's used to remove
 * noise.
 *
 * @param img The image to apply the operation on.
 * @param kernelSize The size of the kernel to be used for the operation.
 * @return The image after the 'Opening' operation.
 */
Image open(Image img, int kernelSize) {
    // Perform an erosion followed by a dilation (Opening)
    return dilate(erode(std::move(img), kernelSize), kernelSize);
}

/**
 * @brief Perform the morphological operation 'Closing' on an image.
 * Closing is the erosion of the dilation of an image. It's used to close small
 * holes in the object.
 *
 * @param img The image to apply the operation on.
 * @param kernelSize The size of the kernel to be used for the operation.
 * @return The image after the 'Closing' operation.
 */
Image close(Image img, int kernelSize) {
    // Perform a dilation followed by an erosion (Closing)
    return erode(dilate(std::move(img), kernelSize), kernelSize);
}

/**
 * @brief Perform histogram equalization on the Value channel of an HSV image.
 *
 * @param hsv The input HSV image.
 * @return The equalized HSV image.
 */
Image equalizeHistogram(Image hsv) {
    vector<uint32_t> histogram(256, 0);

    // Calculate histogram for the Value channel
    for (int i = 0; i < hsv.height; ++i) {
        for (int j = 0; j < hsv.width; ++j) {
            histogram[hsv.data[i][j][2]]++;
        }
    }

    // Compute cumulative distribution function (CDF)
    vector<uint32_t> cdf(256, 0);
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; ++i) {
        cdf[i] = cdf[i - 1] + histogram[i];
    }

    // Normalize CDF to range 0-255
    uint32_t cdf_min = *min_element(cdf.begin(), cdf.end());
    uint32_t cdf_max = cdf[0xff];
    for (int i = 0; i < 256; ++i) {
        cdf[i] = ((cdf[i] - cdf_min) * 0xff) / (cdf_max - cdf_min);
    }

    // Apply histogram equalization
    Image equalized = hsv;
    for (int i = 0; i < equalized.height; ++i) {
        for (int j = 0; j < equalized.width; ++j) {
            equalized.data[i][j][2] = cdf[hsv.data[i][j][2]];
        }
    }

    return equalized;
}

vector<Point> directions = {{-1, 0}, {-1, 1}, {0, 1},  {1, 1},
                            {1, 0},  {1, -1}, {0, -1}, {-1, -1}};

/**
 * @brief Get the next direction in the Moore-Neighbor Tracing algorithm.
 *
 * @param currentDirection The current direction.
 * @return The next direction.
 */
static inline int getNextDirection(int currentDirection) {
    return (currentDirection + 1) % 8;
}

/**
 * @brief Find contours in a binary image using the Moore-Neighbor Tracing
 * algorithm.
 *
 * @param binaryImage The binary image.
 * @return A vector of contours, where each contour is represented as a vector
 * of points.
 */
vector<vector<Point>> findContours(const Image& binaryImage) {
    int height = binaryImage.height;
    int width = binaryImage.width;

    vector<vector<Point>> contours;
    vector<vector<bool>> visited(height, vector<bool>(width, false));

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            if (binaryImage.data[i][j][0] == 0xff && !visited[i][j]) {
                // New contour
                vector<Point> contour;

                // Moore-Neighbor Tracing
                Point p = {i, j};
                int bDirection = 0;
                do {
                    contour.push_back(p);
                    visited[p.x][p.y] = true;

                    int direction = bDirection;
                    for (int d = 0; d < 8; ++d) {
                        direction = getNextDirection(direction);
                        Point pCandidate = {p.x + directions[direction].x,
                                            p.y + directions[direction].y};

                        if (pCandidate.x >= 0 && pCandidate.x < height &&
                            pCandidate.y >= 0 && pCandidate.y < width &&
                            binaryImage.data[pCandidate.x][pCandidate.y][0] ==
                                0xff) {
                            p = pCandidate;
                            bDirection = (direction + 4) % 8;
                            break;
                        }
                    }
                } while (p.x != i || p.y != j);

                contours.push_back(contour);
            }
        }
    }

    return contours;
}

/**
 * @brief Calculate the area of a contour using the shoelace formula.
 *
 * @param contour The contour represented as a vector of points.
 * @return The area of the contour.
 */
static inline double calculateArea(const vector<Point>& contour) {
    double area = 0.0;
    int n = contour.size();
    for (int i = 0; i < n; ++i) {
        Point p1 = contour[i];
        Point p2 = contour[(i + 1) % n];
        area += (p1.x * p2.y - p2.x * p1.y);
    }
    return abs(area / 2.0);
}

/**
 * @brief Calculate the centroid of a binary image.
 *
 * @param binaryImage The binary image.
 * @return The centroid as a Point object.
 */
static inline Point calculateCentroid(const Image& binaryImage) {
    double pixelsX = 0, pixelsY = 0, totalP = 0;
    for (int i = 0; i < binaryImage.height; ++i) {
        for (int j = 0; j < binaryImage.width; ++j) {
            if (binaryImage.data[i][j][0] == 0xff) {
                pixelsX += j;
                pixelsY += i;
                totalP += 1;
            }
        }
    }
    if (totalP != 0) {  // avoid division by zero
        return {static_cast<int>(pixelsX / totalP),
                static_cast<int>(pixelsY / totalP)};
    } else {
        return {0, 0};
    }
}

/**
 * @brief Calculate the perimeter of a contour.
 *
 * @param contour The contour represented as a vector of points.
 * @return The perimeter of the contour.
 */
static inline double calculatePerimeter(const vector<Point>& contour) {
    double perimeter = 0.0;
    int n = contour.size();
    for (int i = 0; i < n; i++) {
        Point p1 = contour[i];
        Point p2 = contour[(i + 1) % n];
        perimeter += sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
    }
    return perimeter;
}

/**
 * @brief Calculate the circularity of a contour.
 *
 * @param contour The contour represented as a vector of points.
 * @return The circularity of the contour.
 */
static inline double calculateCircularity(const vector<Point>& contour) {
    double area = calculateArea(contour);
    double perimeter = calculatePerimeter(contour);
    return (4 * M_PI * area) / (perimeter * perimeter);
}

/**
 * @brief Get a circle fitted to a contour using the centroid and average
 * distance to points.
 *
 * @param contour The contour represented as a vector of points.
 * @return The fitted circle as a Circle object.
 */
static inline Circle getCircleFromContour(const vector<Point>& contour) {
    double sumX = 0.0, sumY = 0.0;
    for (const auto& point : contour) {
        sumX += point.x;
        sumY += point.y;
    }
    Point centroid = {static_cast<int>(sumX / contour.size()),
                      static_cast<int>(sumY / contour.size())};

    double sumDistance = 0.0;
    for (const auto& point : contour) {
        sumDistance +=
            sqrt(pow(point.x - centroid.x, 2) + pow(point.y - centroid.y, 2));
    }
    double radius = sumDistance / contour.size();

    return {centroid, radius};
}

/**
 * @brief Find circles in a binary image based on circularity criteria.
 *
 * @param binaryImage The binary image.
 * @param minCircularity The minimum circularity value for a circle to be
 * considered.
 * @return A vector of circles, each represented as a Circle object.
 */
vector<Circle> findCircles(const Image& binaryImage, double minCircularity) {
    vector<vector<Point>> contours = findContours(binaryImage);
    vector<Circle> circles;
    for (const auto& contour : contours) {
        double circularity = calculateCircularity(contour);
        if (circularity >= minCircularity) {
            circles.push_back(getCircleFromContour(contour));
        }
    }
    return circles;
}
