#ifndef LPTC_CODERDOJO_OPENKINECT_DEVICE_H_
#define LPTC_CODERDOJO_OPENKINECT_DEVICE_H_

#include "device.h"
#include "frame_queue.h"

#include "libfreenect.hpp"

namespace lptc_coderdojo {

class OpenKinectDevice : public KinectDevice, public Freenect::FreenectDevice {
 public:
  OpenKinectDevice(freenect_context* ctx, int index);

  void DepthCallback(void* _depth, uint32_t timestamp);
  void VideoCallback(void* _rgb, uint32_t timestamp);

  int GetDepthFrameRectSize();
  int GetVideoFrameRectSize();
  bool GetNextDepthFrame(std::vector<uint16_t>&);
  bool GetNextVideoFrame(std::vector<uint8_t>&);
  void StartDepth();
  void StartVideo();
  void StopDepth();
  void StopVideo();

 private:
  freenect_frame_mode depth_mode;
  freenect_frame_mode video_mode;

  FrameQueue<uint16_t> depth_frames;
  FrameQueue<uint8_t> video_frames;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_OPENKINECT_DEVICE_H_