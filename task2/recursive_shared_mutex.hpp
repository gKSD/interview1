#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <map>

class RecursiveSharedMutex
{
public:
    RecursiveSharedMutex();

    void lock();
    bool tryLock();
    void unlock();

    void lockShared();
    bool tryLockShared();
    void unlockShared();

private:
    std::mutex m_Mutex;

    std::map<std::thread::id, int> m_Readers;

    std::atomic<std::thread::id> m_Writer;
    std::atomic<int> m_WritersWaitingForLock;

    int m_WriterLockLevel = 0;
};

RecursiveSharedMutex::RecursiveSharedMutex() :
    m_WritersWaitingForLock(0)
{
}

void RecursiveSharedMutex::lock()
{
    std::thread::id tid = std::this_thread::get_id();
    {
        // check if thread is already writer
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Writer == tid)
        {
            ++m_WriterLockLevel;
            return;
        }
    }

    // this is new thread
    // wait for write-locking
    ++m_WritersWaitingForLock;

    // try to setup new writer (only after old writer unlocks)
    std::thread::id tmp = std::thread::id();
    while (!m_Writer.compare_exchange_weak(tmp, tid))
    {
        continue;
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        ++m_WriterLockLevel;
    }

    // wait for readers to unlock
    while (1)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Readers.empty())
            return;
    }
}

bool RecursiveSharedMutex::tryLock()
{
    std::thread::id tid = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(m_Mutex);

    // check if thread is already writer
    if (m_Writer == tid)
    {
        ++m_WriterLockLevel;
        return true;
    }

    std::thread::id tmp = std::thread::id();
    if (m_Writer.compare_exchange_weak(tmp, tid) && m_Readers.empty())
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        ++m_WriterLockLevel;
        return true;
    }

    return false;
}

void RecursiveSharedMutex::unlock()
{
    if (m_Writer == std::thread::id())
    {
        throw std::runtime_error("error unlocking: no writer available");
    }

    std::thread::id tid = std::this_thread::get_id();
    if (tid != m_Writer)
    {
        throw std::runtime_error("error unlocking: not writer thread");
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (--m_WriterLockLevel == 0)
    {
        // all writer locks unlocked
        // remove writer thread id
        m_Writer = std::thread::id();
    }
}

void RecursiveSharedMutex::lockShared()
{
    std::thread::id tid = std::this_thread::get_id();
    if (tid == m_Writer)
    {
        // this is writer thread, just increase its level of locks
        std::lock_guard<std::mutex> lock(m_Mutex);

        ++m_WriterLockLevel;
        return;
    }

    // its common reader thread 

    // wait for all available writers to finish thier work and unlock
    while (1)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_WritersWaitingForLock == 0 && m_Writer == std::thread::id())
        {
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // check if is already locked
        auto it = m_Readers.find(tid);
        if (it == m_Readers.end())
        {
            // it is new reader
            m_Readers.emplace(tid, 1);
        }
        else
        {
            ++it->second;
        }
    }
}

bool RecursiveSharedMutex::tryLockShared()
{
    std::thread::id tid = std::this_thread::get_id();
    if (tid == m_Writer)
    {
        // this is writer thread, just increase its level of locks
        std::lock_guard<std::mutex> lock(m_Mutex);

        ++m_WriterLockLevel;
        return true;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_WritersWaitingForLock == 0 && m_Writer == std::thread::id())
    {
        // check if is already locked
        auto it = m_Readers.find(tid);
        if (it == m_Readers.end())
        {
            // it is new reader
            m_Readers.emplace(tid, 1);
        }
        else
        {
            ++it->second;
        }

        return true;
    }

    return false;
}



void RecursiveSharedMutex::unlockShared()
{
    std::thread::id tid = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_Writer == tid)
    {
        if (--m_WriterLockLevel == 0)
        {
            // all writer locks unlocked
            // remove writer thread id
            m_Writer = std::thread::id();
        }

        return;
    }

    if (m_Readers.empty())
    {
        throw std::runtime_error("error shared unlocking: no readers avaliable");
    }

    auto it = m_Readers.find(tid);
    if (it == m_Readers.end())
    {
        throw std::runtime_error("error shared unlocking: no locks avaliable for thread");
    }

    if (--it->second == 0)
    {
        m_Readers.erase(it);
    }
}
