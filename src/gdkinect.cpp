#include "gdkinect.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <stdexcept>
#include <algorithm>

#define LOWEST_DEPTH 5000
#define KINECT_WIDTH 640
#define KINECT_HEIGHT 480
#define SQUARE_SIZE 30
#define SEARCH_RADIUS 30
#define SCREEN_WIDTH 1920.0
#define SCREEN_HEIGHT 1080.0
#define HAND_DEPTH_CLOSE 20
#define HAND_DEPTH_FAR 30

using namespace godot;


HandPos::HandPos()
    : x(-1),
    y(-1),
    avg(LOWEST_DEPTH) {}


GDKinect::GDKinect()
    : rgbMatrix(cv::Size(KINECT_WIDTH, KINECT_HEIGHT), CV_8UC3, cv::Scalar(0)),
    depthMatrix(cv::Size(KINECT_WIDTH, KINECT_HEIGHT), CV_16UC1),
    depthf(cv::Size(KINECT_WIDTH, KINECT_HEIGHT), CV_8UC1) {

    try {
	depthDat = std::make_unique<uint16_t[]>(KINECT_HEIGHT * KINECT_WIDTH * sizeof(uint16_t));
	kinect_device = freenect.createDevice<CustomFreenectDevice>(0);
	kinect_device->get().startVideo();
	kinect_device->get().startDepth();
	UtilityFunctions::print("Started Kinect RGB and depth video");
    } catch (std::runtime_error &e) {
	UtilityFunctions::print(e.what());
    }
}

GDKinect::~GDKinect() {
    if (kinect_device) {
	kinect_device->get().stopVideo();
	kinect_device->get().stopDepth();
    }
    UtilityFunctions::print("Stopped Kinect RGB and depth video");
}

cv::Mat& GDKinect::get_rgb_matrix() {
    if (kinect_device) kinect_device->get().getVideo(rgbMatrix);
    return rgbMatrix;
}

cv::Mat& GDKinect::get_depth_matrix() {
    if (kinect_device && kinect_device->get().getDepth(depthDat.get())) {
	depthMatrix.data = (uchar*) depthDat.get();
        depthMatrix.convertTo(depthf, CV_8UC1, 255.0/2048.0);
    }

    return depthf;
}

Vector2 GDKinect::get_position() {
	std::optional<HandPos> h = get_hand_pos();
	if(!h) {
		return Vector2(0, 0);
	}
	return Vector2(SCREEN_WIDTH - h->x * (SCREEN_WIDTH / KINECT_WIDTH), h->y * (SCREEN_HEIGHT / KINECT_HEIGHT));
}

Ref<Texture> GDKinect::get_texture() {
    cv::Mat dst;
    cv::cvtColor(get_depth_matrix(), dst, cv::COLOR_BGR2RGB);
    int sizear = dst.cols * dst.rows * dst.channels();

    std::optional<HandPos> h = get_hand_pos();
    // UtilityFunctions::print(h ? h->x : -1);
    // UtilityFunctions::print(h ? h->y : -1);
    // UtilityFunctions::print(h ? h->avg : -1);

    PackedByteArray bytes;
    bytes.resize(sizear);
    memcpy(bytes.ptrw(), dst.data, sizear);

    Ref<Image> image = Image::create_from_data(dst.cols, dst.rows, false,
				Image::Format::FORMAT_RGB8, bytes);
    return ImageTexture::create_from_image(image);
}

void GDKinect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_texture"), &GDKinect::get_texture);
	ClassDB::bind_method(D_METHOD("get_position"), &GDKinect::get_position);
	ClassDB::bind_method(D_METHOD("is_fist"), &GDKinect::is_fist);
}

std::optional<HandPos> GDKinect::get_hand_pos() {
    get_depth_matrix();
    int avg;
    HandPos best;
    int lowest_depth{LOWEST_DEPTH};
    uint16_t *depthMat = depthDat.get();

    if (SQUARE_SIZE < KINECT_HEIGHT && SQUARE_SIZE < KINECT_WIDTH) {
	int count;
	uint16_t dat;

	// optimization: only search in surroundings of previous position
	if (!hand_pos) {
	    for (int i{SQUARE_SIZE/2}; i < KINECT_HEIGHT - SQUARE_SIZE / 2; i += 4) {
		for (int j{SQUARE_SIZE/2}; j < KINECT_WIDTH - SQUARE_SIZE / 2; j += 4) {
		    count = 0;
		    avg = 0;

		    for (int ri{i - SQUARE_SIZE / 2}; ri <= i + SQUARE_SIZE / 2; ri++) {
			for (int rj{j - SQUARE_SIZE / 2}; rj <= j + SQUARE_SIZE / 2; rj++) {
			    dat = depthMat[KINECT_WIDTH * ri + rj];

			    if (dat >= 500 && dat <=1500) {
				avg += static_cast<int>(dat);
				count++;
			    }
			}
		    }

		    // if (count >= (SQUARE_SIZE) * (SQUARE_SIZE)) {
		    if (count > 0) {
			avg /= count;

			if (avg < lowest_depth) {
			    // consider what to do when avg=lowest_depth
			    lowest_depth = avg;
			    best.x = j;
			    best.y = i;
			    best.avg = avg;
			}
		    }
		}
	    }
	} else {
	    for (int i{std::max(SQUARE_SIZE / 2, hand_pos->y - SEARCH_RADIUS)}; i < hand_pos->y + SEARCH_RADIUS && i < KINECT_HEIGHT - SQUARE_SIZE / 2; i += 4) {
		for (int j{std::max(SQUARE_SIZE / 2, hand_pos->x - SEARCH_RADIUS)}; j < hand_pos->x + SEARCH_RADIUS && KINECT_WIDTH - SQUARE_SIZE / 2; j += 4) {
		    count = 0;
		    avg = 0;

		    for (int ri{i - SQUARE_SIZE / 2}; ri <= i + SQUARE_SIZE / 2; ri++) {
			for (int rj{j - SQUARE_SIZE / 2}; rj <= j + SQUARE_SIZE / 2; rj++) {
			    dat = depthMat[KINECT_WIDTH * ri + rj];

			    if (dat >= 500 && dat <=1500) {
				avg += static_cast<int>(dat);
				count++;
			    }
			}
		    }

		    // if (count >= (SQUARE_SIZE + 1) * (SQUARE_SIZE + 1) * 0.98) {
		    if (count > 0) {
			avg /= count;

			if (avg < lowest_depth) {
			    // consider what to do when avg=lowest_depth
			    lowest_depth = avg;
			    best.x = j;
			    best.y = i;
			    best.avg = avg;
			}
		    }
		}
	    }
	}
    }

    if (best.x == -1) hand_pos = {};
    else hand_pos = best;
    return hand_pos;
}

// will use the last calculated hand position and depth avg.
bool GDKinect::is_fist(){
	// UtilityFunctions::print("executing is_fist");
	// if(pos.avg < 800){ //otherwise no hand is recognized/accepted.
	uint16_t* depth = depthDat.get();
	int half_side{(SQUARE_SIZE - 1)/2};
	int min_height{hand_pos->y - half_side}; // highest point
	int max_height{hand_pos->y + half_side}; // lowest point
	int min_width{hand_pos->x - half_side};
	int max_width{hand_pos->x + half_side};
	int depth_far{hand_pos->avg + HAND_DEPTH_FAR};
	int depth_close{hand_pos->avg - HAND_DEPTH_CLOSE};
	int box_up{min_height};
	int box_down{max_height};
	int box_left{min_width};
	int box_right{max_width};
	uint16_t cur;
	bool found{true};
	//height search
	for(int i{min_height - 1}; i>=std::max(0, hand_pos->y - 100) && found; i--){
		found = false;
		for(int j = min_width; j <= max_width; j++){
			cur = depth[i*640 + j];
			if(cur < depth_far && cur > depth_close){
				box_up--;
				found = true;
				break;
			}
		}
		if(!found){
			break;
		}
	}
	found = true;
	for(int i{max_height + 1}; i<=std::min(479, hand_pos->y + 100) && found; i++){
		found = false;
		for(int j{min_width}; j <= max_width; j++){
			cur = depth[i*640 + j];
			if(cur < depth_far && cur > depth_close){
				box_down++;
				found = true;
				break;
			}
		}
		if(!found){
			break;
		}
	}
	found = true;
	for(int i{min_width - 1}; i>=std::max(0, hand_pos->x - 100) && found; i--){
		found = false;
		for(int j{min_height}; j <= max_height; j++){
			cur = depth[j*640 + i];
			if(cur < depth_far && cur > depth_close){
				box_left--;
				found = true;
				break;
			}
		}
		if(!found){
			break;
		}
	}
	found = true;
	for(int i{max_width + 1}; i<=std::min(639, hand_pos->x + 100) && found; i++){
		found = false;
		for(int j{min_height}; j <= max_height; j++){
			cur = depth[j*640 + i];
			if(cur < depth_far && cur > depth_close){
				box_right++;
				found = true;
				break;
			}
		}
		if(!found){
			break;
		}
	}	
	int area{(box_down - box_up) * (box_right - box_left)};
	// }
	UtilityFunctions::print(area);
	return true;
}

