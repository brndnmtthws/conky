#ifndef _CONKY_STR_BUFFER_H
#define _CONKY_STR_BUFFER_H

#include <cstdlib>
#include <cstring>
#include <string>

/// @brief Container for string manipulation that grows automatically like
/// `std::vector`, but supports its internal storage to be moved out for raw
/// access.
/// Interface is similar to `std::string`, but mutating raw state m_data is not
/// considered UB.
///
/// @note Not thread-safe.
class str_buffer {
  char *m_data;
  size_t m_capacity = 0;
  size_t m_length = 0;

 public:
  str_buffer(size_t size) { m_data = new char[size]; }
  str_buffer() : str_buffer(128) {}
  ~str_buffer() { free(m_data); }

  inline char *data() { return m_data; }
  char *take_data();
  inline std::string to_string() { return std::string(m_data, m_length); }

  void resize(size_t size);
  void set_length(size_t len);

 public:
  void append(const char *str, size_t len);
  inline void append(const char *str) { append(str, strlen(str)); }
  void push(char c);
  void append(const str_buffer &other);
  void append(const str_buffer &other, size_t offset, size_t length);
  void clear();

  inline size_t size() const { return m_length; }
  inline size_t capacity() const { return m_capacity; }

  inline char &operator[](size_t i) { return m_data[i]; }
  inline const char operator[](size_t i) const { return m_data[i]; }
  inline const char at(size_t i) const { return m_data[i]; }

  inline str_buffer &operator+=(const char *str) {
    append(str, strlen(str));
    return *this;
  }
  inline str_buffer &operator+=(char c) {
    push(c);
    return *this;
  }
  inline str_buffer &operator+=(const str_buffer &other) {
    append(other);
    return *this;
  }

  // assignment
  str_buffer &operator=(const str_buffer &other);
  str_buffer &operator=(const char *str);
  str_buffer &operator=(const std::string &str);

  // move assignment
  str_buffer &operator=(str_buffer &&other);
  str_buffer &operator=(std::string &&str);

  // comparison
  inline bool operator==(const str_buffer &other) const {
    return m_length == other.m_length &&
           std::memcmp(m_data, other.m_data, m_length) == 0;
  }
  inline bool operator==(const char *other) const {
    return std::memcmp(m_data, other, m_length) == 0;
  }
  inline bool operator==(const std::string &other) const {
    return m_length == other.length() &&
           std::memcmp(m_data, other.data(), m_length) == 0;
  }

  inline bool operator!=(const str_buffer &other) const {
    return !operator==(other);
  }
  inline bool operator!=(const char *other) const { return !operator==(other); }

  // iterators
  inline const char *begin() const { return m_data; }
  inline const char *end() const { return m_data + m_length; }

  // string operations
  inline void to_lower() {
    for (size_t i = 0; i < m_length; ++i) {
      m_data[i] = std::tolower(m_data[i]);
    }
  }
  inline void to_upper() {
    for (size_t i = 0; i < m_length; ++i) {
      m_data[i] = std::toupper(m_data[i]);
    }
  }

  void replace_first(const char *old_str, const char *new_str);
  void replace_all(const char *old_str, const char *new_str);
  void replace_last(const char *old_str, const char *new_str);
};

#endif /* _CONKY_STR_BUFFER_H */
