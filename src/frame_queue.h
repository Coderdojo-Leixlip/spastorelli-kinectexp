#ifndef LPTC_CODERDOJO_FRAME_QUEUE_H_
#define LPTC_CODERDOJO_FRAME_QUEUE_H_

#include <mutex>
#include <queue>
#include <vector>

namespace lptc_coderdojo {

template <typename T>
class FrameQueue {
 public:
  FrameQueue();

  void Push(const std::vector<T>& data);
  bool Pop(std::vector<T>&, const std::chrono::milliseconds& timeout);

 private:
  bool PopInternal(std::vector<T>&);
  bool QueueNotEmptyPred();

  std::queue<std::vector<T>> queue;
  std::mutex queue_lock;
  std::condition_variable queue_cond;
};

}  // namespace lptc_coderdojo

#include "frame_queue.cc"

#endif  // LPTC_CODERDOJO_FRAME_QUEUE_H_