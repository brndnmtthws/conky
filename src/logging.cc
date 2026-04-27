#include "logging.h"

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef BUILD_JOURNAL
#include <spdlog/sinks/systemd_sink.h>
#endif

static std::shared_ptr<spdlog::sinks::stderr_color_sink_mt> stderr_sink;

namespace {
// __FILE__ for this file is "<project_root>/src/logging.cc"
// Strip the known suffix to get the project root prefix length.
static constexpr size_t project_root_len =
    sizeof(__FILE__) - 1 - sizeof("src/logging.cc") + 1;
}  // namespace

// Custom source location formatter that omits [file:line] when empty
class source_loc_formatter_flag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg &msg,
              const std::tm &, spdlog::memory_buf_t &dest) override {
    if (msg.source.empty()) return;
    const char *file = msg.source.filename;
    if (strlen(file) > project_root_len) { file += project_root_len; }
    bool hide_func = !stderr_sink->should_log(spdlog::level::debug);
    std::string result;
    if (hide_func) {
      result = fmt::format(" [{}:{}]", file, msg.source.line);
    } else {
      result = fmt::format(" [{}:{}:{}]", file, msg.source.line, msg.source.funcname);
    }
    dest.append(result.data(), result.data() + result.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<source_loc_formatter_flag>();
  }
};

void conky::log::init_logger() {
  std::vector<spdlog::sink_ptr> sinks;

  stderr_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
  stderr_sink->set_level(spdlog::level::info);
  sinks.push_back(stderr_sink);

#ifdef BUILD_JOURNAL
  auto journal_sink = std::make_shared<spdlog::sinks::systemd_sink_mt>();
  journal_sink->set_level(spdlog::level::warn);
  sinks.push_back(journal_sink);
#endif

  auto logger =
      std::make_shared<spdlog::logger>("conky", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::trace);

  spdlog::set_default_logger(logger);

  // [timestamp][level][source:line][spans...] message
  // %^ = source location, %* = span context, %& = per-message attributes
  auto formatter = std::make_unique<spdlog::pattern_formatter>();
  formatter->add_flag<source_loc_formatter_flag>('^');
  formatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-5l]%^ %v");
  spdlog::set_formatter(std::move(formatter));
}

void conky::log::log_more() {
  auto lvl = static_cast<int>(stderr_sink->level());
  if (lvl < static_cast<int>(spdlog::level::trace)) return;
  stderr_sink->set_level(static_cast<spdlog::level::level_enum>(lvl - 1));
}

void conky::log::log_less() {
  auto lvl = static_cast<int>(stderr_sink->level());
  if (lvl >= static_cast<int>(spdlog::level::off)) return;
  stderr_sink->set_level(static_cast<spdlog::level::level_enum>(lvl + 1));
}

void conky::log::set_quiet() {
  stderr_sink->set_level(spdlog::level::off);
}
