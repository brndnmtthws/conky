#include "logging.h"

#include <algorithm>

namespace conky::log {
static const char *DEFAULT_LOG_PATH = "/tmp/conky.log";

static level current_log_level = level::DEBUG;

bool is_enabled(level log_level) {
  return static_cast<int>(current_log_level) >= static_cast<int>(log_level);
}
void set_log_level(level log_level) { current_log_level = log_level; }
void log_more() {
  current_log_level = static_cast<level>(std::min(
      static_cast<int>(current_log_level) + 1, static_cast<int>(level::TRACE)));
}
void log_less() {
  current_log_level = static_cast<level>(std::max(
      static_cast<int>(current_log_level) - 1, static_cast<int>(level::OFF)));
}

static std::array<FILE *, 2> LOG_STREAMS{stderr, nullptr};
void use_log_file(const char *path) {
  if (LOG_STREAMS[1] != nullptr) { fclose(LOG_STREAMS[1]); }

  if (path != nullptr) {
    LOG_STREAMS[1] = fopen(path, "a+");
  } else {
    LOG_STREAMS[1] = fopen(DEFAULT_LOG_PATH, "a+");
  }
}

void init_system_logging() {
#ifdef HAS_SYSLOG
  openlog(PACKAGE_NAME, LOG_PID, LOG_USER);
#else
  use_log_file();
#endif
}

FILE **_log_streams() { return LOG_STREAMS.data(); };

void terminate_logging() {
  if (LOG_STREAMS[1] != nullptr) {
    fclose(LOG_STREAMS[1]);
    LOG_STREAMS[1] = nullptr;
  }
#ifdef HAS_SYSLOG
  closelog();
#endif
}

}  // namespace conky::log