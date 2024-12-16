#include "catch2/catch.hpp"
#include "mock/display-mock.hh"
#include "mock/mock.hh"

class testRunListener : public Catch::EventListenerBase {
 public:
  using Catch::EventListenerBase::EventListenerBase;

  void testRunStarting(Catch::TestRunInfo const&) {
    mock::__internal::init_display_output_mock();
  }
  void testRunEnded(Catch::TestRunStats const&) {
    mock::__internal::delete_display_output_mock();
  }
  void testCaseStarting(Catch::SectionInfo const&) {
    mock::__internal::state_changes.clear();
  }
};

CATCH_REGISTER_LISTENER(testRunListener)
