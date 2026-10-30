#include "catch2/catch.hpp"
#include "config.h"

#ifdef BUILD_PULSEAUDIO
#include "conky.h"
#include "data/audio/pulseaudio.h"
#include "lua/lua-config.hh"

TEST_CASE("pulseaudio source volume", "[pulseaudio][source_volume]") {
  pulseaudio_c audio;
  auto &result = audio.result;
  result.set_default_source("microphone");
  pa_source_info source{};
  source.name = "microphone";
  source.state = PA_SOURCE_RUNNING;
  pa_cvolume_set(&source.volume, 2, PA_VOLUME_NORM);

  SECTION("starts with no cached volume") {
    REQUIRE(result.source_volume == 0);
    REQUIRE(result.source_state == PA_SOURCE_SUSPENDED);
  }
  SECTION("averages channels and preserves configured volume while muted") {
    source.volume.values[0] = PA_VOLUME_NORM / 2;
    source.volume.values[1] = PA_VOLUME_NORM;
    source.mute = 1;
    result.update_source_info(source);
    REQUIRE(result.source_volume == 75);
    REQUIRE(result.source_mute == 1);
    REQUIRE(result.source_state == PA_SOURCE_RUNNING);

    source.mute = 0;
    pa_cvolume_set(&source.volume, 2, PA_VOLUME_MUTED);
    result.update_source_info(source);
    REQUIRE(result.source_volume == 0);
    REQUIRE(result.source_mute == 0);
  }
  SECTION("reports amplified volume") {
    pa_cvolume_set(&source.volume, 2, PA_VOLUME_NORM * 3 / 2);
    result.update_source_info(source);
    REQUIRE(result.source_volume == 150);
  }
  SECTION("default switch resets data and ignores a late old-source reply") {
    source.mute = 1;
    result.update_source_info(source);
    result.set_default_source("headset");
    REQUIRE(result.source_name == "headset");
    REQUIRE(result.source_volume == 0);
    REQUIRE(result.source_mute == 0);
    REQUIRE(result.source_state == PA_SOURCE_SUSPENDED);
    result.update_source_info(source);
    REQUIRE(result.source_volume == 0);
    REQUIRE(result.source_mute == 0);

    source.name = "headset";
    pa_cvolume_set(&source.volume, 1, PA_VOLUME_NORM / 4);
    result.update_source_info(source);
    REQUIRE(result.source_volume == 25);
    result.set_default_source("headset");
    REQUIRE(result.source_volume == 25);
  }
  SECTION("removing the default clears its data") {
    result.update_source_info(source);
    result.set_default_source(nullptr);
    REQUIRE(result.source_name.empty());
    REQUIRE(result.source_volume == 0);
    source.name = nullptr;
    result.update_source_info(source);
    REQUIRE(result.source_volume == 0);
  }
  SECTION("text accessor uses the source rather than the sink") {
    auto previous_state = std::move(state);
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    audio.cstate = PULSE_CONTEXT_READY;
    result.sink_volume = 90;
    result.source_volume = 25;
    text_object obj{};
    obj.data.opaque = &audio;
    char output[32]{};
    print_puau_source_volume(&obj, output, sizeof(output));
    CHECK(std::string(output) == "25");
    result.source_volume = 300;
    print_puau_source_volume(&obj, output, sizeof(output));
    CHECK(std::string(output) == "300");
    audio.cstate = PULSE_CONTEXT_FINISHED;
    print_puau_source_volume(&obj, output, sizeof(output));
    CHECK(std::string(output) == "0");
    state = std::move(previous_state);
  }
}
#endif
