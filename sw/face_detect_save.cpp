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
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <thread>

// required OpenCV headers
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

// other headers
#include "haar.h"
#include "safe_queue.h"

typedef struct {
	std::thread::id id;
	cv::Mat frame;
	int last;
	float fps;
	double real_fps;
} gui_frame;

void submitter(cv::VideoCapture &video, SafeQueue<gui_frame> &queue, int frameNo) {
	std::cout << "Submitter thread\n";

	int minNeighbours = 1;
	float scaleFactor = 1.2f;
	float seconds = 1;

	std::thread::id t_id = std::this_thread::get_id();

	MyImage inputobj;
	MyImage *input = &inputobj;
	input->width = IMAGE_WIDTH;
	input->height = IMAGE_HEIGHT;
	input->data = (unsigned char *) malloc(IMAGE_HEIGHT * IMAGE_WIDTH * sizeof(unsigned char));

	myCascade cascadeobj;
	myCascade *cascade = &cascadeobj;
	MySize minSize = {20, 20};
	MySize maxSize = {0, 0};

	cascade->n_stages=25;
	cascade->total_nodes=2913;
	cascade->orig_window_size.height = 24;
	cascade->orig_window_size.width = 24;

	std::vector<MyRect> result;

	int *stages_array = NULL;
	int *rectangles_array = NULL;
	int *weights_array = NULL;
	int *alpha1_array = NULL;
	int *alpha2_array = NULL;
	int *tree_thresh_array = NULL;
	int *stages_thresh_array = NULL;
	int **scaled_rectangles_array = NULL;
	double real_fps = video.get(cv::CAP_PROP_FPS);

	readTextClassifier(&stages_array, &rectangles_array, &weights_array, &alpha1_array, &alpha2_array, &tree_thresh_array, &stages_thresh_array, &scaled_rectangles_array);

	for (int i = 0; i < frameNo; i++) {
		auto start = std::chrono::high_resolution_clock::now();

		cv::Mat frame;
		cv::Mat gray;

		video >> frame;
		if(frame.empty()) break;

		resize(frame, frame, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT));
		cv::cvtColor(frame, gray, cv::COLOR_RGB2GRAY);

		memcpy(input->data, gray.data, IMAGE_HEIGHT * IMAGE_WIDTH * sizeof(unsigned char));

		result = detectObjects(input, minSize, maxSize, cascade, scaleFactor, minNeighbours,
						stages_array, rectangles_array, weights_array, alpha1_array,
						alpha2_array, tree_thresh_array, stages_thresh_array, scaled_rectangles_array);

		if (result.size()) {
			std::vector<cv::Mat> channels(3);
			split(frame, channels);

			for(int j = 0; j < (int) result.size(); j++) {
				drawRectangle(channels[1].data, result[j]);
			}

			merge(channels, frame);
		}

		auto end = std::chrono::high_resolution_clock::now();

		seconds += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0f;
		float fps = i ? (i / seconds) : 1 / seconds;

		std::stringstream fps_stream;
		fps_stream << std::fixed << std::setprecision(2) << fps;

		cv::putText(frame, "AVG FPS: " + fps_stream.str(), cv::Point(IMAGE_WIDTH - 165, IMAGE_HEIGHT - 15), cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cv::Scalar(0,255,0), 1, cv::LINE_AA);

		gui_frame gui;
		gui.id = t_id;
		gui.last = (i == frameNo - 1) ? 1 : 0;
		gui.fps = fps;
		gui.frame = frame;
		gui.real_fps = real_fps;

		queue.enqueue(gui);
	}

	releaseTextClassifier(stages_array, rectangles_array, weights_array, alpha1_array, alpha2_array, tree_thresh_array, stages_thresh_array, scaled_rectangles_array);

	free(input->data);

	video.release();
}

void viewer(unsigned long num_videos, SafeQueue<gui_frame> &queue) {
	std::cout << "Viewer Thread\n";

	std::map<std::thread::id, cv::VideoWriter> video_writers;

	unsigned videos_finished = 0;
	while(true) {
		gui_frame gui = queue.dequeue();

		cv::VideoWriter writer = std::move(video_writers[gui.id]);

		if (!writer.isOpened()) {
			std::stringstream ss;
			ss << gui.id;
			std::string filename = std::string("video-") + ss.str() + std::string(".mp4");
			video_writers[gui.id] = std::move(cv::VideoWriter(filename, cv::VideoWriter::fourcc('M','P','4','V'), gui.real_fps, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT)));
			writer = std::move(video_writers[gui.id]);
		}

		writer.write(gui.frame);

		if (gui.last) {
			videos_finished++;
			video_writers[gui.id].release();
		}

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

	SafeQueue<gui_frame> queue;

	std::thread viewerThread;
	std::vector<std::thread> submitters(video.size());

	for (unsigned i = 0; i < video.size(); i++) {
		int frameNo = video[i].get(cv::CAP_PROP_FRAME_COUNT);

		submitters[i] = std::thread(submitter, std::ref(video[i]), std::ref(queue), frameNo);
	}

	if (video.size()) {
		viewerThread = std::thread(viewer, video.size(), std::ref(queue));

		viewerThread.join();
	}

	for (unsigned i = 0; i < submitters.size(); i++) {
		submitters[i].join();
	}

	return 0;
}
