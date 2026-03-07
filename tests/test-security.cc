#include <string>

#include "catch2/catch.hpp"

#include <config.h>

#ifdef BUILD_HTTP
#include <output/display-http.hh>
#endif
#ifdef BUILD_RSS
#include <data/network/rss.h>
#endif
#ifdef BUILD_CURL
#include <common.h>
#endif

#ifdef BUILD_HTTP
TEST_CASE("html_escape escapes active markup", "[security][http]") {
  REQUIRE(
      conky::html_escape("<script>alert('x') & \"y\"</script>") ==
      "&lt;script&gt;alert(&#39;x&#39;) &amp; &quot;y&quot;&lt;/script&gt;");
}
#endif

#ifdef BUILD_RSS
TEST_CASE("rss_safe_append truncates without overflowing", "[security][rss]") {
  char buffer[8] = "ab";

  rss_safe_append(buffer, sizeof(buffer), "cdef");
  REQUIRE(std::string(buffer) == "abcdef");

  rss_safe_append(buffer, sizeof(buffer), "ghijk");
  REQUIRE(std::string(buffer) == "abcdefg");
}
#endif

#ifdef BUILD_CURL
TEST_CASE("github_notifications uses auth header instead of query params",
          "[security][github]") {
  REQUIRE(github_notifications_url() == "https://api.github.com/notifications");
  REQUIRE(github_authorization_header("secret-token") ==
          "Authorization: Bearer secret-token");
}
#endif
