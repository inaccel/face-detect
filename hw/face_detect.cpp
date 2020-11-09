/*===============================================================*/
/*                                                               */
/*                       face_detect.cpp                         */
/*                                                               */
/*     Main host function for the Face Detection application.    */
/*                                                               */
/*===============================================================*/

// standard C/C++ headers
#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

// required OpenCV headers
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

// InAccel API
#include <inaccel/coral>

// other headers
#include "rectangles.h"
#include "safe_queue.h"
#include "utils.h"

typedef struct {
	cv::Mat frame;
	inaccel::vector<unsigned char> *input;
	inaccel::vector<int> *result_x;
	inaccel::vector<int> *result_y;
	inaccel::vector<int> *result_w;
	inaccel::vector<int> *result_h;
	inaccel::vector<int> *res_size;
	inaccel::session future;
} frame_request;

typedef struct {
	std::thread::id id;
	cv::Mat frame;
	int last;
	float fps;
} gui_frame;

void submitter(cv::VideoCapture &video, SafeQueue<frame_request> &queue, int frameNo) {
	std::cout << "Submitter thread\n";
	for (int i = 0; i < frameNo; i++) {
		cv::Mat frame;
		cv::Mat gray;

		video >> frame;
		if(frame.empty()) break;

		resize(frame, frame, cvSize(IMAGE_WIDTH, IMAGE_HEIGHT));
		cv::cvtColor(frame, gray, cv::COLOR_RGB2GRAY);

		inaccel::vector<unsigned char> *input = new inaccel::vector<unsigned char>;
		inaccel::vector<int> *result_x = new inaccel::vector<int>;
		inaccel::vector<int> *result_y = new inaccel::vector<int>;
		inaccel::vector<int> *result_w = new inaccel::vector<int>;
		inaccel::vector<int> *result_h = new inaccel::vector<int>;
		inaccel::vector<int> *res_size = new inaccel::vector<int>;

		input->resize(IMAGE_HEIGHT * IMAGE_WIDTH);
		result_x->resize(RESULT_SIZE);
		result_y->resize(RESULT_SIZE);
		result_w->resize(RESULT_SIZE);
		result_h->resize(RESULT_SIZE);
		res_size->resize(1);

		unsigned char *input_ptr = input->data();
		memcpy(input_ptr, gray.data, IMAGE_HEIGHT * IMAGE_WIDTH * sizeof(unsigned char));

		inaccel::request facedetect("edu.cornell.ece.zhang.rosetta.face-detect");
		facedetect.arg(*input).arg(*result_x).arg(*result_y).arg(*result_w).arg(*result_h).arg(*res_size);

		inaccel::session future = inaccel::submit(facedetect);

		frame_request queue_element;
		queue_element.frame = frame;
		queue_element.input = input;
		queue_element.result_x = result_x;
		queue_element.result_y = result_y;
		queue_element.result_w = result_w;
		queue_element.result_h = result_h;
		queue_element.res_size = res_size;
		queue_element.future = future;

		queue.enqueue(queue_element);
	}
	
	video.release();
}

void waiter(SafeQueue<frame_request> &queue, SafeQueue<gui_frame> &gui_queue, int frameNo) {
	std::cout << "Waiter thread\n";

	std::thread::id t_id = std::this_thread::get_id();

	float seconds = 0.05f;

	for (int i = 0; i < frameNo; i++) {
		auto start = std::chrono::high_resolution_clock::now();
		frame_request queue_element;

		queue_element = queue.dequeue();

		inaccel::wait(queue_element.future);

		if ((*queue_element.res_size)[0]) {
			const float GROUP_EPS = 0.4f;
			int minNeighbours = 1;

			groupRectangles(*(queue_element.result_x), *(queue_element.result_y),
							*(queue_element.result_w), *(queue_element.result_h),
							(*queue_element.res_size)[0], minNeighbours, GROUP_EPS);

		 	std::vector<cv::Mat> channels(3);
			split(queue_element.frame, channels);

			drawRectangles(queue_element.result_x->size(),
				queue_element.result_x->data(),
				queue_element.result_y->data(),
				queue_element.result_w->data(),
				queue_element.result_h->data(),
				channels[1].data);

			merge(channels, queue_element.frame);
		}

		float fps = i ? (i / seconds) : 1 / seconds;

		std::stringstream fps_stream;
		fps_stream << std::fixed << std::setprecision(2) << fps;

		cv::putText	(queue_element.frame, "AVG FPS: " + fps_stream.str(), cvPoint(IMAGE_WIDTH - 165, IMAGE_HEIGHT - 15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,255,0), 1, CV_AA);

		gui_frame gui;
		gui.id = t_id;
		gui.last = (i == frameNo - 1) ? 1 : 0;
		gui.fps = fps;
		gui.frame = queue_element.frame;

		gui_queue.enqueue(gui);

		delete queue_element.input;
		delete queue_element.result_x;
		delete queue_element.result_y;
		delete queue_element.result_w;
		delete queue_element.result_h;
		delete queue_element.res_size;

		auto end = std::chrono::high_resolution_clock::now();
		
		seconds += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0f;
		//if (i) seconds += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0f;
		//else seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0f;
	}
}

void viewer(unsigned long num_videos, SafeQueue<gui_frame> &queue) {
	std::cout << "Viewer Thread\n";

	cv::namedWindow("output", 1);
	cv::setWindowTitle("output", "InAccel Face Detection (FPGA)");

	std::map<std::thread::id, gui_frame> frames_map;

	unsigned videos_finished = 0;
	while(true) {
		gui_frame gui = queue.dequeue();

		if (gui.last) videos_finished++;

		frames_map[gui.id] = gui;

		float fps_sum = 0.0f;

		int window_width = ceil(sqrt(frames_map.size()));
		int window_height = ceil(frames_map.size() / (float) window_width);

		std::vector<std::vector<cv::Mat>> frames(window_height);
		for(unsigned i = 0; i < frames.size(); i++) {
			frames[i].resize(window_width);
			for (int j = 0; j < window_width; j++ ){
				frames[i][j] = cv::Mat::zeros(cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT), 16);
			}
		}

		int cnt = 0;
		for(auto it = frames_map.begin(); it != frames_map.end(); ++it) {
			gui_frame temp = it->second;

			fps_sum += temp.fps;

			frames[cnt / window_width][cnt % window_width] = temp.frame;
			cnt++;
		}

		std::vector<cv::Mat> temp(window_height);
		for(unsigned i = 0; i < temp.size(); i++) {
			hconcat(frames[i], temp[i]);
		}

		cv::Mat output;
		vconcat(temp, output);

		if (!videos_finished) {
			std::stringstream fps_stream;
			fps_stream << std::fixed << std::setprecision(2) << fps_sum;

			cv::Mat black = cv::Mat::zeros(cv::Size(output.cols, 30), output.type());
			cv::putText(black, "AVG System FPS: " + fps_stream.str(), cv::Point(black.cols - 250, black.rows - 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0,255,0), 1, CV_AA);
			vconcat(black, output, output);
		}

		cv::imshow("output", output);
		cv::waitKey(1);

		if (videos_finished == num_videos) break;
	}

	return;
}

int main(int argc, char ** argv) {
	std::cout << "Face Detection Application\n";

	std::vector<std::string> videoName;
	std::vector<cv::VideoCapture> video;

	for (int i = 0; i < argc - 1; i++) {
		video.push_back(cv::VideoCapture(argv[i + 1]));

		if (!video.back().isOpened()) {
			std::cerr << "Unable to open video file: " << argv[i + 1] << std::endl;
			video.pop_back();
		}
		else {
			videoName.push_back(std::to_string(video.size()) + ": " + std::string(argv[i + 1]));
		}
	}

	SafeQueue<gui_frame> gui_queue;
	std::vector<SafeQueue<frame_request>> queues(video.size(), SafeQueue<frame_request>(64));

	std::thread viewerThread;
	std::vector<std::thread> submitters(video.size());
	std::vector<std::thread> waiters(video.size());

	for (unsigned i = 0; i < video.size(); i++) {
		int frameNo = video[i].get(cv::CAP_PROP_FRAME_COUNT);

		waiters[i] = std::thread(waiter, std::ref(queues[i]), std::ref(gui_queue), frameNo);
		submitters[i] = std::thread(submitter, std::ref(video[i]), std::ref(queues[i]), frameNo);
	}

	if (video.size()) {
		viewerThread = std::thread(viewer, video.size(), std::ref(gui_queue));

		viewerThread.join();
	}

	for (unsigned i = 0; i < submitters.size(); i++) {
		submitters[i].join();
		waiters[i].join();
	}

	return 0;
}
