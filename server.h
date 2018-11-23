#ifndef LPTC_CODERDOJO_SERVER_H_
#define LPTC_CODERDOJO_SERVER_H_

#include "device.h"

#include <iostream>
#include <set>
#include <vector>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace lptc_coderdojo {

typedef websocketpp::server<websocketpp::config::asio> AsioServer;

class Command {
 public:
  enum Action { SUBSCRIBE, UNSUBSCRIBE, INVALID };

  Command(Action a);
  Command(Action a, const std::string t);

  const Action& GetAction() const;
  const std::string& GetTopic() const;

  static std::string ActionStr(Action a);
  static Action ActionFromToken(const std::string& token);
  static std::vector<std::string> GetTokensFromPayload(
      const std::string& msg_payload);
  static Command FromMessagePayload(const std::string& msg);

 private:
  Action action;
  std::string topic;
};

class Channel {
 public:
  Channel(const std::string& t, AsioServer& s);
  Channel(const Channel& ch);

  const std::string& GetTopic() const;

  void Publish(void const* data, size_t len);
  void Subscribe(websocketpp::connection_hdl hdl);
  void Unsubscribe(websocketpp::connection_hdl hdl);

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

class BroadcastServer {
 public:
  BroadcastServer(lptc_coderdojo::KinectDevice& _device, const int _port);

  void Run();

 private:
  void BroadcastToChannel(const std::string ch_name, DataProducer producer);
  Channel* GetChannel(const std::string& topic);
  void OnConnectionClosed(websocketpp::connection_hdl hdl);
  void OnMessage(websocketpp::connection_hdl hdl, AsioServer::message_ptr msg);
  void RegisterChannel(const std::string& name);

  typedef std::map<std::string, Channel> ChannelMap;

  const int port;
  AsioServer s;
  lptc_coderdojo::KinectDevice& device;
  ChannelMap channels;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_SERVER_H_