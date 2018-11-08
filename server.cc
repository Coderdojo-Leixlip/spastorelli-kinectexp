#include "device.h"

#include "libfreenect.hpp"

#include <iostream>
#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

class DeviceBroadcastServer {
 public:
  DeviceBroadcastServer(lptc_coderdojo::KinectDevice& _device)
      : device(_device) {
    s.clear_access_channels(websocketpp::log::alevel::all);
    s.init_asio();
    s.set_open_handler(std::bind(&DeviceBroadcastServer::OnConnectionOpen, this,
                                 std::placeholders::_1));
    s.set_close_handler(std::bind(&DeviceBroadcastServer::OnConnectionClosed,
                                  this, std::placeholders::_1));
  }

  void BroadcastFrames() {
    while (1) {
      const std::vector<uint8_t>& frame = device.GetNextDepthFrame();
      if (!frame.empty()) {
        std::lock_guard<std::mutex> guard(connections_lock);
        connection_set::iterator iter;
        for (iter = connections.begin(); iter != connections.end(); ++iter) {
          int size = sizeof(uint8_t) * frame.size();
          s.send(*iter, frame.data(), size, websocketpp::frame::opcode::binary);
        }
      }
    }
  }

  void StartDeviceStreams() { device.StartDepth(); }

  void OnConnectionClosed(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(connections_lock);
    connections.erase(hdl);
  }

  void OnConnectionOpen(websocketpp::connection_hdl hdl) {
    std::lock_guard<std::mutex> guard(connections_lock);
    connections.insert(hdl);
  }

  void run() {
    s.listen(port);
    s.start_accept();
    std::cout << "Listening on port " << port << "..." << std::endl;

    StartDeviceStreams();
    std::cout << "Started Kinect Streaming." << std::endl;
    std::thread broadcast_thread(
        std::bind(&DeviceBroadcastServer::BroadcastFrames, this));

    s.run();
    broadcast_thread.join();
  }

 private:
  typedef std::set<websocketpp::connection_hdl,
                   std::owner_less<websocketpp::connection_hdl>>
      connection_set;

  const int port = 9002;
  websocketpp::server<websocketpp::config::asio> s;
  connection_set connections;
  std::mutex connections_lock;

  lptc_coderdojo::KinectDevice& device;
};

int main() {
  Freenect::Freenect freenect;
  lptc_coderdojo::KinectDevice& device =
      freenect.createDevice<lptc_coderdojo::OpenKinectDevice>(0);

  DeviceBroadcastServer s(device);
  s.run();
}