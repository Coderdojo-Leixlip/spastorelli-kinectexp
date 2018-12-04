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
  bool Pop(std::vector<T>&, const std::chrono::milliseconds& timeout);

 private:
  bool PopInternal(std::vector<T>&);
  bool QueueNotEmptyPred();

  std::queue<std::vector<T>> queue;
  std::mutex queue_lock;
  std::condition_variable queue_cond;
};

class KinectDevice {
 public:
  KinectDevice() = default;
  virtual ~KinectDevice() = default;

  virtual int GetDepthFrameRectSize() = 0;
  virtual int GetVideoFrameRectSize() = 0;
  virtual bool GetNextDepthFrame(std::vector<uint16_t>&) = 0;
  virtual bool GetNextVideoFrame(std::vector<uint8_t>&) = 0;
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

#endif  // LPTC_CODERDOJO_DEVICE_H_
