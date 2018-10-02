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
std::vector<T> FrameQueue<T>::Pop(const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lock(queue_lock);

  if (queue.empty()) {
    if (queue_cond.wait_for(lock, timeout,
                            std::bind(&FrameQueue::QueueNotEmptyPred, this))) {
      return PopInternal();
    } else {
      return std::vector<T>();
    }
  } else {
    return PopInternal();
  }
}

template <typename T>
std::vector<T> FrameQueue<T>::PopInternal() {
  std::vector<T> data = queue.front();
  queue.pop();
  return data;
}

template <typename T>
bool FrameQueue<T>::QueueNotEmptyPred() {
  return !queue.empty();
}

OpenKinectDevice::OpenKinectDevice(freenect_context* ctx, int index)
    : Freenect::FreenectDevice(ctx, index) {
  setVideoFormat(FREENECT_VIDEO_RGB);
  setDepthFormat(FREENECT_DEPTH_REGISTERED);
  depth_mode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM,
                                        FREENECT_DEPTH_REGISTERED);
  video_mode =
      freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB);
}

void OpenKinectDevice::DepthCallback(void* _depth, uint32_t timestamp) {
  std::vector<uint16_t> frame(depth_mode.bytes / 2);
  uint16_t* depth = static_cast<uint16_t*>(_depth);

  int depthBufSize = getDepthBufferSize();
  std::copy(depth, depth + depthBufSize / 2, frame.begin());
  depth_frames.Push(frame);
}

void OpenKinectDevice::VideoCallback(void* _rgb, uint32_t timestamp) {
  uint8_t* rgb = static_cast<uint8_t*>(_rgb);

  int frame_rect_size = video_mode.width * video_mode.height;
  std::vector<uint8_t> frame = ConvertToRGBAFrame(rgb, frame_rect_size);
  video_frames.Push(frame);
}

std::vector<uint16_t> OpenKinectDevice::GetNextDepthFrame() {
  return depth_frames.Pop(kLockTimeout);
}

std::vector<uint8_t> OpenKinectDevice::GetNextVideoRGBAFrame() {
  return video_frames.Pop(kLockTimeout);
}

void OpenKinectDevice::StartDepth() { startDepth(); }

void OpenKinectDevice::StartVideo() { startVideo(); }

void OpenKinectDevice::StopDepth() { stopDepth(); }

void OpenKinectDevice::StopVideo() { stopDepth(); }

std::vector<uint8_t> OpenKinectDevice::ConvertToRGBAFrame(uint8_t* rgb, const int rect_size) {
  std::vector<uint8_t> frame(rect_size * 4);
  for (int i = 0; i < rect_size; i++) {
    frame[i * 4] = rgb[i * 3];
    frame[i * 4 + 1] = rgb[i * 3 + 1];
    frame[i * 4 + 2] = rgb[i * 3 + 2];
    frame[i * 4 + 3] = 255;
  }
  return frame;
}

}  // namespace lptc_coderdojo