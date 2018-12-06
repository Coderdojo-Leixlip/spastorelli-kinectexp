#ifndef LPTC_CODERDOJO_PUBLISHER_H_
#define LPTC_CODERDOJO_PUBLISHER_H_

#include "channel.h"
#include "device.h"

namespace lptc_coderdojo {

class Publisher {
 public:
  Publisher() = default;
  virtual ~Publisher() = default;

  virtual void PublishNewData(lptc_coderdojo::Channel* channel) = 0;
};

class DepthDataPublisher : public Publisher {
 public:
  DepthDataPublisher(lptc_coderdojo::KinectDevice& _device);

  void PublishNewData(lptc_coderdojo::Channel* channel);
  void Transform();

 private:
  lptc_coderdojo::KinectDevice& device;
  std::vector<uint16_t> buf;
  std::vector<uint8_t> frame;
};

class VideoDataPublisher : public Publisher {
 public:
  VideoDataPublisher(lptc_coderdojo::KinectDevice& _device);

  void PublishNewData(lptc_coderdojo::Channel* channel);
  void Transform();

 private:
  lptc_coderdojo::KinectDevice& device;
  std::vector<uint8_t> buf;
  std::vector<uint8_t> frame;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_PUBLISHER_H_