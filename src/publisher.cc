#include "publisher.h"
#include "../protocol/protocol_generated.h"

#include <flatbuffers/flatbuffers.h>

namespace {

std::tuple<uint8_t*, size_t> SerializeMessage(
    flatbuffers::FlatBufferBuilder& builder, const std::vector<uint8_t>& frame,
    lptc_coderdojo::protocol::DataType type) {
  flatbuffers::Offset<flatbuffers::Vector<uint8_t>> data =
      builder.CreateVector(frame);

  lptc_coderdojo::protocol::DeviceDataBuilder dev_data_builder(builder);
  dev_data_builder.add_type(type);

  if (type == lptc_coderdojo::protocol::DataType::Depth) {
    dev_data_builder.add_depth(data);
  } else if (type == lptc_coderdojo::protocol::DataType::Video) {
    dev_data_builder.add_video(data);
  }
  flatbuffers::Offset<lptc_coderdojo::protocol::DeviceData> dev_data =
      dev_data_builder.Finish();
  builder.Finish(dev_data);

  lptc_coderdojo::protocol::MessageBuilder msg_builder(builder);
  msg_builder.add_type(lptc_coderdojo::protocol::MessageType::DeviceData);
  msg_builder.add_data(dev_data);
  msg_builder.add_timestamp(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  flatbuffers::Offset<lptc_coderdojo::protocol::Message> msg =
      msg_builder.Finish();
  builder.Finish(msg);

  return std::make_tuple(builder.GetBufferPointer(), builder.GetSize());
}

}  // namespace

namespace lptc_coderdojo {

DepthDataPublisher::DepthDataPublisher(
    lptc_coderdojo::KinectDeviceProxy& _device)
    : device(_device), frame(_device.GetDepthFrameRectSize() * 4) {}

void DepthDataPublisher::PublishNewData(lptc_coderdojo::Channel* channel) {
  if (!device.GetNextDepthFrame(buf)) return;

  Transform();

  flatbuffers::FlatBufferBuilder builder;
  SerializeMessage(builder, frame, lptc_coderdojo::protocol::DataType::Depth);
  channel->Publish(builder.GetBufferPointer(), builder.GetSize());
}

void DepthDataPublisher::Transform() {
  // resize to an RGBA frame;
  int rect_size = device.GetDepthFrameRectSize();
  for (int i = 0; i < rect_size; i++) {
    uint8_t val = (uint8_t)buf[i];
    frame[i * 4] = 255 - val;
    frame[i * 4 + 1] = 255 - val;
    frame[i * 4 + 2] = 255 - val;
    frame[i * 4 + 3] = 255;
  }
}

VideoDataPublisher::VideoDataPublisher(
    lptc_coderdojo::KinectDeviceProxy& _device)
    : device(_device), frame(_device.GetVideoFrameRectSize() * 4) {}

void VideoDataPublisher::PublishNewData(lptc_coderdojo::Channel* channel) {
  if (!device.GetNextVideoFrame(buf)) return;

  Transform();

  flatbuffers::FlatBufferBuilder builder;
  SerializeMessage(builder, frame, lptc_coderdojo::protocol::DataType::Video);
  channel->Publish(builder.GetBufferPointer(), builder.GetSize());
}

void VideoDataPublisher::Transform() {
  // resize to an RGBA frame;
  int rect_size = device.GetVideoFrameRectSize();
  for (int i = 0; i < rect_size; i++) {
    frame[i * 4] = buf[i * 3];
    frame[i * 4 + 1] = buf[i * 3 + 1];
    frame[i * 4 + 2] = buf[i * 3 + 2];
    frame[i * 4 + 3] = 255;
  }
}

}  // namespace lptc_coderdojo