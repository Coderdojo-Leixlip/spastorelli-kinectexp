#ifndef LPTC_CODERDOJO_OPENNI2_DEVICE_H_
#define LPTC_CODERDOJO_OPENNI2_DEVICE_H_

#include "device.h"
#include "frame_queue.h"

#include "OpenNI.h"

namespace lptc_coderdojo {

class OpenNI2FrameListener : public openni::VideoStream::NewFrameListener {
 public:
  OpenNI2FrameListener(FrameQueue<uint16_t>& depth_f,
                       FrameQueue<uint8_t>& video_f);
  virtual ~OpenNI2FrameListener() = default;
  void onNewFrame(openni::VideoStream& stream);

 private:
  FrameQueue<uint16_t>& depth_frames;
  std::vector<uint16_t> depth_buf;

  FrameQueue<uint8_t>& video_frames;
  std::vector<uint8_t> video_buf;
};

class OpenNI2DeviceProxy : public KinectDeviceProxy {
 public:
  OpenNI2DeviceProxy(const std::string& uri) throw(KinectDeviceException);
  ~OpenNI2DeviceProxy();

  int GetDepthFrameRectSize();
  int GetVideoFrameRectSize();
  bool GetNextDepthFrame(std::vector<uint16_t>&);
  bool GetNextVideoFrame(std::vector<uint8_t>&);
  void StartVideo() throw(KinectDeviceException);
  void StartDepth() throw(KinectDeviceException);
  void StopVideo();
  void StopDepth();

 private:
  openni::Device device;
  openni::VideoStream depth_stream;
  openni::VideoStream video_stream;
  OpenNI2FrameListener depth_frame_listener;
  OpenNI2FrameListener video_frame_listener;

  FrameQueue<uint16_t> depth_frames;
  FrameQueue<uint8_t> video_frames;

  bool depth_started;
  bool video_started;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_OPENNI2_DEVICE_H_