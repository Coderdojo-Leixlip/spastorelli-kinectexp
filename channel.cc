#include "channel.h"

namespace lptc_coderdojo {

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

}  // namespace lptc_coderdojo