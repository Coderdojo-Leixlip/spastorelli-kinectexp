#include "server.h"

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

Channel::Channel(const std::string& t, AsioServer& s) : topic(t), server(s) {}
Channel::Channel(const Channel& ch) : topic(ch.topic), server(ch.server) {
  std::lock_guard<std::mutex> guard(subscribers_lock);
  subscribers = ch.subscribers;
}

const std::string& Channel::GetTopic() const { return topic; }

void Channel::Publish(void const* data, size_t len) {
  std::lock_guard<std::mutex> guard(subscribers_lock);

  if (subscribers.empty()) {
    return;
  }

  ConnectionSet::iterator iter;
  for (iter = subscribers.begin(); iter != subscribers.end(); ++iter) {
    try {
      server.send(*iter, data, len, websocketpp::frame::opcode::binary);
    } catch (websocketpp::exception const& e) {
      std::cerr << "!!!Error: " << e.m_msg << std::endl;
    }
  }
}

void Channel::Subscribe(websocketpp::connection_hdl hdl) {
  std::lock_guard<std::mutex> guard(subscribers_lock);
  subscribers.insert(hdl);
}

void Channel::Unsubscribe(websocketpp::connection_hdl hdl) {
  std::lock_guard<std::mutex> guard(subscribers_lock);
  subscribers.erase(hdl);
}

BroadcastServer::BroadcastServer(lptc_coderdojo::KinectDevice& _device,
                                 const int _port)
    : port(_port), device(_device) {
  s.clear_access_channels(websocketpp::log::alevel::all);
  s.init_asio();
  s.set_close_handler(std::bind(&BroadcastServer::OnConnectionClosed, this,
                                std::placeholders::_1));
  s.set_message_handler(std::bind(&BroadcastServer::OnMessage, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
}

void BroadcastServer::BroadcastToChannel(const std::string ch_name,
                                         DataProducer producer) {
  std::cout << "Broadcasting to `" << ch_name << "` channel..." << std::endl;
  Channel* ch = GetChannel(ch_name);
  if (!ch) {
    std::cerr << "!!!Error: no matching channel." << std::endl;
    return;
  }

  while (1) {
    std::vector<uint8_t> frame;
    if (producer(frame) && !frame.empty()) {
      ch->Publish(frame.data(), frame.size());
    }
  }
}

Channel* BroadcastServer::GetChannel(const std::string& topic) {
  ChannelMap::iterator search = channels.find(topic);
  if (search != channels.end()) return &search->second;

  return NULL;
}

void BroadcastServer::OnConnectionClosed(websocketpp::connection_hdl hdl) {
  ChannelMap::iterator iter = channels.begin();
  for (iter = channels.begin(); iter != channels.end(); ++iter) {
    iter->second.Unsubscribe(hdl);
  }
}

void BroadcastServer::OnMessage(websocketpp::connection_hdl hdl,
                                AsioServer::message_ptr msg) {
  Command cmd = Command::FromMessagePayload(msg->get_payload());
  Command::Action action = cmd.GetAction();

  if (action == Command::Action::INVALID) {
    s.send(hdl, "Error: invalid command provided.",
           websocketpp::frame::opcode::text);
    return;
  }

  Channel* ch = GetChannel(cmd.GetTopic());
  if (!ch) {
    s.send(hdl, "Error: no matching channel.",
           websocketpp::frame::opcode::text);
    return;
  }

  if (action == Command::Action::SUBSCRIBE) {
    ch->Subscribe(hdl);
  } else if (action == Command::Action::UNSUBSCRIBE) {
    ch->Unsubscribe(hdl);
  }
}

void BroadcastServer::RegisterChannel(const std::string& name) {
  Channel ch(name, s);
  channels.insert(ChannelMap::value_type(ch.GetTopic(), ch));
}

void BroadcastServer::Run() {
  s.listen(port);
  s.start_accept();
  std::cout << "Listening on port " << port << "..." << std::endl;
  std::cout << "Started Kinect BroadcastServer." << std::endl;

  device.StartVideo();
  RegisterChannel("video");
  DataProducer video_prod = [&](std::vector<uint8_t>& frame) -> bool {
    return device.GetNextVideoFrame(frame);
  };
  std::thread video_broadcast_thread(std::bind(
      &BroadcastServer::BroadcastToChannel, this, "video", video_prod));

  device.StartDepth();
  RegisterChannel("depth");
  DataProducer depth_prod = [&](std::vector<uint8_t>& frame) -> bool {
    return device.GetNextDepthFrame(frame);
  };
  std::thread depth_broadcast_thread(std::bind(
      &BroadcastServer::BroadcastToChannel, this, "depth", depth_prod));

  s.run();
  video_broadcast_thread.join();
  depth_broadcast_thread.join();
}
}  // namespace lptc_coderdojo