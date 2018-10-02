#ifndef LPTC_CODERDOJO_DEVICE_H_
#define LPTC_CODERDOJO_DEVICE_H_

#include "libfreenect.hpp"

#include <mutex>
#include <queue>
#include <vector>

namespace lptc_coderdojo {

template <typename T>
class FrameQueue {
 public:
  FrameQueue();

  void Push(const std::vector<T>& data);
  std::vector<T> Pop(const std::chrono::milliseconds& timeout);

 private:
  std::vector<T> PopInternal();
  bool QueueNotEmptyPred();

  std::queue<std::vector<T>> queue;
  std::mutex queue_lock;
  std::condition_variable queue_cond;
};

class KinectDevice {
 public:
  KinectDevice() = default;
  virtual ~KinectDevice() = default;

  virtual std::vector<uint16_t> GetNextDepthFrame() = 0;
  virtual std::vector<uint8_t> GetNextVideoRGBAFrame() = 0;
  virtual void StartVideo() = 0;
  virtual void StartDepth() = 0;
  virtual void StopVideo() = 0;
  virtual void StopDepth() = 0;
};

class OpenKinectDevice : public KinectDevice, public Freenect::FreenectDevice {
 public:
  OpenKinectDevice(freenect_context* ctx, int index);

  void DepthCallback(void* _depth, uint32_t timestamp);
  void VideoCallback(void* _rgb, uint32_t timestamp);

  std::vector<uint16_t> GetNextDepthFrame();
  std::vector<uint8_t> GetNextVideoRGBAFrame();
  void StartDepth();
  void StartVideo();
  void StopDepth();
  void StopVideo();

 private:
  std::vector<uint8_t> ConvertToRGBAFrame(uint8_t* rgb, const int rect_size);

  freenect_frame_mode depth_mode;
  freenect_frame_mode video_mode;

  FrameQueue<uint16_t> depth_frames;
  FrameQueue<uint8_t> video_frames;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_DEVICE_H_
