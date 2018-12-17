#include "device.h"

#include <functional>

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
  return false;
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
  int len = GetDepthFrameRectSize();
  std::vector<uint16_t> buf;
  buf.resize(len);
  std::copy(depth, depth + len, buf.begin());
  depth_frames.Push(buf);
}

void OpenKinectDevice::VideoCallback(void* _video, uint32_t timestamp) {
  uint8_t* video = static_cast<uint8_t*>(_video);
  int len = GetVideoFrameRectSize() * 3;
  std::vector<uint8_t> buf;
  buf.resize(len);
  std::copy(video, video + len, buf.begin());
  video_frames.Push(buf);
}

int OpenKinectDevice::GetDepthFrameRectSize() {
  return depth_mode.width * depth_mode.height;
}

int OpenKinectDevice::GetVideoFrameRectSize() {
  return video_mode.width * video_mode.height;
}

bool OpenKinectDevice::GetNextDepthFrame(std::vector<uint16_t>& frame) {
  return depth_frames.Pop(frame, kLockTimeout);
}

bool OpenKinectDevice::GetNextVideoFrame(std::vector<uint8_t>& frame) {
  return video_frames.Pop(frame, kLockTimeout);
}

void OpenKinectDevice::StartDepth() { startDepth(); }

void OpenKinectDevice::StartVideo() { startVideo(); }

void OpenKinectDevice::StopDepth() { stopDepth(); }

void OpenKinectDevice::StopVideo() { stopVideo(); }

}  // namespace lptc_coderdojo