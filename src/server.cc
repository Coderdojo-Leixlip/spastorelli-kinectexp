#include "server.h"
#include "../protocol/protocol_generated.h"

#include <flatbuffers/flatbuffers.h>

namespace lptc_coderdojo {

Command::Command(Action a) : action(a) {}
Command::Command(Action a, const std::string t) : action(a), topic(t) {}

const Command::Action& Command::GetAction() const { return action; }
const std::string& Command::GetTopic() const { return topic; }

std::string Command::ActionStr(Action a) {
  switch (a) {
    case Action::SUBSCRIBE:
      return "SUBSCRIBE";
    case Action::UNSUBSCRIBE:
      return "UNSUBSCRIBE";
    case Action::INVALID:
    default:
      return "INVALID";
  }
}

Command::Action Command::ActionFromToken(const std::string& token) {
  if (token.compare(ActionStr(Action::SUBSCRIBE)) == 0) {
    return Action::SUBSCRIBE;
  } else if (token.compare(ActionStr(Action::UNSUBSCRIBE)) == 0) {
    return Action::UNSUBSCRIBE;
  } else {
    return Action::INVALID;
  }
}

std::vector<std::string> Command::GetTokensFromPayload(
    const std::string& msg_payload) {
  std::istringstream iss(msg_payload);
  std::vector<std::string> tokens;
  std::string token;

  while (std::getline(iss, token, ' '))
    if (!token.empty()) tokens.push_back(token);

  return tokens;
}

Command Command::FromMessagePayload(const std::string& msg) {
  std::vector<std::string> tokens = GetTokensFromPayload(msg);

  if (tokens.size() != 2 || tokens[0].length() == 0 || tokens[1].length() == 0)
    return Command(Action::INVALID);

  return Command(ActionFromToken(tokens[0]), tokens[1]);
}

BroadcastServer::BroadcastServer(lptc_coderdojo::KinectDevice& _device,
                                 const int _port)
    : port(_port), device(_device) {
  s.clear_access_channels(websocketpp::log::alevel::all);
  s.init_asio();
  s.set_open_handler(std::bind(&BroadcastServer::OnConnectionOpened, this,
                               std::placeholders::_1));
  s.set_close_handler(std::bind(&BroadcastServer::OnConnectionClosed, this,
                                std::placeholders::_1));
  s.set_message_handler(std::bind(&BroadcastServer::OnMessage, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
}

void BroadcastServer::BroadcastToChannel(const std::string ch_name,
                                         lptc_coderdojo::Publisher& publisher) {
  std::cout << "Broadcasting to `" << ch_name << "` channel..." << std::endl;
  lptc_coderdojo::Channel* ch = GetChannel(ch_name);
  if (!ch) {
    std::cerr << "!!!Error: no matching channel." << std::endl;
    return;
  }

  while (term_future.wait_for(std::chrono::microseconds(1)) ==
         std::future_status::timeout) {
    publisher.PublishNewData(ch);
  }
  std::cout << "Stopped broadcasting to `" << ch_name << "` channel."
            << std::endl;
}

void BroadcastServer::CloseConnections(const std::string& reason) {
  std::lock_guard<std::mutex> guard(connections_lock);

  ConnectionSet::iterator iter;
  for (iter = connections.begin(); iter != connections.end(); ++iter) {
    try {
      AsioServer::connection_ptr conn = s.get_con_from_hdl(*iter);
      conn->close(websocketpp::close::status::going_away, reason);
    } catch (websocketpp::exception const& e) {
      std::cerr << "!!!Error: " << e.m_msg << std::endl;
    }
  }
}

lptc_coderdojo::Channel* BroadcastServer::GetChannel(const std::string& topic) {
  ChannelMap::iterator search = channels.find(topic);
  if (search != channels.end()) return &search->second;

  return NULL;
}

void BroadcastServer::OnConnectionClosed(websocketpp::connection_hdl hdl) {
  ChannelMap::iterator iter = channels.begin();
  for (iter = channels.begin(); iter != channels.end(); ++iter) {
    iter->second.Unsubscribe(hdl);
  }

  std::lock_guard<std::mutex> guard(connections_lock);
  connections.erase(hdl);
}

void BroadcastServer::OnConnectionOpened(websocketpp::connection_hdl hdl) {
  std::lock_guard<std::mutex> guard(connections_lock);
  connections.insert(hdl);
}

void BroadcastServer::OnMessage(websocketpp::connection_hdl hdl,
                                AsioServer::message_ptr msg) {
  Command cmd = Command::FromMessagePayload(msg->get_payload());
  Command::Action action = cmd.GetAction();

  if (action == Command::Action::INVALID) {
    SendErrorMessage(hdl, "Invalid command provided.");
    return;
  }

  lptc_coderdojo::Channel* ch = GetChannel(cmd.GetTopic());
  if (!ch) {
    SendErrorMessage(hdl, "No matching channel.");
    return;
  }

  if (action == Command::Action::SUBSCRIBE) {
    ch->Subscribe(hdl);
  } else if (action == Command::Action::UNSUBSCRIBE) {
    ch->Unsubscribe(hdl);
  }
}

void BroadcastServer::RegisterChannel(const std::string& name) {
  lptc_coderdojo::Channel ch(name, s);
  channels.insert(ChannelMap::value_type(ch.GetTopic(), ch));
}

void BroadcastServer::StopAllChannelBroadcasts() { term_sig.set_value(); }

void BroadcastServer::SendErrorMessage(websocketpp::connection_hdl hdl,
                                       const std::string& error_msg) {
  flatbuffers::FlatBufferBuilder builder;
  flatbuffers::Offset<flatbuffers::String> error =
      builder.CreateString(error_msg);

  lptc_coderdojo::protocol::MessageBuilder msg_builder(builder);
  msg_builder.add_type(lptc_coderdojo::protocol::MessageType::Error);
  msg_builder.add_error(error);
  msg_builder.add_timestamp(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  flatbuffers::Offset<lptc_coderdojo::protocol::Message> msg =
      msg_builder.Finish();
  builder.Finish(msg);

  s.send(hdl, builder.GetBufferPointer(), builder.GetSize(),
         websocketpp::frame::opcode::binary);
}

void BroadcastServer::Run() {
  s.listen(port);
  s.start_accept();
  std::cout << "Listening on port " << port << "..." << std::endl;
  std::cout << "Started Kinect BroadcastServer." << std::endl;

  term_future = term_sig.get_future();

  device.StartVideo();
  RegisterChannel("video");
  lptc_coderdojo::VideoDataPublisher video_pub(device);
  std::thread video_broadcast_thread(std::bind(
      &BroadcastServer::BroadcastToChannel, this, "video", video_pub));

  device.StartDepth();
  RegisterChannel("depth");
  lptc_coderdojo::DepthDataPublisher depth_pub(device);
  std::thread depth_broadcast_thread(std::bind(
      &BroadcastServer::BroadcastToChannel, this, "depth", depth_pub));

  s.run();
  video_broadcast_thread.join();
  depth_broadcast_thread.join();
}

void BroadcastServer::Stop() {
  std::cout << "Shutting down BroadcastServer...." << std::endl;
  s.stop_listening();

  device.StopVideo();
  device.StopDepth();

  std::cout << "Closing connections..." << std::endl;
  CloseConnections("Goodbye!");
  std::cout << "Stopping channel broadcasts..." << std::endl;
  StopAllChannelBroadcasts();
}

}  // namespace lptc_coderdojo