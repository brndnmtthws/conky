#include "logger.hh"

#include <chrono>

using namespace conky::log;

// enum class detail {
//   TIME,
//   LEVEL,
//   SOURCE,
//   CONTEXT,
// };

namespace conky::log::_priv {

void format_time(str_buffer &buffer, const ::conky::log::logger &logger_state,
                 const ::conky::log::msg_details &entry_state) {
  std::time_t current_time =
      std::chrono::system_clock::to_time_t(entry_state.time.value());
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      entry_state.time.value().time_since_epoch());
  struct tm local_time;
  localtime_r(&current_time, &local_time);
  size_t time_len = 0;
  time_len += std::strftime(out, max_length, "%F %T", &local_time);
  time_len += snprintf(&out[time_len], max_length - time_len, ".%03d",
                       static_cast<int>(millis.count() % 1000));

  return time_len;
}

#define VAL_AND_LEN(STR) (std::make_pair(STR, sizeof(STR)))
const std::array<std::pair<const char *, size_t>, 7> _LVL_STRINGS{
    VAL_AND_LEN("CRITICAL"), VAL_AND_LEN("ERROR"), VAL_AND_LEN("WARNING"),
    VAL_AND_LEN("NOTICE"),   VAL_AND_LEN("INFO"),  VAL_AND_LEN("DEBUG"),
    VAL_AND_LEN("TRACE"),
};
#undef VAL_AND_LEN
void format_level(str_buffer &buffer, const ::conky::log::logger &logger_state,
                  const ::conky::log::msg_details &entry_state) {
  buffer.append(
      _LVL_STRINGS[static_cast<uint8_t>(entry_state.level) - 1].first);
}

void format_source(str_buffer &buffer, const ::conky::log::logger &logger_state,
                   const ::conky::log::msg_details &entry_state) {
  entry_state->level
}

void format_detail(str_buffer &buffer, const ::conky::log::logger &logger_state,
                   const ::conky::log::msg_details &entry_state,
                   detail detail) {
  switch (detail) {
    case detail::TIME:
      format_time(buffer, logger_state, entry_state);
      break;
    case detail::LEVEL:
      format_level(buffer, logger_state, entry_state);
      break;
    case detail::SOURCE:
      format_source(buffer, logger_state, entry_state);
      break;
    case detail::CONTEXT:
      format_context(buffer, logger_state, entry_state);
      break;
  }
}

}  // namespace conky::log::_priv
