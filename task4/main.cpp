#include <iostream>
#include <unordered_map>
#include <thread>
#include <string>
#include <set>
#include <mutex>

std::string select_from_db(const std::string &key)
{
    std::string data;
    // ... selecting ...
    return data;
}

void create_or_update_db(const std::string &key, const std::string &data)
{
    // ... create or update ...
}

void delete_from_db(const std::string &key)
{
    // ... delete ...
}

void start_transaction_db()
{
    // ... start ...
}

void commit_transaction_db()
{
    // ... commit ...
}

void rollback_db()
{
    // ... roll back ...
}

enum class OperationType
{
    UPDATE,
    DELETE
};

using CacheData = std::unordered_map<std::string, std::string>;
using DeletedData = std::set<std::string>;
struct OperationData
{
    DeletedData deleted;
    CacheData updated;
};

struct i_db
{
    bool begin_transaction()
    {
        std::thread::id tid = std::thread::id();
        auto it = m_Operations.find(tid);
        if (it != m_Operations.end())
        {
            std::cout << "error: transaction is already open" << std::endl;
            return false;
        }

        m_Operations.emplace(tid, OperationData());
        return true;
    }

    bool commit_transaction()
    {
        std::thread::id tid = std::thread::id();
        auto it = m_Operations.find(tid);
        if (it != m_Operations.end())
        {
            std::cout << "error: no transaction open" << std::endl;
            return false;
        }

        start_transaction_db();

        try
        {
            for (auto const& upd : it->second.updated)
            {
                create_or_update_db(upd.first, upd.second);
            }

            for (auto const& del : it->second.deleted)
            {
                delete_from_db(del);
            }
        }
        catch(...)
        {
            rollback_db();
            return false;
        }

        for (auto const& upd : it->second.updated)
        {
            create_or_update_db(upd.first, upd.second);

            {
                std::lock_guard<std::mutex> lock(m_Mutex);

                m_Data.emplace(upd.first, upd.second);
            }
        }

        for (auto const& del : it->second.deleted)
        {
            delete_from_db(del);

            {
                std::lock_guard<std::mutex> lock(m_Mutex);

                m_Data.erase(del);
            }
        }

        return true;
    }

    bool abort_transaction()
    {
        std::thread::id tid = std::thread::id();
        m_Operations.erase(tid);

        return true;
    }

    std::string get(const std::string& key)
    {
        std::thread::id tid = std::thread::id();
        auto it = m_Operations.find(tid);
        if (it != m_Operations.end())
        {
            // search in local cache
            auto updated_it = it->second.updated.find(key);
            if (updated_it != it->second.updated.end())
            {
                return updated_it->second;
            }
            
            // check if deleted
            auto deleted_it = it->second.deleted.find(key);
            if (deleted_it != it->second.deleted.end())
            {
                throw std::runtime_error("deleted");
                // TODO: or return nullptr
            }
        }

        std::lock_guard<std::mutex> lock(m_Mutex);
        auto data_it = m_Data.find(key);
        if (data_it != m_Data.end())
        {
            return data_it->second;
        }

        std::string data = select_from_db(key);
        m_Data.emplace(key, data);
        return data;
    }

    void set(const std::string& key, const std::string& data)
    {
        std::thread::id tid = std::thread::id();
        auto it = m_Operations.find(tid);
        if (it == m_Operations.end())
        {
            throw std::runtime_error("no transaction opened");
        }

        auto insert_result = it->second.updated.emplace(key, data);
        if (!insert_result.second)
        {
            insert_result.first->second = data;
        }

        // delete from deleted if so
        it->second.deleted.erase(key);
    }

    std::string delet(const std::string& key)
    {
        std::thread::id tid = std::thread::id();
        auto it = m_Operations.find(tid);
        if (it == m_Operations.end())
        {
            throw std::runtime_error("no transaction opened");
        }

        it->second.deleted.insert(key);

        // delete from updated if so
        it->second.updated.erase(key);
    }

    CacheData m_Data;
    std::unordered_map<std::thread::id, OperationData> m_Operations;
    std::mutex m_Mutex;
};

int main()
{
    return 0;
}
