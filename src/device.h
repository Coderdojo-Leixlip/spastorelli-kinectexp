#ifndef LPTC_CODERDOJO_DEVICE_H_
#define LPTC_CODERDOJO_DEVICE_H_

#include <vector>

namespace lptc_coderdojo {

class KinectDeviceProxy {
 public:
  virtual ~KinectDeviceProxy() = default;

  virtual int GetDepthFrameRectSize() = 0;
  virtual int GetVideoFrameRectSize() = 0;
  virtual bool GetNextDepthFrame(std::vector<uint16_t>&) = 0;
  virtual bool GetNextVideoFrame(std::vector<uint8_t>&) = 0;
  virtual void StartVideo() = 0;
  virtual void StartDepth() = 0;
  virtual void StopVideo() = 0;
  virtual void StopDepth() = 0;
};

KinectDeviceProxy* CreateKinectDeviceProxy(int index);

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_DEVICE_H_
