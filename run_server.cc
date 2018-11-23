#include "server.h"

int main() {
  Freenect::Freenect freenect;
  lptc_coderdojo::KinectDevice& device =
      freenect.createDevice<lptc_coderdojo::OpenKinectDevice>(0);

  lptc_coderdojo::BroadcastServer s(device, 9002);
  s.Run();
}