#include "openkinect_device.h"

#include <memory>

namespace lptc_coderdojo {

const std::chrono::milliseconds kLockTimeout = std::chrono::milliseconds(30);

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
  int size = GetDepthFrameRectSize();
  if (depth_buf.size() != size) {
    depth_buf.resize(size);
  }
  std::copy(depth, depth + size, depth_buf.begin());
  depth_frames.Push(depth_buf);
}

void OpenKinectDevice::VideoCallback(void* _video, uint32_t timestamp) {
  uint8_t* video = static_cast<uint8_t*>(_video);
  int size = GetVideoFrameRectSize() * 3;
  if (video_buf.size() != size) {
    video_buf.resize(size);
  }
  std::copy(video, video + size, video_buf.begin());
  video_frames.Push(video_buf);
}

int OpenKinectDevice::GetDepthFrameRectSize() {
  return depth_mode.width * depth_mode.height;
}

int OpenKinectDevice::GetVideoFrameRectSize() {
  return video_mode.width * video_mode.height;
}

FrameQueue<uint16_t>& OpenKinectDevice::GetDepthFrameQueue() {
  return depth_frames;
}

FrameQueue<uint8_t>& OpenKinectDevice::GetVideoFrameQueue() {
  return video_frames;
}

OpenKinectDeviceProxy::OpenKinectDeviceProxy(int index) throw(
    KinectDeviceException) {
  try {
    device = &freenect.createDevice<OpenKinectDevice>(index);
  } catch (const std::runtime_error& e) {
    throw KinectDeviceException("OpenKinect device creation", e.what());
  }
}

int OpenKinectDeviceProxy::GetDepthFrameRectSize() {
  return device->GetDepthFrameRectSize();
}

int OpenKinectDeviceProxy::GetVideoFrameRectSize() {
  return device->GetVideoFrameRectSize();
}

bool OpenKinectDeviceProxy::GetNextDepthFrame(std::vector<uint16_t>& frame) {
  return device->GetDepthFrameQueue().Pop(frame, kLockTimeout);
}

bool OpenKinectDeviceProxy::GetNextVideoFrame(std::vector<uint8_t>& frame) {
  return device->GetVideoFrameQueue().Pop(frame, kLockTimeout);
}

void OpenKinectDeviceProxy::StartDepth() throw(KinectDeviceException) {
  try {
    device->startDepth();
  } catch (const std::runtime_error& e) {
    throw KinectDeviceException("OpenKinectDeviceProxy::StartDepth", e.what());
  }
}

void OpenKinectDeviceProxy::StartVideo() throw(KinectDeviceException) {
  try {
    device->startVideo();
  } catch (const std::runtime_error& e) {
    throw KinectDeviceException("OpenKinectDeviceProxy::StartVideo", e.what());
  }
}

void OpenKinectDeviceProxy::StopDepth() throw(KinectDeviceException) {
  try {
    device->stopDepth();
  } catch (const std::runtime_error& e) {
    throw KinectDeviceException("OpenKinectDeviceProxy::StopDepth", e.what());
  }
}

void OpenKinectDeviceProxy::StopVideo() throw(KinectDeviceException) {
  try {
    device->stopVideo();
  } catch (const std::runtime_error& e) {
    throw KinectDeviceException("OpenKinectDeviceProxy::StopVideo", e.what());
  }
}

std::unique_ptr<KinectDeviceProxy> CreateKinectDeviceProxy(int index) throw(
    KinectDeviceException) {
  return std::unique_ptr<KinectDeviceProxy>(new OpenKinectDeviceProxy(index));
}

}  // namespace lptc_coderdojo