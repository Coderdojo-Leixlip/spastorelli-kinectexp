#ifndef LPTC_CODERDOJO_OPENKINECT_DEVICE_H_
#define LPTC_CODERDOJO_OPENKINECT_DEVICE_H_

#include "device.h"
#include "frame_queue.h"

#include "libfreenect.hpp"

namespace lptc_coderdojo {

class OpenKinectDevice : public Freenect::FreenectDevice {
 public:
  OpenKinectDevice(freenect_context* ctx, int index);

  void DepthCallback(void* _depth, uint32_t timestamp);
  void VideoCallback(void* _rgb, uint32_t timestamp);

  int GetDepthFrameRectSize();
  int GetVideoFrameRectSize();
  FrameQueue<uint16_t>& GetDepthFrameQueue();
  FrameQueue<uint8_t>& GetVideoFrameQueue();

 private:
  freenect_frame_mode depth_mode;
  freenect_frame_mode video_mode;

  FrameQueue<uint16_t> depth_frames;
  std::vector<uint16_t> depth_buf;

  FrameQueue<uint8_t> video_frames;
  std::vector<uint8_t> video_buf;
};

class OpenKinectDeviceProxy : public KinectDeviceProxy {
 public:
  OpenKinectDeviceProxy(int index);

  int GetDepthFrameRectSize();
  int GetVideoFrameRectSize();
  bool GetNextDepthFrame(std::vector<uint16_t>&);
  bool GetNextVideoFrame(std::vector<uint8_t>&);
  void StartDepth();
  void StartVideo();
  void StopDepth();
  void StopVideo();

 private:
  Freenect::Freenect freenect;
  OpenKinectDevice* device;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_OPENKINECT_DEVICE_H_