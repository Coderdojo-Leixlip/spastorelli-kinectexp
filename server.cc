#include "device.h"

#include "libfreenect.hpp"

#include <iostream>
#include <set>
#include <sstream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> AsioServer;

class Command {
 public:
  enum Action { SUBSCRIBE, UNSUBSCRIBE, INVALID };

  Command(Action a) : action(a) {}
  Command(Action a, const std::string t) : action(a), topic(t) {}

  const Action& GetAction() const { return action; }
  const std::string& GetTopic() const { return topic; }

  static std::string ActionStr(Action a) {
    switch (a) {
      case Action::SUBSCRIBE:
        return "SUBSCRIBE";
      case Action::UNSUBSCRIBE:
        return "UNSUBSCRIBE";
      case Action::INVALID:
        return "INVALID";
    }
  }

  static std::vector<std::string> GetTokensFromPayload(
      std::string msg_payload) {
    std::istringstream iss(msg_payload);
    std::vector<std::string> tokens;
    std::string token;

    while (std::getline(iss, token, ' '))
      if (!token.empty()) tokens.push_back(token);

    return tokens;
  }

  static Action ActionFromToken(const std::string token) {
    if (token.compare(ActionStr(Action::SUBSCRIBE)) == 0) {
      return Action::SUBSCRIBE;
    } else if (token.compare(ActionStr(Action::UNSUBSCRIBE)) == 0) {
      return Action::UNSUBSCRIBE;
    } else {
      return Action::INVALID;
    }
  }

  static Command FromMessage(AsioServer::message_ptr msg) {
    std::string m = msg->get_payload();
    std::vector<std::string> tokens = GetTokensFromPayload(m);

    if (tokens.size() != 2 || tokens[0].length() == 0 ||
        tokens[1].length() == 0)
      return Command(Action::INVALID);

    return Command(ActionFromToken(tokens[0]), tokens[1]);
  }

 private:
  Action action;
  std::string topic;
};

class Channel {
 public:
  Channel(const std::string& t, AsioServer& s) : topic(t), server(s) {}
  Channel(const Channel& ch) : topic(ch.topic), server(ch.server) {
    std::lock_guard<std::mutex> guard(subscribers_lock);
    subscribers = ch.subscribers;
  }

  const std::string& GetTopic() const { return topic; }

  void Publish(void const* data, size_t len) {
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

  void Subscribe(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(subscribers_lock);
    subscribers.insert(hdl);
  }

  void Unsubscribe(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(subscribers_lock);
    subscribers.erase(hdl);
  }

 private:
  typedef std::set<websocketpp::connection_hdl,
                   std::owner_less<websocketpp::connection_hdl>>
      ConnectionSet;

  std::string topic;
  ConnectionSet subscribers;
  std::mutex subscribers_lock;
  AsioServer& server;
};

typedef std::function<bool(std::vector<uint8_t>&)> DataProducer;

class DeviceBroadcastServer {
 public:
  DeviceBroadcastServer(lptc_coderdojo::KinectDevice& _device, const int _port)
      : port(_port), device(_device) {
    s.clear_access_channels(websocketpp::log::alevel::all);
    s.init_asio();
    s.set_close_handler(std::bind(&DeviceBroadcastServer::OnConnectionClosed,
                                  this, std::placeholders::_1));
    s.set_message_handler(std::bind(&DeviceBroadcastServer::OnMessage, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2));
  }

  void BroadcastToChannel(const std::string ch_name, DataProducer producer) {
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

  void OnConnectionClosed(websocketpp::connection_hdl hdl) {
    ChannelMap::iterator iter = channels.begin();
    for (iter = channels.begin(); iter != channels.end(); ++iter) {
      iter->second.Unsubscribe(hdl);
    }
  }

  void OnMessage(websocketpp::connection_hdl hdl, AsioServer::message_ptr msg) {
    Command cmd = Command::FromMessage(msg);
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

  Channel* GetChannel(const std::string& topic) {
    ChannelMap::iterator search = channels.find(topic);
    if (search != channels.end()) return &search->second;

    return NULL;
  }

  void RegisterChannel(const std::string& name) {
    Channel ch(name, s);
    channels.insert(ChannelMap::value_type(ch.GetTopic(), ch));
  }

  void run() {
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
        &DeviceBroadcastServer::BroadcastToChannel, this, "video", video_prod));

    device.StartDepth();
    RegisterChannel("depth");
    DataProducer depth_prod = [&](std::vector<uint8_t>& frame) -> bool {
      return device.GetNextDepthFrame(frame);
    };
    std::thread depth_broadcast_thread(std::bind(
        &DeviceBroadcastServer::BroadcastToChannel, this, "depth", depth_prod));

    s.run();
    video_broadcast_thread.join();
    depth_broadcast_thread.join();
  }

 private:
  typedef std::map<std::string, Channel> ChannelMap;

  const int port;
  AsioServer s;
  lptc_coderdojo::KinectDevice& device;
  ChannelMap channels;
};

int main() {
  Freenect::Freenect freenect;
  lptc_coderdojo::KinectDevice& device =
      freenect.createDevice<lptc_coderdojo::OpenKinectDevice>(0);

  DeviceBroadcastServer s(device, 9002);
  s.run();
}