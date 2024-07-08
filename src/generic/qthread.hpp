#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

// class QThread manages a thread which processes items in a queue.
// Items can be added to the queue, and the queue can be emptied.

template <class T>
class QThread
{
public:
    QThread(std::function<void(T)>);
    ~QThread();

    void add(T item);
    void clear_queue();
    void join();

private:
    T       pop();
    void    worker();

private:
    std::queue<T>   queue;  // regular queue
    std::mutex      mutex;  // to make the queue thread-safe
    std::condition_variable cond; // notify/wait for the thread

    std::function<void(T)> process_item; // function to process a single item
    std::thread     qt; // thread that processes items from the queue
};

template <class T>
QThread<T>::QThread(std::function<void(T)> f)
    :   queue(),
        mutex(),
        cond(),
        process_item(f),
        qt(&QThread::worker, this) // also starts the thread
{}

template <class T>
QThread<T>::~QThread() {}

template <class T>
void QThread<T>::add(T item)
{
    std::unique_lock<std::mutex> lock(mutex);
    queue.push(item);
    cond.notify_one();
}

template <class T>
void QThread<T>::clear_queue()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (!queue.empty())
    {
        queue.pop();
    }
}

// Due to the worker's while(true)-loop, this method never terminates!
template <class T>
void QThread<T>::join()
{
    qt.join();
}

template <class T>
T QThread<T>::pop()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (queue.empty())
    {
        cond.wait(lock);
    }

    T item = queue.front();
    queue.pop();
    return item;
}

template <class T>
void QThread<T>::worker()
{
    while (true) {
        T item = pop();
        process_item(item);
    }
}
