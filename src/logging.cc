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

// Used to avoid the nasty 1-2-4-8 initial growth for relatively small data (spans/attributes)
template <typename T>
std::vector<T> make_reserved(size_t capacity) {
  std::vector<T> v;
  v.reserve(capacity);
  return v;
}
}  // namespace

// Thread-local span stack
static thread_local auto tl_spans = make_reserved<conky::log::span>(8);

// Thread-local per-message attributes (set before log call, cleared after)
static thread_local auto tl_msg_attrs = make_reserved<conky::log::attribute>(4);

static std::string format_attrs(std::initializer_list<conky::log::attribute> attrs) {
  std::string result = "{";
  size_t i = 0;
  for (const auto &a : attrs) {
    if (i++ > 0) result += ' ';
    result += a.key;
    result += '=';
    result += a.value;
  }
  result += '}';
  return result;
}

void conky::log::span_guard::open(spdlog::source_loc loc, std::string name,
                                  std::initializer_list<attribute> attrs) {
  tl_spans.emplace_back(std::move(name));
  m_active = true;
  if (attrs.size() > 0) {
    spdlog::default_logger()->log(loc, spdlog::level::trace, ">> {}{}",
                                  tl_spans.back().name(), format_attrs(attrs));
  } else {
    spdlog::default_logger()->log(loc, spdlog::level::trace, ">> {}",
                                  tl_spans.back().name());
  }
}

conky::log::span_guard::~span_guard() { drop(); }

void conky::log::span_guard::drop() {
  if (!m_active) return;
  if (tl_spans.empty()) return;

  spdlog::default_logger()->log(spdlog::level::trace, "<< {}",
                                tl_spans.back().name());
  tl_spans.pop_back();
  m_active = false;
}

std::string conky::log::current_span_context() {
  if (tl_spans.empty()) return {};
  std::string result = "[";
  for (size_t i = 0; i < tl_spans.size(); i++) {
    if (i > 0) result += "::";
    result += tl_spans[i].name();
  }
  result += ']';
  return result;
}

std::vector<conky::log::span> conky::log::capture_context() { return tl_spans; }

void conky::log::install_context(const std::vector<span> &ctx) {
  tl_spans = ctx;
}

// Custom spdlog formatter flag that injects span context
// info+: no spans; debug: spans without attrs; trace: spans with attrs
class span_formatter_flag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg &msg,
              const std::tm &, spdlog::memory_buf_t &dest) override {
    if (msg.level > spdlog::level::debug) return;
    auto ctx = conky::log::current_span_context();
    if (!ctx.empty()) {
      std::string out = " " + ctx;
      dest.append(out.data(), out.data() + out.size());
    }
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<span_formatter_flag>();
  }
};

// Custom spdlog formatter flag that appends per-message attributes
// Only visible at trace level
class msg_attr_formatter_flag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg &msg,
              const std::tm &, spdlog::memory_buf_t &dest) override {
    if (msg.level > spdlog::level::trace) return;
    if (tl_msg_attrs.empty()) return;
    std::string result = " {";
    for (size_t i = 0; i < tl_msg_attrs.size(); i++) {
      if (i > 0) result += ' ';
      result += tl_msg_attrs[i].key;
      result += '=';
      result += tl_msg_attrs[i].value;
    }
    result += '}';
    dest.append(result.data(), result.data() + result.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<msg_attr_formatter_flag>();
  }
};

// Custom source location formatter that omits [file:line] when empty
class source_loc_formatter_flag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg &msg,
              const std::tm &, spdlog::memory_buf_t &dest) override {
    if (msg.source.empty()) return;
    const char *file = msg.source.filename;
    if (strlen(file) > project_root_len) { file += project_root_len; }
    bool hide_func = !stderr_sink->should_log(spdlog::level::debug) ||
        (strcmp(msg.source.funcname, "operator()") == 0 &&
         msg.payload.size() >= 2 && msg.payload[0] == '>' && msg.payload[1] == '>');
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

void conky::log::push_msg_attrs(std::initializer_list<attribute> attrs) {
  tl_msg_attrs.insert(tl_msg_attrs.end(), attrs.begin(), attrs.end());
}

void conky::log::clear_msg_attrs() {
  tl_msg_attrs.clear();
}

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
  formatter->add_flag<span_formatter_flag>('*');
  formatter->add_flag<msg_attr_formatter_flag>('&');
  formatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%-5l]%^%* %v%&");
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
