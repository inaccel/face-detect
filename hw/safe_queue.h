#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>

// A threadsafe-queue.
template <class T>
class SafeQueue {
public:
	SafeQueue(): q(), m(), c(), maxSize(-1) {}

	SafeQueue(int maxSize): q(), m(), c(), maxSize(maxSize) {}

	SafeQueue<T>(const SafeQueue<T>&sq) {
		maxSize = sq.maxSize;
		q = sq.q;
	}

	~SafeQueue(void) {}

	// Add an element to the queue.
	void enqueue(T t) {
		while(true) {
			std::lock_guard<std::mutex> lock(m);
			if (maxSize < 0) {
				q.push(std::move(t));
				break;
			}
			else if (q.size() < (unsigned) maxSize) {
				q.push(std::move(t));
				break;
			}
			else c.notify_one();
		}
		c.notify_one();
	}

	// Get the "front"-element.
	// If the queue is empty, wait till a element is avaiable.
	T dequeue(void) {
		std::unique_lock<std::mutex> lock(m);
		while(q.empty()) {
			// release lock as long as the wait and reaquire it afterwards.
			c.wait(lock);
		}

		T val = std::move(q.front());
		q.pop();
		return val;
	}

private:
	std::queue<T> q;
	std::mutex m;
	std::condition_variable c;
	int maxSize;
};

#endif
