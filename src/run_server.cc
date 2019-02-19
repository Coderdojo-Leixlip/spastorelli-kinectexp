#include "openkinect_device.h"
#include "server.h"

namespace {
volatile std::sig_atomic_t sig_status;
lptc_coderdojo::BroadcastServer* kserver;
}  // namespace

void SignalHandler(int signal) {
  sig_status = signal;

  if (sig_status == SIGINT || sig_status == SIGTERM) kserver->Stop();
}

int main() {
  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);

  Freenect::Freenect freenect;
  lptc_coderdojo::KinectDevice& device =
      freenect.createDevice<lptc_coderdojo::OpenKinectDevice>(0);

  kserver = new lptc_coderdojo::BroadcastServer(device, 9002);
  kserver->Run();
}