#ifndef LPTC_CODERDOJO_SERVER_H_
#define LPTC_CODERDOJO_SERVER_H_

#include "channel.h"
#include "device.h"
#include "publisher.h"

#include <future>
#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace lptc_coderdojo {

typedef websocketpp::server<websocketpp::config::asio> AsioServer;
typedef std::set<websocketpp::connection_hdl,
                 std::owner_less<websocketpp::connection_hdl>>
    ConnectionSet;

class BroadcastServer {
 public:
  BroadcastServer(lptc_coderdojo::KinectDeviceProxy& _device, const int _port);

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
  lptc_coderdojo::KinectDeviceProxy& device;

  ChannelMap channels;
  ConnectionSet connections;
  std::mutex connections_lock;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_SERVER_H_