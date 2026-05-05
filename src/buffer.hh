#ifndef CONKY_BUFFER_HH
#define CONKY_BUFFER_HH

#include <cstring>
#include <memory>
#include <string_view>
#include <utility>
#include <spdlog/fmt/fmt.h>
#include "logging.h"

namespace conky {

/// Bounds-checked sequential writer into a fixed-capacity char buffer.
///
/// Two modes:
///   buffer_writer w(cap);           — allocates its own buffer; use release() to
///                                  transfer ownership as a unique_ptr
///   buffer_writer w(cap, buf);      — writes into caller-owned buf; use
///                                  terminate() to null-terminate in place
class buffer_writer {
 public:
  buffer_writer(size_t cap, char *buf = nullptr)
      : m_cap(cap), m_owned(buf == nullptr) {
    m_buf = m_owned ? new char[cap] : buf;
  }

  ~buffer_writer() {
    if (m_owned) delete[] m_buf;
  }

  buffer_writer(const buffer_writer &) = delete;
  buffer_writer &operator=(const buffer_writer &) = delete;

  buffer_writer(buffer_writer &&o) noexcept
      : m_buf(o.m_buf), m_pos(o.m_pos), m_cap(o.m_cap), m_owned(o.m_owned) {
    o.m_buf = nullptr;
    o.m_owned = false;
  }

  // Append a single character. Returns false if the buffer is full.
  constexpr bool append(char c) {
    if (m_pos >= m_cap) return false;
    m_buf[m_pos++] = c;
    return true;
  }

  // Append a byte range. Returns false if it does not fit entirely.
  bool append(const char *s, size_t len) {
    if (m_pos + len > m_cap) return false;
    memcpy(m_buf + m_pos, s, len);
    m_pos += len;
    return true;
  }

  bool append(std::string_view sv) { return append(sv.data(), sv.size()); }

  template <typename T,
            typename = std::enable_if_t<fmt::is_formattable<T>::value>>
  bool append(const T &value) {
    size_t rem = remaining();
    auto result = fmt::format_to_n(cursor(), rem, "{}", value);
    m_pos += std::min(result.size, rem);
    return result.size <= rem;
  }

  // Pointer to the next write position, for use with C APIs like strftime that
  // write directly into a buffer. Call advance() afterwards.
  constexpr char *cursor() { return m_buf + m_pos; }

  // Advance the write position by n bytes after a direct write via cursor().
  // Clamps to capacity.
  constexpr void advance(size_t n) { m_pos = std::min(m_pos + n, m_cap); }

  constexpr size_t size() const { return m_pos; }
  constexpr size_t capacity() const { return m_cap; }
  constexpr size_t remaining() const { return m_cap - m_pos; }
  constexpr bool full() const { return m_pos >= m_cap; }

  // View of the bytes written so far (not null-terminated).
  constexpr std::string_view view() const { return {m_buf, m_pos}; }

  // Null-terminate in place. Safe to call on non-owning writers before
  // handing p back to the caller (cap must be > 0).
  constexpr void terminate() {
    if (m_cap > 0) m_buf[m_pos < m_cap ? m_pos : m_cap - 1] = '\0';
  }

  // Null-terminate and transfer ownership of the buffer to the caller.
  // Terminates if called on a non-owning writer — the caller does not own the
  // buffer and taking it would allow use-after-free.
  std::unique_ptr<char[]> take() {
    if (!m_owned)
      CRIT_ERR("take() called on a non-owning buffer_writer; "
               "the buffer belongs to the caller and cannot be transferred");
    terminate();
    m_owned = false;
    return std::unique_ptr<char[]>(std::exchange(m_buf, nullptr));
  }

 private:
  char *m_buf;
  size_t m_pos = 0;
  size_t m_cap;
  bool m_owned;
};

}  // namespace conky

#endif /* CONKY_BUFFER_HH */
