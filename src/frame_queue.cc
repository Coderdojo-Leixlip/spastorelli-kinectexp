#include <functional>

namespace lptc_coderdojo {

template <typename T>
FrameQueue<T>::FrameQueue() {}

template <typename T>
void FrameQueue<T>::Push(const std::vector<T>& data) {
  std::lock_guard<std::mutex> guard(queue_lock);
  queue.push(data);
  queue_cond.notify_one();
}

template <typename T>
bool FrameQueue<T>::Pop(std::vector<T>& data,
                        const std::chrono::milliseconds& timeout) {
  std::unique_lock<std::mutex> lock(queue_lock);

  if (queue.empty()) {
    if (queue_cond.wait_for(lock, timeout,
                            std::bind(&FrameQueue::QueueNotEmptyPred, this))) {
      return PopInternal(data);
    } else {
      return false;
    }
  } else {
    return PopInternal(data);
  }
  return false;
}

template <typename T>
bool FrameQueue<T>::PopInternal(std::vector<T>& data) {
  data = queue.front();
  queue.pop();
  return true;
}

template <typename T>
bool FrameQueue<T>::QueueNotEmptyPred() {
  return !queue.empty();
}

}  // namespace lptc_coderdojo