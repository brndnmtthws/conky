/*
 * To generate colour-names.cc, you must have gperf installed during build.
 * This is a dummy implementation for builds without gperf.
 * Color name matching will always return null (i.e. no match).
 */

struct rgb {
  const char *name;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

class color_name_hash {
 public:
  static const struct rgb *in_word_set(const char *str, size_t len);
};

const struct rgb *color_name_hash::in_word_set(const char *str, size_t len) {
  DBGP("color parsing not supported");
  return nullptr;
}
