#include "device.h"

namespace lptc_coderdojo {

const std::chrono::milliseconds kLockTimeout = std::chrono::milliseconds(30);

template <typename T>
FrameQueue<T>::FrameQueue() {}

template <typename T>
void FrameQueue<T>::Push(const std::vector<T>& data) {
  std::lock_guard<std::mutex> guard(queue_lock);
  queue.push(data);
  queue_cond.notify_one();
}

template <typename T>
bool FrameQueue<T>::Pop(std::vector<T>& data,
                        const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lock(queue_lock);

  if (queue.empty()) {
    if (queue_cond.wait_for(lock, timeout,
                            std::bind(&FrameQueue::QueueNotEmptyPred, this))) {
      return PopInternal(data);
    } else {
      return false;
    }
  } else {
    return PopInternal(data);
  }
}

template <typename T>
bool FrameQueue<T>::PopInternal(std::vector<T>& data) {
  data = queue.front();
  queue.pop();
  return true;
}

template <typename T>
bool FrameQueue<T>::QueueNotEmptyPred() {
  return !queue.empty();
}

OpenKinectDevice::OpenKinectDevice(freenect_context* ctx, int index)
    : Freenect::FreenectDevice(ctx, index) {
  setVideoFormat(FREENECT_VIDEO_RGB);
  setDepthFormat(FREENECT_DEPTH_11BIT);
  freenect_set_log_level(ctx, FREENECT_LOG_ERROR);
  depth_mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM,
                                        FREENECT_DEPTH_11BIT);
  video_mode =
      freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
}

void OpenKinectDevice::DepthCallback(void* _depth, uint32_t timestamp) {
  uint16_t* depth = static_cast<uint16_t*>(_depth);

  int frame_rect_size = depth_mode.width * depth_mode.height;
  std::vector<uint8_t> frame =
      ConvertDepthDataToGreyscale(depth, frame_rect_size);
  depth_frames.Push(frame);
}

void OpenKinectDevice::VideoCallback(void* _video, uint32_t timestamp) {
  uint8_t* video = static_cast<uint8_t*>(_video);

  int frame_rect_size = video_mode.width * video_mode.height;
  std::vector<uint8_t> frame = ConvertVideoDataToRGBA(video, frame_rect_size);
  video_frames.Push(frame);
}

bool OpenKinectDevice::GetNextDepthFrame(std::vector<uint8_t>& frame) {
  return depth_frames.Pop(frame, kLockTimeout);
}

bool OpenKinectDevice::GetNextVideoFrame(std::vector<uint8_t>& frame) {
  return video_frames.Pop(frame, kLockTimeout);
}

void OpenKinectDevice::StartDepth() { startDepth(); }

void OpenKinectDevice::StartVideo() { startVideo(); }

void OpenKinectDevice::StopDepth() { stopDepth(); }

void OpenKinectDevice::StopVideo() { stopDepth(); }

std::vector<uint8_t> OpenKinectDevice::ConvertDepthDataToGreyscale(
    uint16_t* depth, const int rect_size) {
  std::vector<uint8_t> frame(rect_size * 4);
  for (int i = 0; i < rect_size; i++) {
    uint8_t val = (uint8_t)depth[i];
    frame[i * 4] = 255 - val;
    frame[i * 4 + 1] = 255 - val;
    frame[i * 4 + 2] = 255 - val;
    frame[i * 4 + 3] = 255;
  }
  return frame;
}

std::vector<uint8_t> OpenKinectDevice::ConvertVideoDataToRGBA(
    uint8_t* video, const int rect_size) {
  std::vector<uint8_t> frame(rect_size * 4);
  for (int i = 0; i < rect_size; i++) {
    frame[i * 4] = video[i * 3];
    frame[i * 4 + 1] = video[i * 3 + 1];
    frame[i * 4 + 2] = video[i * 3 + 2];
    frame[i * 4 + 3] = 255;
  }
  return frame;
}

}  // namespace lptc_coderdojo