#include <iostream>
#include "stdafx.h"
#include "common.h"

#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/tracking.hpp>

// 480rows 640cols -> camera
// resized -> (240,320)

bool DEBUG = false;
bool PRINT_DEBUG = false;
int histograms[60000][256];
bool first = true;
int const TRESHOLD = 200;

int histogramIntersection(int h1[256], int h2[256]) {
	int H = 0;
	for (int i = 0; i < 256; i++) {
		printf("%d -> %d \n", h1[i], h2[i]);
		H += min(h1[i], h2[i]);
	}
	return H;
}

void showHistogram(const std::string& name, int* hist, const int  hist_cols, const int hist_height)
{
	Mat imgHist(hist_height, hist_cols, CV_8UC3, CV_RGB(255, 255, 255)); // constructs a white image

																		 //computes histogram maximum
	int max_hist = 0;
	for (int i = 0; i<hist_cols; i++)
		if (hist[i] > max_hist)
			max_hist = hist[i];
	double scale = 1.0;
	scale = (double)hist_height / max_hist;
	int baseline = hist_height - 1;

	for (int x = 0; x < hist_cols; x++) {
		Point p1 = Point(x, baseline);
		Point p2 = Point(x, baseline - cvRound(hist[x] * scale));
		line(imgHist, p1, p2, CV_RGB(255, 0, 255)); // histogram bins colored in magenta
	}

	imshow(name, imgHist);
}

int * createHistogram(cv::Mat src, int startI, int startJ, int stepI, int stepJ) {
	int hist[256];
	for (int i = 0; i < 256; i++)
		hist[i] = 0;
	for (int i = startI; i < startI + stepI; i++)
		for (int j = startJ; j < startJ + stepJ; j++)
			hist[src.at<uchar>(i, j)]++;

	return hist;
}

cv::Mat convertImageToGrayScale(Mat src) {
	int height = src.rows;
	int width = src.cols;
	Mat gray = Mat(height, width, CV_8UC1);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			gray.at<uchar>(i, j) = (src.at<Vec3b>(i, j)[0] + src.at<Vec3b>(i, j)[1] + src.at<Vec3b>(i, j)[2]) / 3;
		}

	if (DEBUG) {
		imshow("Gray", gray);
	}
	return gray;
}

cv::Mat LBP(cv::Mat frame) {
	const int IMG_WIDTH = frame.cols;
	const int IMG_HEIGH = frame.rows;
	cv::Mat result = frame.clone();
	int kernel[3][3] = { { 1, 2,  4},
						 { 8, 0, 16},
						 {32,64,128} };
	int kernelSize = 3;
	int treshold = 0;
	int k = kernelSize / 2;	
	int temp;

	for (int i = k; i < IMG_HEIGH - k; i++) {
		for (int j = k; j < IMG_WIDTH - k; j++) {
			temp = 0;
			for (int u = 0; u < 2 * k + 1; u++) {
				for (int v = 0; v < 2 * k + 1; v++) {
					if (frame.at<uchar>(i - k + u, j - k + v) >= frame.at<uchar>(i, j)) {
						if (!(u == i && v == j)) {
							temp += kernel[u][v];
							
						}
					}
				}
			}
			result.at<uchar>(i, j) = temp;
		}
	}

	if (DEBUG) {
		imshow("After LBP", result);
	}
	return result;
}




int * egalizareHisto(int hist[256], cv::Mat img, int startI, int startJ, int stepI, int stepJ) {
	if (DEBUG)
		showHistogram("hist initial", hist, 265, 300);

	int hc[256] = {};
	int tab[256] = {};
	hc[0] = 0;

	for (int g = 1; g < 256; g++)
		hc[g] = hc[g - 1] + hist[g];

	Mat dst = img.clone();

	int M = stepI * stepJ;

	for (int g = 0; g < 256; g++)
		tab[g] = 255 * hc[g] / M;

	for (int i = startI; i < startI + stepI; i++)
		for (int j = startJ; j < startJ + stepJ; j++)
			dst.at<uchar>(i, j) = tab[img.at<uchar>(i, j)];

	int *histFinal = createHistogram(dst, startI, startJ, stepI, stepJ);
	if (DEBUG)
		showHistogram("hist final",histFinal, 265, 300);
	return histFinal;
}

void moveHisto(int tempEgalizedHisto[256], int noHist) {
	for (int i = 0; i < 256; i++)
		histograms[noHist][i] = tempEgalizedHisto[i];
}

void afisareValoriHisto(int his[256]) {
	printf("\nhist values\n");
	for (int i = 0; i < 256; i++)
		printf("\n%d %d ", i, his[i]);
}

void divideAndUpdateHisto(cv::Mat img) {
	//int *histogram = createHistogram(img, 0, 0, img.rows, img.cols);
	int noHist = 0;
	int step = 40;

	for (int i = 0; i < img.rows; i += step) {
		for (int j = 0; j < img.cols; j += step) {

			// histograma pentru un bloc
			int *tempHisto = createHistogram(img, i, j, step, step);
			afisareValoriHisto(tempHisto);
			break;
			// histograma egalizata pt un bloc 
			/*int *tempEgalizedHisto = egalizareHisto(tempHisto, img, i, j, step, step);

			if (first)
				moveHisto(tempEgalizedHisto, noHist);
			else {
				// compare histograms and change img
				int tresh = histogramIntersection(histograms[noHist], tempEgalizedHisto);
				//printf("%d\n", tresh);
				/*if (tresh > TRESHOLD)
					printf("move!");
			}
			noHist++;
			if (PRINT_DEBUG)
				printf("\n(%d,%d)", i, j);*/
		}
	}
}

void process(cv::Mat frame) {
	// convertim imaginea la grayscale
	cv::Mat gray = convertImageToGrayScale(frame);
	// calculeaza LBP pt imaginea grayscale
	cv::Mat imgAfterLBP = LBP(gray);
	//
	divideAndUpdateHisto(imgAfterLBP);
	
	if (first) 
		first = false;
}

int main()
{
	/*cv::Mat test = Mat(3, 3, CV_8UC1);
	test.at<uchar>(0, 0) = 5;
	test.at<uchar>(0, 1) = 4;
	test.at<uchar>(0, 2) = 3;
	test.at<uchar>(1, 0) = 4;
	test.at<uchar>(1, 1) = 3;
	test.at<uchar>(1, 2) = 1;
	test.at<uchar>(2, 0) = 2;
	test.at<uchar>(2, 1) = 0;
	test.at<uchar>(2, 2) = 3;
	cv::Mat rez = LBP(test);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++)
			printf("%d ", rez.at<uchar>(i, j));
		printf("\n");
	}
	*/
 	cv::VideoCapture cap;
	cap.open(0);

	if (!cap.isOpened())
	{
		printf( "***Could not initialize capturing...***\n");
		return -1;
	}

	cv::Mat frame;
	cv::Mat resized;

	//while (1) {
		cap >> frame;
		//process frame
		
		cv::resize(frame, resized, cv::Size(), 0.5, 0.5);
		
		if (resized.empty()) {
			printf( "frame is empty" );
			//break;
		}
		else {
			if (PRINT_DEBUG)
				printf("Size of the image: (%d,%d)", resized.rows, resized.cols);
			//process(resized);
			first = false;
		}
		cv::Mat gray = convertImageToGrayScale(frame);
		// calculeaza LBP pt imaginea grayscale
		cv::Mat imgAfterLBP = LBP(gray);
		cv::imshow("", imgAfterLBP);
		
				
		cv::waitKey(10);
	//}

	return 1;
}