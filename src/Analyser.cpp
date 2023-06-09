/**
 * Class `Analyser` is responsible for processing video streams and performing
 * color and shape detection. It can identify blue circles, red circles,
 * octagons, and squares in the frames of a video stream.
 */
#include "Analyser.hpp"

/**
 * The `processFrame` function performs color conversion and histogram
 * equalization on a frame.
 *
 * @param frame - A reference to a cv::Mat object representing the frame to
 * be processed.
 */
static inline void processFrame(cv::Mat& frame) {
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // Split the image into H, S, and V channels
    vector<cv::Mat> hsv_channels;
    cv::split(hsv, hsv_channels);
    // Apply histogram equalization to the V channel
    cv::equalizeHist(hsv_channels[2], hsv_channels[2]);
    // Merge the H, S, and V channels back into a single image
    cv::merge(hsv_channels, hsv);
    // Convert back to BGR color space for color detection
    cv::cvtColor(hsv, frame, cv::COLOR_HSV2BGR);
}

/**
 * The `handleBlueCircles` function processes detected blue circles, drawing
 * them on the frame, indicating their centers, and displaying a direction
 * of movement based on the circle's center of mass.
 *
 * @param blueCircles - A vector of pairs. Each pair contains a cv::Vec3f
 * representing the detected circle and a cv::Point2f representing the
 * center of mass of the corresponding circle.
 * @param frame - A reference to a cv::Mat object representing the frame to
 * be modified.
 */
static inline void handleBlueCircles(
    const vector<pair<cv::Vec3f, cv::Point2f>>& blueCircles, cv::Mat& frame) {
    for (const auto& bcircle : blueCircles) {
        cv::Vec3f circle = bcircle.first;
        cv::Point2f center_of_mass = bcircle.second;

        cv::circle(
            frame,
            cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
            static_cast<int>(circle[2]), cv::Scalar(0, 255, 0), 3, cv::LINE_AA);

        double delta = cv::Point2f(circle[0], circle[1]).x - center_of_mass.x;

        string circle_center = "(" + to_string((int)circle[0]) + ", " +
                               to_string((int)circle[1]) + ")";
        cv::putText(frame, circle_center,
                    cv::Point(static_cast<int>(circle[0] - 40),
                              static_cast<int>(circle[1] + 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0x00, 0x00, 0x00),
                    2);

        if (center_of_mass.x > circle[0]) {
            cv::putText(frame, "Turn Left",
                        cv::Point(static_cast<int>(circle[0] - 40),
                                  static_cast<int>(circle[1] - 20)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(40, 255, 50),
                        2);

            cout << "TurnLeft\tDetected blue circle: center: (" << circle[0]
                 << ", " << circle[1] << "), center of Mass: ("
                 << center_of_mass.x << ", " << center_of_mass.y
                 << "), delta: " << delta << endl;
        } else {
            cv::putText(frame, "Turn Right",
                        cv::Point(static_cast<int>(circle[0] - 40),
                                  static_cast<int>(circle[1] - 20)),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(40, 255, 50),
                        2);

            cout << "TurnRight\tDetected blue circle: center: (" << circle[0]
                 << ", " << circle[1] << "), center of Mass: ("
                 << center_of_mass.x << ", " << center_of_mass.y
                 << "), delta: " << delta << endl;
        }
    }
}

/**
 * The `handleRedCircles` function processes detected red circles, drawing
 * them on the frame, indicating their centers, and labeling them as
 * forbidden.
 *
 * @param redCircles - A vector of pairs. Each pair contains a cv::Vec3f
 * representing the detected circle and a cv::Point2f representing the
 * center of mass of the corresponding circle.
 * @param frame - A reference to a cv::Mat object representing the frame to
 * be modified.
 */
static inline void handleRedCircles(
    const vector<pair<cv::Vec3f, cv::Point2f>>& redCircles, cv::Mat& frame) {
    for (const auto& rcircle : redCircles) {
        cv::Vec3f circle = rcircle.first;

        if (circle[2] < 10 || circle[2] > 500) {
            continue;
        }

        cv::circle(
            frame,
            cv::Point(static_cast<int>(circle[0]), static_cast<int>(circle[1])),
            static_cast<int>(circle[2]), cv::Scalar(0, 255, 0), 3, cv::LINE_AA);

        cout << "STOP\t\tDetected red circle: center: (" << circle[0] << ", "
             << circle[1] << endl;

        string circle_center = "(" + to_string((int)circle[0]) + ", " +
                               to_string((int)circle[1]) + ")";
        cv::putText(frame, circle_center,
                    cv::Point(static_cast<int>(circle[0] - 40),
                              static_cast<int>(circle[1] + 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0x00, 0x00, 0x00),
                    2);

        cv::putText(frame, "Forbidden",
                    cv::Point(static_cast<int>(circle[0] - 40),
                              static_cast<int>(circle[1] - 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(40, 255, 50), 2);
    }
}

/**
 * The `handleOctagons` function processes detected octagons, drawing them
 * on the frame, indicating their centers, and labeling them as "Stop".
 *
 * @param octagons - A vector of cv::Point vectors, each representing an
 * octagon.
 * @param frame - A reference to a cv::Mat object representing the frame to
 * be modified.
 */
static inline void handleOctagons(const vector<vector<cv::Point>>& octagons,
                                  cv::Mat& frame) {
    for (const auto& octagon : octagons) {
        cv::polylines(frame, octagon, true, cv::Scalar(0, 255, 0), 3,
                      cv::LINE_AA);

        // Calculate the center of the contour
        cv::Moments M = cv::moments(octagon);
        int cX = static_cast<int>(M.m10 / M.m00);
        int cY = static_cast<int>(M.m01 / M.m00);

        string octagon_center =
            "(" + to_string(cX) + ", " + to_string(cY) + ")";
        cv::putText(frame, octagon_center, cv::Point(cX - 40, cY + 20),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255),
                    2);

        cv::putText(frame, "Stop", cv::Point(cX, cY), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(255, 255, 255), 2);
    }
}

/**
 * The `handleSquares` function processes detected squares, drawing them on
 * the frame, indicating their centers, and displaying a direction of
 * movement based on the square's center of mass.
 *
 * @param squares - A vector of pairs. Each pair contains a vector of
 * cv::Point objects representing the square and a cv::Point2f representing
 * the center of mass of the corresponding square.
 * @param frame - A reference to a cv::Mat object representing the frame to
 * be modified.
 */
static inline void handleSquares(
    const vector<pair<vector<cv::Point>, cv::Point2f>>& squares,
    cv::Mat& frame) {
    for (const auto& square : squares) {
        vector<cv::Point> contour = square.first;
        cv::Point2f center_of_mass = square.second;

        cv::Rect boundingRect = cv::boundingRect(contour);
        cv::Point2f center_of_square =
            (boundingRect.br() + boundingRect.tl()) * 0.5;

        cv::polylines(frame, contour, true, cv::Scalar(255, 0, 0), 3,
                      cv::LINE_AA);

        string square_center = "(" + to_string((int)center_of_square.x) + ", " +
                               to_string((int)center_of_square.y) + ")";
        cv::putText(frame, square_center,
                    cv::Point(static_cast<int>(center_of_square.x - 40),
                              static_cast<int>(center_of_square.y + 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0x00, 0x00, 0x00),
                    2);

        double delta = center_of_mass.y - center_of_square.y;

        string direction;
        if (delta < 0) {
            direction = "Highway";
        } else {
            direction = "Vram";
        }

        cout << "Square: Center of Square: (" << center_of_square.x << ", "
             << center_of_square.y << "), Center of Mass: (" << center_of_mass.x
             << ", " << center_of_mass.y << "), Delta: " << delta
             << ", Direction: " << direction << endl;

        cv::putText(frame, direction,
                    cv::Point(static_cast<int>(center_of_square.x),
                              static_cast<int>(center_of_square.y - 20)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0x00, 0x00, 0x00),
                    2);
    }
}

/**
 * The `processVideo` function reads frames from a video source, performs
 * color and shape detection, and processes each frame accordingly.
 *
 * @param cap - A reference to a cv::VideoCapture object representing the
 * video source.
 */
void Analyser::processVideo(cv::VideoCapture& cap) {
    cv::Mat frame;

    while (cap.read(frame)) {
        processFrame(frame);
        cv::Mat redMask = ColorDetector::detectRed(frame);
        cv::Mat blueMask = ColorDetector::detectBlue(frame);

        redMask = ShapeDetector::removeSmallComponents(redMask);
        blueMask = ShapeDetector::removeSmallComponents(blueMask);
        cv::Mat colorMask = redMask | blueMask;

        // Send blue mask to detect circles, and mass center
        vector<pair<cv::Vec3f, cv::Point2f>> blueCircles =
            ShapeDetector::detectCircles(blueMask);
        // Send red mask to detect circles, and mass center
        vector<pair<cv::Vec3f, cv::Point2f>> redCircles =
            ShapeDetector::detectCircles(redMask);
        // Send red mask to detect octagons
        vector<vector<cv::Point>> octagons =
            ShapeDetector::detectOctagons(redMask);
        // Send color mask to detect squares
        vector<pair<vector<cv::Point>, cv::Point2f>> squares =
            ShapeDetector::detectSquares(colorMask);

        handleBlueCircles(blueCircles, frame);
        handleRedCircles(redCircles, frame);
        handleOctagons(octagons, frame);
        handleSquares(squares, frame);

        cv::imshow("binary", colorMask);
        cv::imshow("Analyser", frame);

        int key = cv::waitKey(30);
        if (key == 'x' || key == 'X') {
            cout << "Exiting program..." << endl;
            break;
        }
    }

    cv::destroyAllWindows();
}
