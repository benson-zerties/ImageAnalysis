/**
 * A implementation based on std::vector with locking:
 *  * a single producer
 *  * multiple consumers
 */

#ifndef DATA_VECTOR_H_
#define DATA_VECTOR_H_

#include <atomic>
#include <thread>
#include <vector>

using namespace std;

template<class T, size_t UChuckSize>
class DataVector 
{
    public:
        size_t size() const noexcept
        {
            return m_values.size();
        }
        /**
         * \brief Reserve space for the vector
         *
         * \details 
         *      Locking of resize is needed, since if the container capacity is exhausted,
         *      realllocation at a different memory location might take place
         */
        void reserve(size_t n)
        {
            size_t new_size = m_values.capacity();
            while (new_size < n) {
                new_size += UChuckSize;
            }
            while (m_lock.test_and_set(std::memory_order_acquire));  // acquire m_lock
            m_values.reserve(new_size);
            m_lock.clear(std::memory_order_release);
        }

        void push_back(T i)
        {
            if (m_values.capacity() < m_values.size() + sizeof(T)) {
                reserve(m_values.size() + sizeof(T));
            }
            
            auto* before = &m_values[0];
            m_values.push_back(i);
            auto* after = &m_values[0];
            assert(before == after && "Reallocation should never be needed in push_back");
        }

        std::vector<T> toStdVector()
        {
            std::vector<T> n(0);
            while (m_lock.test_and_set());  // acquire m_lock
            n = m_values;
            m_lock.clear();
            return n;
        }
        
        double operator()(int i)
        { 
            while (m_lock.test_and_set());  // acquire m_lock
            auto v = m_values[i];
            m_lock.clear(std::memory_order_release);
            return v;
        }

    private:
        std::vector<T> m_values;
        std::atomic_flag m_lock = ATOMIC_FLAG_INIT;
};

#endif /* DATA_VECTOR_H_ */
