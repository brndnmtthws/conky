#include "str_buffer.hh"

#include <algorithm>
#include <cstring>

char *str_buffer::take_data() {
  char *ret = this->m_data;
  this->m_data = nullptr;
  this->m_capacity = 0;
  m_length = 0;
  return ret;
}

void str_buffer::resize(size_t size) {
  char *new_data = new char[size + 1];
  memcpy(new_data, m_data, std::min(m_length, size));
  free(m_data);
  m_data = new_data;
  std::fill(m_data + m_length, m_data + size + 1, '\0');
  m_capacity = size;
  m_length = std::min(m_length, size);
}
void str_buffer::set_length(size_t len) {
  m_length = len;
  m_data[m_length] = '\0';
}

static inline double GROW_FACTOR = 1.5;
inline void autogrow(str_buffer *buffer) {
  size_t new_capacity =
      std::max(static_cast<size_t>(GROW_FACTOR *
                                   static_cast<double>(buffer->capacity())),
               static_cast<size_t>(1));
  buffer->resize(new_capacity);
}

void str_buffer::append(const char *str, size_t len) {
  while (m_length + len >= sizeof(m_data)) { autogrow(this); }
  std::memcpy(m_data + m_length, str, len);
  m_length += len;
}
void str_buffer::push(char c) {
  if (m_length + 1 >= sizeof(m_data)) { autogrow(this); }
  m_data[m_length++] = c;
}
void str_buffer::append(const str_buffer &other) {
  while (m_length + other.m_length >= sizeof(m_data)) { autogrow(this); }
  std::memcpy(m_data + m_length, other.m_data, other.m_length);
  m_length += other.m_length;
}
void str_buffer::append(const str_buffer &other, size_t offset, size_t len) {
  while (m_length + len >= sizeof(m_data)) { autogrow(this); }
  std::memcpy(m_data + m_length, other.m_data + offset, len);
  m_length += len;
}
void str_buffer::clear() {
  m_length = 0;
  m_data[0] = '\0';
}

// assignment
str_buffer &str_buffer::operator=(const str_buffer &other) {
  if (this == &other) { return *this; }
  if (m_capacity < other.m_length) { resize(other.m_length); }
  std::memcpy(m_data, other.m_data, other.m_length);
  m_length = other.m_length;
  return *this;
}
str_buffer &str_buffer::operator=(const char *str) {
  if (m_capacity < strlen(str)) { resize(strlen(str)); }
  std::memcpy(m_data, str, strlen(str));
  m_length = strlen(str);
  return *this;
}
str_buffer &str_buffer::operator=(const std::string &str) {
  if (m_capacity < str.length()) { resize(str.length()); }
  std::memcpy(m_data, str.data(), str.length());
  m_length = str.length();
  return *this;
}
// move assignment
str_buffer &str_buffer::operator=(str_buffer &&other) {
  if (this == &other) { return *this; }
  free(m_data);
  m_data = other.m_data;
  m_capacity = other.m_capacity;
  m_length = other.m_length;
  other.m_data = nullptr;
  other.m_capacity = 0;
  other.m_length = 0;
  return *this;
}
str_buffer &str_buffer::operator=(std::string &&str) {
  if (m_capacity < str.length()) { resize(str.length()); }
  std::memcpy(m_data, str.data(), str.length());
  m_length = str.length();
  return *this;
}

void str_buffer::replace_first(const char *old_str, const char *new_str) {
  size_t old_len = std::strlen(old_str);
  size_t new_len = std::strlen(new_str);
  char *pos = std::strstr(m_data, old_str);
  if (pos == NULL) { return; }
  std::memmove(pos + new_len, pos + old_len,
               m_length - (pos + old_len - m_data));
  std::memcpy(pos, new_str, new_len);
  m_length += new_len - old_len;
}
void str_buffer::replace_all(const char *old_str, const char *new_str) {
  size_t old_len = strlen(old_str);
  size_t new_len = strlen(new_str);
  while (true) {
    char *pos = strstr(m_data, old_str);
    if (pos == NULL) { break; }
    std::memmove(pos + new_len, pos + old_len,
                 m_length - (pos + old_len - m_data));
    std::memcpy(pos, new_str, new_len);
    m_length += new_len - old_len;
  }
}
void str_buffer::replace_last(const char *old_str, const char *new_str) {
  size_t old_len = std::strlen(old_str);
  size_t new_len = std::strlen(new_str);
  char *pos = std::strrchr(m_data, old_str[0]);
  if (pos == NULL) { return; }
  if (pos == m_data) {
    if (old_len == 1) {
      std::memcpy(pos, new_str, new_len);
      m_length += new_len - 1;
      return;
    }
    pos += old_len - 1;
  }
  std::memmove(pos + new_len, pos + old_len,
               m_length - (pos + old_len - m_data));
  std::memcpy(pos, new_str, new_len);
  m_length += new_len - old_len;
}