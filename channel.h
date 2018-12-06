#ifndef LPTC_CODERDOJO_CHANNEL_H_
#define LPTC_CODERDOJO_CHANNEL_H_

#include <iostream>
#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace lptc_coderdojo {

typedef websocketpp::server<websocketpp::config::asio> AsioServer;
typedef std::set<websocketpp::connection_hdl,
                 std::owner_less<websocketpp::connection_hdl>>
    ConnectionSet;

class Channel {
 public:
  Channel(const std::string& t, AsioServer& s);
  Channel(const Channel& ch);

  const std::string& GetTopic() const;

  void Publish(void const* data, size_t len);
  void Subscribe(websocketpp::connection_hdl hdl);
  void Unsubscribe(websocketpp::connection_hdl hdl);

 private:
  std::string topic;
  ConnectionSet subscribers;
  std::mutex subscribers_lock;
  AsioServer& server;
};

}  // namespace lptc_coderdojo

#endif  // LPTC_CODERDOJO_CHANNEL_H_