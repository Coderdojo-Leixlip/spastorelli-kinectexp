#include "openni2_device.h"

namespace lptc_coderdojo {

const std::chrono::milliseconds kLockTimeout = std::chrono::milliseconds(30);

OpenNI2FrameListener::OpenNI2FrameListener(FrameQueue<uint16_t>& depth_f,
                                           FrameQueue<uint8_t>& video_f)
    : depth_frames(depth_f), video_frames(video_f) {}

void OpenNI2FrameListener::onNewFrame(openni::VideoStream& stream) {
  openni::VideoFrameRef frame;
  stream.readFrame(&frame);

  if (frame.isValid()) {
    const openni::VideoMode& video_mode = frame.getVideoMode();
    switch (video_mode.getPixelFormat()) {
      case openni::PIXEL_FORMAT_DEPTH_1_MM: {
        const uint16_t* depth = static_cast<const uint16_t*>(frame.getData());
        int size = frame.getDataSize() / 2;
        if (depth_buf.size() != size) {
          depth_buf.resize(size);
        }
        std::copy(depth, depth + size, depth_buf.begin());
        depth_frames.Push(depth_buf);
        break;
      }
      case openni::PIXEL_FORMAT_RGB888: {
        const uint8_t* video = static_cast<const uint8_t*>(frame.getData());
        int size = frame.getDataSize();
        if (video_buf.size() != size) {
          video_buf.resize(size);
        }
        std::copy(video, video + size, video_buf.begin());
        video_frames.Push(video_buf);
        break;
      }
      default:
        throw KinectDeviceException("OpenNI2FrameListener::onNewFrame",
                                    "Pixel format not supported");
        break;
    }
  }
}

OpenNI2DeviceProxy::OpenNI2DeviceProxy(const std::string& uri) throw(
    KinectDeviceException)
    : device(),
      depth_frame_listener(depth_frames, video_frames),
      video_frame_listener(depth_frames, video_frames),
      depth_started(false),
      video_started(false) {
  openni::Status rc = openni::STATUS_OK;
  rc = openni::OpenNI::initialize();

  if (rc != openni::STATUS_OK) {
    throw KinectDeviceException("OpenNI2 initialization",
                                openni::OpenNI::getExtendedError());
  }

  if (uri.length() > 0) {
    rc = device.open(uri.c_str());
  } else {
    rc = device.open(openni::ANY_DEVICE);
  }

  if (rc != openni::STATUS_OK) {
    throw KinectDeviceException("OpenNI2 Device::open",
                                openni::OpenNI::getExtendedError());
  }
}

OpenNI2DeviceProxy::~OpenNI2DeviceProxy() {
  StopVideo();
  StopDepth();

  device.close();
  openni::OpenNI::shutdown();
}

int OpenNI2DeviceProxy::GetDepthFrameRectSize() {
  return depth_stream.getVideoMode().getResolutionX() *
         depth_stream.getVideoMode().getResolutionY();
}

int OpenNI2DeviceProxy::GetVideoFrameRectSize() {
  return video_stream.getVideoMode().getResolutionX() *
         video_stream.getVideoMode().getResolutionY();
}

bool OpenNI2DeviceProxy::GetNextDepthFrame(std::vector<uint16_t>& frame) {
  return depth_frames.Pop(frame, kLockTimeout);
}

bool OpenNI2DeviceProxy::GetNextVideoFrame(std::vector<uint8_t>& frame) {
  return video_frames.Pop(frame, kLockTimeout);
}

void OpenNI2DeviceProxy::StartDepth() throw(KinectDeviceException) {
  if (!depth_started) {
    if (!device.hasSensor(openni::SENSOR_DEPTH)) {
      throw KinectDeviceException("OpenNI2DeviceProxy::StartDepth",
                                  "Video sensor type not defined for device");
    }

    const openni::Status rc = depth_stream.create(device, openni::SENSOR_DEPTH);
    if (rc != openni::STATUS_OK) {
      throw KinectDeviceException("OpenNI2 depth_stream::create",
                                  openni::OpenNI::getExtendedError());
    }

    depth_stream.start();
    depth_stream.addNewFrameListener(&depth_frame_listener);
    depth_started = true;
  }
}

void OpenNI2DeviceProxy::StartVideo() throw(KinectDeviceException) {
  if (!video_started) {
    if (!device.hasSensor(openni::SENSOR_COLOR)) {
      throw KinectDeviceException("OpenNI2DeviceProxy::StartVideo",
                                  "Video sensor type not defined for device");
    }

    const openni::Status rc = video_stream.create(device, openni::SENSOR_COLOR);
    if (rc != openni::STATUS_OK) {
      throw KinectDeviceException("OpenNI2 video_stream::create",
                                  openni::OpenNI::getExtendedError());
    }

    video_stream.start();
    video_stream.addNewFrameListener(&video_frame_listener);
    video_started = true;
  }
}

void OpenNI2DeviceProxy::StopDepth() {
  if (depth_started) {
    depth_stream.removeNewFrameListener(&depth_frame_listener);
    depth_stream.stop();
    depth_started = false;
  }
}

void OpenNI2DeviceProxy::StopVideo() {
  if (video_started) {
    video_stream.removeNewFrameListener(&video_frame_listener);
    video_stream.stop();
    video_started = false;
  }
}

std::unique_ptr<KinectDeviceProxy> CreateKinectDeviceProxy(int index) throw(
    KinectDeviceException) {
  const std::string uri = "freenect://" + std::to_string(index);
  return std::unique_ptr<KinectDeviceProxy>(new OpenNI2DeviceProxy(uri));
}

}  // namespace lptc_coderdojo