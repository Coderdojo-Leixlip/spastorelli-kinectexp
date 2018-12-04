#ifndef LPTC_CODERDOJO_SERVER_H_
#define LPTC_CODERDOJO_SERVER_H_

#include "channel.h"
#include "device.h"
#include "publisher.h"

#include <future>
#include <iostream>
#include <set>
#include <vector>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace lptc_coderdojo {

typedef websocketpp::server<websocketpp::config::asio> AsioServer;
typedef std::set<websocketpp::connection_hdl,
                 std::owner_less<websocketpp::connection_hdl>>
    ConnectionSet;

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

class BroadcastServer {
 public:
  BroadcastServer(lptc_coderdojo::KinectDevice& _device, const int _port);

  void Run();
  void Stop();

 private:
  void BroadcastToChannel(const std::string ch_name,
                          lptc_coderdojo::Publisher& publisher);
  void CloseConnections(const std::string& reason);
  lptc_coderdojo::Channel* GetChannel(const std::string& topic);
  void OnConnectionClosed(websocketpp::connection_hdl hdl);
  void OnConnectionOpened(websocketpp::connection_hdl hdl);
  void OnMessage(websocketpp::connection_hdl hdl, AsioServer::message_ptr msg);
  void RegisterChannel(const std::string& name);
  void SendErrorMessage(websocketpp::connection_hdl hdl,
                        const std::string& error_msg);
  void StopAllChannelBroadcasts();

  typedef std::map<std::string, lptc_coderdojo::Channel> ChannelMap;

  const int port;

  std::promise<void> term_sig;
  std::future<void> term_future;

  AsioServer s;
  lptc_coderdojo::KinectDevice& device;

  ChannelMap channels;
  ConnectionSet connections;
  std::mutex connections_lock;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_SERVER_H_