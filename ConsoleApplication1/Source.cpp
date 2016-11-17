#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include <iostream>
#include "UrlRequest.hpp"
#include <time.h>


using namespace std;
using namespace cv;

//int main(int argc, char* argv[]){
//	cv::namedWindow("Frame");
//	//cv::VideoCapture capture("http://88.53.197.250/axis-cgi/mjpg/video.cgi?resolution=320x240");
//	cv::VideoCapture capture("http://127.0.0.1:8080/");
//	//cv::VideoCapture capture("rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov");
//	while (capture.isOpened())     // check !!
//	{
//		cv::Mat frame;
//		if (!capture.read(frame)) // another check !!
//			break;
//
//		cv::imshow("Frame", frame);
//		if (cv::waitKey(1) >= 0)
//			break;
//	}
//	return 0;
//}
RNG rng(12345);
const static int SENSITIVITY_VALUE = 50; //sensitivity value is used in the absdiff() function
const static int BLUR_SIZE = 10; //blur is used to smooth the intensity image output from absdiff() function
const static int TIME_INTERVAL = 20;  // time between notifications [Clock_per_secon]/1000
static clock_t begin_time = clock();

void tracking(Mat thresholdImage, Mat &cameraFeed) {
	bool objectDetected = false;
	Mat thresholdCopy;
	thresholdImage.copyTo(thresholdCopy);
	//vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(thresholdCopy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // retrieves external contours


	if (contours.size()>0)objectDetected = true; //if contours vector is not empty, we have found some objects
	else objectDetected = false;


	if (objectDetected && (((float(clock() - begin_time))/1000) > TIME_INTERVAL)) {
		//the largest contour is found at the end of the contours vector, we assume that the biggest contour is the object we are looking for.
		vector< vector<Point> > largestContourVec;
		largestContourVec.push_back(contours.at(contours.size() - 1));
		putText(cameraFeed, "Wykryto ruch!", Point(5, 40), 2, 2, Scalar(5, 5, 255), 2);

		/*UrlRequest firebaserequest("https://fcm.googleapis.com", "/fcm/send");
		firebaserequest.addHeader("Content-Type: application/json\nAuthorization: key=AIzaSyDHdW5TlcGBwReOe5WhRBoH3aDzRlVmbNU");
		vector<pair<string, JsonValueAdapter> > body;
		body.push_back(pair<string, JsonValueAdapter>("notification", "{\"body\" : \"Move was detected!\",\"title\" : \"Check it!\"}"));
		body.push_back(pair<string, JsonValueAdapter>("to", "/topics/movementDetection"));
		firebaserequest.bodyJson(body);
		firebaserequest.method("POST");
		auto response = std::move(firebaserequest.perform());
		if (response.statusCode() == 200) {
			cout << "status code = " << response.statusCode() << ", body = *" << response.body() << "*, user is registered!" << endl;
		}
		else {
			cout << "status code = " << response.statusCode() << ", description = " << response.statusDescription() << endl;
		}*/

		stringstream push_notify;
		push_notify << "python"
			<< " "
			<< "postRequest.py";
		system(push_notify.str().c_str());
		cout << float(clock() - begin_time) << endl;
		begin_time = clock();
	}


	//draw contour and circles arround the object


	vector<vector<Point> > contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Point2f>center(contours.size());
	vector<float>radius(contours.size());
	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
		boundRect[i] = boundingRect(Mat(contours_poly[i]));
		minEnclosingCircle((Mat)contours_poly[i], center[i], radius[i]);
	}
	for (int i = 0; i< contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(cameraFeed, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
		circle(cameraFeed, center[i], (int)radius[i], color, 2, 8, 0);
	}
}
int main() {
	// variables for case purposes
	bool objectDetected = true;
	bool debugMode = false;
	bool trackingEnabled = true;
	bool pause = false;

	Mat frame1, frame2; //two frames used for comparison
	Mat grayImage1, grayImage2; //their grayscale images needed for absdiff() function
	Mat differenceImage; //resulting difference image
	Mat thresholdImage; //thresholded difference image (for use in findContours() function)
	VideoCapture capture; //video capture object.
						  // initialization of camera capture and resizing of frame window
	capture.open(0);
	double dWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
	double dHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the video
	Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));
	// precaution that we have camera feed
	if (!capture.isOpened()) {
		cout << "ERROR ACQUIRING VIDEO FEED\n";
		getchar();
		return -1;
	}
	while (1) {
		capture.read(frame1); //read first frame
		cvtColor(frame1, grayImage1, COLOR_BGR2GRAY); //convert frame1 to gray scale for frame differencing
		capture.read(frame2); //read second frame
		cvtColor(frame2, grayImage2, COLOR_BGR2GRAY); //convert frame2 to gray scale for frame differencing
		absdiff(grayImage1, grayImage2, differenceImage); // frame differencing
		threshold(differenceImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY); //threshold intensity image at a given sensitivity value
		blur(thresholdImage, thresholdImage, cv::Size(BLUR_SIZE, BLUR_SIZE)); //blur the image to get rid of the noise
		threshold(thresholdImage, thresholdImage, SENSITIVITY_VALUE, 255, THRESH_BINARY); //threshold again to obtain binary image from blur output
		if (debugMode == true) {
			//show the difference image and threshold image
			imshow("Difference Image", differenceImage);
			imshow("Threshold Image", thresholdImage);
			imshow("Final Threshold Image", thresholdImage); //show the threshold image after blur
		}
		else {
			//if not in debug mode, destroy the windows
			destroyWindow("Difference Image");
			destroyWindow("Threshold Image");
			destroyWindow("Final Threshold Image");
		}


		//if tracking enabled, search for contours in thresholded image
		if (trackingEnabled) {
			tracking(thresholdImage, frame1);
		}


		imshow("Frame1", frame1); //show our camera feed
								  // key kodes are for windows purposes. If running on linux codes will be different. Please check and adjust accordingly :)
		switch (waitKey(10)) {
		case 27: //'esc' -> exit program.
			return 0;
		case 116: //'t' -> tracking
			trackingEnabled = !trackingEnabled;
			if (trackingEnabled == false) cout << "Tracking disabled." << endl;
			else cout << "Tracking enabled." << endl;
			break;
		case 100: //'d' -> debug mode
			debugMode = !debugMode;
			if (debugMode == false) cout << "Debug mode disabled." << endl;
			else cout << "Debug mode enabled." << endl;
			break;
		case 112: //'p' -> pause video, press again to resume
			pause = !pause;
			if (pause == true) {
				cout << "Video paused, press 'p' again to resume" << endl;
				while (pause == true) {
					switch (waitKey()) {
					case 112:
						pause = false;
						cout << "Video Resumed" << endl;
						break;
					}
				}
			}
		}
	}
	return 0;
}

