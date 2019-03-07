#ifndef LPTC_CODERDOJO_DEVICE_H_
#define LPTC_CODERDOJO_DEVICE_H_

#include <sstream>
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

class KinectDeviceException : public std::exception {
 public:
  KinectDeviceException(const std::string& ctx, const std::string& msg) throw()
      : context(ctx), message(msg) {
    std::stringstream sstream;
    sstream << context << " failed:" << std::endl << message << std::endl;
    description = sstream.str();
  }
  virtual ~KinectDeviceException() throw() {}

  const std::string& getContext() const throw() { return context; }
  const std::string& getMessage() const throw() { return message; }
  const char* what() const throw() { return description.c_str(); }

 private:
  std::string context;
  std::string message;
  std::string description;
};

std::unique_ptr<KinectDeviceProxy> CreateKinectDeviceProxy(int index) throw(
    KinectDeviceException);

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_DEVICE_H_
