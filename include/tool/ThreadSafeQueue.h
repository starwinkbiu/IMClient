#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H
#include <QQueue>
#include <QMutex>

template<typename T>
class ThreadSafeQueue{
public:
    void enqueue(const T& value);
    T dequeue();
    bool isEmpty();
private:
    QQueue<T> m_queue;
    mutable QMutex m_mutex;
};

template<typename T>
void ThreadSafeQueue<T>::enqueue(const T& value){
    QMutexLocker locker(&m_mutex);
    m_queue.enqueue(value);
}

template<typename T>
T ThreadSafeQueue<T>::dequeue()
{
    QMutexLocker locker(&m_mutex);
    return m_queue.dequeue();
}

template<typename T>
bool ThreadSafeQueue<T>::isEmpty()
{
    QMutexLocker locker(&m_mutex);
    return m_queue.isEmpty();
}

#endif // THREADSAFEQUEUE_H
