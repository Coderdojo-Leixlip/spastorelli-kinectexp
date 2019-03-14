#include "server.h"

#include <memory>
#include "device.h"

namespace {
volatile std::sig_atomic_t sig_status;
std::unique_ptr<lptc_coderdojo::BroadcastServer> server;
}  // namespace

void SignalHandler(int signal) {
  sig_status = signal;

  if (sig_status == SIGINT || sig_status == SIGTERM) server->Stop();
}

int main() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  try {
    std::unique_ptr<lptc_coderdojo::KinectDeviceProxy> device =
        lptc_coderdojo::CreateKinectDeviceProxy(0);
    server = std::unique_ptr<lptc_coderdojo::BroadcastServer>(
        new lptc_coderdojo::BroadcastServer(*device, 9002));
    server->Run();
  } catch (const lptc_coderdojo::KinectDeviceException& e) {
    std::cerr << e.what() << std::endl;
  }
}