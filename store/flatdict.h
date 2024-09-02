#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include <stdexcept>

namespace dict {

    /**
     * @brief Key structure representing the 32-bit key and the associated index into the values array
     */
    struct Key {
        uint32_t m_index;
        uint32_t m_key;

        Key(): m_index(0), m_key(0) 
        {}
    };

    /**
     * @brief binary search using std::lower_bound to find a Key based on the key value
     * 
     * @tparam ForwardIt - iterator type
     * @tparam T - key type
     * @tparam Compare - comparison method type
     * @param first - iterator pointed at first element
     * @param last - iterator pointed beyond the last element
     * @param value - key value to search for
     * @param comp - comparison method
     * @return ForwardIt - iterator pointing to the desired Key element or end() if not found
     */
    template<class ForwardIt, class T, class Compare>
    ForwardIt findKey(ForwardIt first, ForwardIt last, const T& value, Compare comp)
    {
        ForwardIt i = std::lower_bound(first, last, value, comp);
        if (i != last && !(value < i->m_key))
            return i;
        else
            return last;
    }

    /**
     * @brief The Dict class implements a "flat" key/value dictionary
     * 
     * @tparam N - maximum number of elements in the dictionary
     */
    template<class Value, std::size_t N>
    class Dict {
    public:
        // Type aliases
        using KeyType = std::array<Key,N>;
        using ValueType = std::array<Value,N>;

        /**
         * @brief Default constructor
         */
        Dict(): m_size(0)
        {}

        /**
         * @brief Destructor
         */
        ~Dict() = default;

        /**
         * @brief Construct a new Dict object from a memory buffer
         * 
         * @param rhs 
         */
        Dict(const Dict &rhs):
            m_size(rhs.m_size),
            m_keys(rhs.m_keys),
            m_values(rhs.m_values)
        {}

        Dict(const char *buf, size_t size) {

            refresh(buf, size);

        }

        void refresh(const char *buf, size_t size) {
            // Validate that the buffer size parameter matches this 
            // class size
            if (size != sizeof(*this)) {
                throw std::runtime_error("Invalid buffer size");
            }
            // Set m_size from the buffer
            m_size = *(reinterpret_cast<const size_t*>(buf));
            // Validate that the buffer's m_size is within the Dict
            // capacity
            if (m_size > N) {
                throw std::runtime_error("Invalid buffer content");
            }
            // Copy buffer's keys into m_keys
            memcpy(m_keys.data(),buf+8,sizeof(m_keys));
            // Validate indices for each key
            for(const auto &key: m_keys) {
                if (key.m_index > m_size) {
                    throw std::runtime_error("Invalid key index value");
                }
            }
            // Copy buffer's values into m_values
            memcpy(m_values.data(),buf+8+sizeof(m_keys), sizeof(m_values));
        }

        /**
         * @brief Assignment operator
         * 
         * @param rhs - Dictionary to copy from 
         * @return Dict& - reference to this Dict
         */
        Dict& operator=(const Dict &rhs) {
            if (this != &rhs) {
                m_keys = rhs.m_keys;
                m_values = rhs.m_values;
                m_size = rhs.m_size;
            }
            return *this;
        }

        /**
         * @brief clear the Dict contents
         */
        void clear() {
            m_keys.clear();
            m_values.clear();
            m_size = 0;
        }

        /**
         * @brief return the current Dict size
         * 
         * @return std::size_t - number of elements
         */
        std::size_t size() const {
            return m_size;
        }

        /**
         * @brief return the Dict capacity
         * 
         * @return std::size_t - element capacity
         */
        std::size_t capacity() const {
            return N;
        }

        /**
         * @brief return a pointer to the Dict's contiguous data
         * 
         * @return const uint8_t* - buffer pointer
         */
        const char *data() const {
            return reinterpret_cast<const char*>(&m_size);
        }

        /**
         * @brief return the Dict's in-memory size
         * 
         * @return size_t - container size
         */
        size_t container_size() const {
            return sizeof(*this);
        }

        /**
         * @brief return an iterator pointing to the first key
         * 
         * @return KeyType::iterator 
         */
        typename KeyType::iterator begin() {
            return m_keys.begin();
        }

        /**
         * @brief return an iterator pointing just beyond the last key
         * 
         * @return KeyType::iterator 
         */
        typename KeyType::iterator end() {
            return m_keys.begin() + m_size;
        }

        /**
         * @brief return a const iterator pointing to the first key
         * 
         * @return KeyType::const_iterator 
         */
        typename KeyType::const_iterator cbegin() const {
            return m_keys.cbegin();
        }

        /**
         * @brief return a const iterator pointing just beyond the last key
         * 
         * @return KeyType::const_iterator 
         */
        typename KeyType::const_iterator cend() const {
            return m_keys.cbegin() + m_size;
        }

        /**
         * @brief insert a key/value pair into the Dict
         * 
         * @param key - 32-bit key
         * @param value - value associated with the key
         * @return true - key/value pair inserted into the Dict
         * @return false - key/value pair not inserted
         */
        bool insert(const uint32_t &key, const Value &value)
        {
            if (m_size == m_keys.size()) {
                return false;
            }
            if (!contains(key))
            {
                m_keys[m_size].m_key = key;
                m_keys[m_size].m_index = static_cast<uint32_t>(m_size);
                m_values[m_size] = value;
                m_size++;
                std::sort(&m_keys[0], &m_keys[m_size], 
                    [](const Key &a, const Key &b) 
                    { return a.m_key < b.m_key; });
                return true;
            }
            return false;
        }

        /**
         * @brief update the value associated with a key/value pair already in the Dict
         * 
         * @param key - 32-bit key
         * @param value - updated value for the key
         * @return true - key/value pair updated
         * @return false - key/value pair not updated
         */
        bool set(const uint32_t &key, const Value &value)
        {
            auto i = find(key);

            if (i != &m_keys[m_size]) {
                m_values.at(i->m_index) = value;
                return true;
            }
            return false;
        }

        /**
         * @brief Return indication of whether a key/value pair is in the Dict
         * 
         * @param key - 32-bit key
         * @return true - key/value pair is present
         * @return false - key/value pair is not present
         */
        bool contains(const uint32_t key) {
            return (find(key) != &m_keys[m_size]);
        }

        /**
         * @brief Return a reference to the value associated with a particular key
         * 
         * @param key - 32-bit key
         * @return Value& - value associated with this key.
         *                  throws std::out_of_range exception if key is not present
         */
        Value &at(const uint32_t key) {
            auto i = find(key);

            if (i != &m_keys[m_size]) {
                return m_values.at(i->m_index);
            }
            throw std::out_of_range("Key not found");
        }

        /**
         * @brief Return a reference to the value associated with a particular key (range-based for loop)
         * 
         * @param key - Key struct
         * @return Value& - value associated with this key.
         *                  throws std::out_of_range exception if key is not present
         */
        Value &at(const Key &key) {
            auto i = find(key.m_key);

            if (i != &m_keys[m_size]) {
                return m_values.at(i->m_index);
            }
            throw std::out_of_range("Key not found");            
        }

        /**
         * @brief Return a const reference to the value associated with a key
         * 
         * @param key - 32-bit key
         * @return const Value& - const reference to value
         *                        throws std::out_of_range exception if key is not present
         */
        const Value &at(const uint32_t key) const {
            auto i = find(key);

            if (i != &m_keys[m_size]) {
                return m_values.at(i->m_index);
            }
            throw std::out_of_range("Key not found");
        }

        /**
         * @note operator[] is not implemented to avoid "accidental" insertion of elements into the Dict
         */

    private:
        /**
         * @brief Find a key in the ordered collection of keys
         * 
         * @param key - key value to search for
         * @return Key* - Key element matching the key value
         */
        Key* find(const uint32_t key) {
            return findKey(&m_keys[0],&m_keys[m_size], key,
                [] (Key &elt, const uint32_t &key)
                { return elt.m_key < key; });
        }
        /**
         * @brief Class members are expected to be a contiguous block of memory based on 
         *        std::array ContiguousContainer requirements
         */
        size_t m_size;
        KeyType m_keys;
        ValueType m_values;
    };
}