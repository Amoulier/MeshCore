#include <gtest/gtest.h>

#include <helpers/sensors/GpsDutyCyclePolicy.h>

namespace {

using gps::GpsDutyCycleConfig;
using gps::GpsDutyCyclePolicy;
using Action = GpsDutyCyclePolicy::Action;
using State = GpsDutyCyclePolicy::State;

TEST(GpsDutyCycle, DefaultWithoutOptInPreservesContinuousUserEnabledGps) {
  GpsDutyCyclePolicy policy;
  EXPECT_EQ(State::Off, policy.state());
  EXPECT_FALSE(policy.receiverOn());
  EXPECT_EQ(Action::Start, policy.update(100, true, false, false, 0));
  EXPECT_EQ(State::Continuous, policy.state());
  EXPECT_TRUE(policy.receiverOn());
  EXPECT_EQ(Action::None, policy.update(4000000000U, true, false, true, 10));
  EXPECT_EQ(UINT32_MAX, policy.millisUntilTransition(4000000000U));
  EXPECT_EQ(Action::Stop, policy.update(4000000001U, false, false, true, 10));
}

TEST(GpsDutyCycle, PowerSavingAloneNeverEnablesAnUnrequestedGps) {
  GpsDutyCyclePolicy policy;
  EXPECT_EQ(Action::None, policy.update(0, false, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(UINT32_MAX, false, true, true, 123));
  EXPECT_EQ(Action::None, policy.update(10, false, false, true, 123));
  EXPECT_EQ(State::Off, policy.state());
}

TEST(GpsDutyCycle, NoFixTimesOutAndSleepsForTheWholeRestPeriod) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 1)));
  EXPECT_EQ(Action::Start, policy.update(100, true, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(5099, true, true, false, 0));
  EXPECT_EQ(1U, policy.millisUntilTransition(5099));
  EXPECT_EQ(Action::Stop, policy.update(5100, true, true, false, 0));
  EXPECT_EQ(State::Resting, policy.state());
  EXPECT_FALSE(policy.receiverOn());
  EXPECT_EQ(Action::None, policy.update(15099, true, true, false, 0));
  EXPECT_EQ(Action::Start, policy.update(15100, true, true, false, 0));
  EXPECT_EQ(State::Acquiring, policy.state());
}

TEST(GpsDutyCycle, ANewFixMustCompleteItsHoldPeriodBeforePowerDown) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, true, 7));
  EXPECT_EQ(Action::None, policy.update(1000, true, true, true, 7));
  EXPECT_EQ(Action::None, policy.update(2000, true, true, true, 8));
  EXPECT_EQ(5000U, policy.millisUntilTransition(2000));
  EXPECT_EQ(Action::None, policy.update(6999, true, true, true, 12));
  EXPECT_EQ(Action::Stop, policy.update(7000, true, true, true, 13));
}

TEST(GpsDutyCycle, CachedValidityCannotCountAsANewAcquisitionFix) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, true, 100));
  for (uint32_t now = 1000; now < 120000; now += 1000) {
    EXPECT_EQ(Action::None, policy.update(now, true, true, true, 100));
  }
  EXPECT_EQ(Action::Stop, policy.update(120000, true, true, true, 100));
}

TEST(GpsDutyCycle, ZeroHoldStillRequiresANewValidPositionSequence) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(120, 900, 0)));
  ASSERT_EQ(Action::Start, policy.update(50, true, true, true, 3));
  EXPECT_EQ(Action::None, policy.update(51, true, true, true, 3));
  EXPECT_EQ(Action::None, policy.update(52, true, true, false, 4));
  EXPECT_EQ(Action::Stop, policy.update(53, true, true, true, 5));
}

TEST(GpsDutyCycle, ANewCycleIgnoresThePreviousCyclesCachedFix) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(10, 1, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(100, true, true, true, 1));
  ASSERT_EQ(Action::Start, policy.update(1100, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(1101, true, true, true, 1));
  EXPECT_EQ(Action::Stop, policy.update(1200, true, true, true, 2));
}

TEST(GpsDutyCycle, LosingTheFixResetsItsHoldWithoutExtendingAcquisitionTimeout) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(1000, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(4000, true, true, false, 1));
  EXPECT_EQ(116000U, policy.millisUntilTransition(4000));
  EXPECT_EQ(Action::None, policy.update(5000, true, true, true, 2));
  EXPECT_EQ(Action::None, policy.update(9999, true, true, true, 6));
  EXPECT_EQ(Action::Stop, policy.update(10000, true, true, true, 7));
}

TEST(GpsDutyCycle, AcquisitionTimeoutWinsWhenALateFixCannotCompleteItsHold) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 5)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(4000, true, true, true, 1));
  EXPECT_EQ(1000U, policy.millisUntilTransition(4000));
  EXPECT_EQ(Action::Stop, policy.update(5000, true, true, true, 2));
}

TEST(GpsDutyCycle, UserOffStopsAnActiveReceiverImmediatelyAndOnlyOnce) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  EXPECT_EQ(Action::Stop, policy.update(1, false, true, true, 1));
  EXPECT_EQ(State::Off, policy.state());
  EXPECT_EQ(Action::None, policy.update(2, false, true, true, 2));
  EXPECT_EQ(Action::None, policy.update(900000, false, false, false, 2));
}

TEST(GpsDutyCycle, UserOffWhileRestingCancelsAllScheduledWakes) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 1, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(1, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(2, false, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(1001, false, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(1000000, false, false, true, 1));
  EXPECT_EQ(State::Off, policy.state());
  EXPECT_EQ(UINT32_MAX, policy.millisUntilTransition(1000000));
}

TEST(GpsDutyCycle, ReenablingAfterUserOffRestartsImmediatelyWithANewFixBaseline) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 86400, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(1, true, true, true, 1));
  ASSERT_EQ(Action::None, policy.update(2, false, true, true, 1));
  EXPECT_EQ(Action::Start, policy.update(2, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(3, true, true, true, 1));
  EXPECT_EQ(Action::Stop, policy.update(4, true, true, true, 2));
}

TEST(GpsDutyCycle, DisablingPowerSavingWhileRestingResumesContinuousReception) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 86400, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(1, true, true, true, 1));
  EXPECT_EQ(Action::Start, policy.update(2, true, false, true, 1));
  EXPECT_EQ(State::Continuous, policy.state());
  EXPECT_EQ(Action::None, policy.update(1000000, true, false, true, 2));
}

TEST(GpsDutyCycle, DisablingPowerSavingCannotOverrideUserOff) {
  GpsDutyCyclePolicy policy;
  EXPECT_EQ(Action::None, policy.update(0, false, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(1, false, false, false, 0));
  EXPECT_EQ(State::Off, policy.state());
  EXPECT_EQ(Action::Start, policy.update(2, true, false, false, 0));
}

TEST(GpsDutyCycle, EnablingPowerSavingWhileOnDoesNotPulseTheReceiverOrAcceptAnOldFix) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, false, true, 100));
  EXPECT_EQ(Action::None, policy.update(100, true, true, true, 120));
  EXPECT_EQ(State::Acquiring, policy.state());
  EXPECT_EQ(Action::None, policy.update(10000, true, true, true, 120));
  EXPECT_EQ(Action::None, policy.update(20000, true, false, true, 130));
  EXPECT_EQ(State::Continuous, policy.state());
}

TEST(GpsDutyCycle, InvalidConfigurationsAreRejectedAtomically) {
  GpsDutyCyclePolicy policy;
  const GpsDutyCycleConfig invalid[] = {
      {4, 900, 0}, {3601, 900, 5}, {120, 0, 5}, {120, 86401, 5},
      {120, 900, 61}, {5, 900, 6}, {UINT32_MAX, UINT32_MAX, UINT32_MAX},
  };
  for (const auto& config : invalid) {
    EXPECT_FALSE(policy.configure(config));
    EXPECT_EQ(120U, policy.config().acquire_seconds);
    EXPECT_EQ(900U, policy.config().rest_seconds);
    EXPECT_EQ(5U, policy.config().fix_hold_seconds);
  }
  EXPECT_TRUE(policy.configure(GpsDutyCycleConfig(5, 1, 0)));
  EXPECT_TRUE(policy.configure(GpsDutyCycleConfig(3600, 86400, 60)));
}

TEST(GpsDutyCycle, ShorteningRestIsAppliedFromItsOriginalStartOnTheNextUpdate) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 20, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(100, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(10100, true, true, true, 1));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 5, 0)));
  EXPECT_EQ(State::Resting, policy.state());
  EXPECT_EQ(0U, policy.millisUntilTransition(10100));
  EXPECT_EQ(Action::Start, policy.update(10100, true, true, true, 1));
}

TEST(GpsDutyCycle, LengtheningRestDoesNotRestartItsElapsedTime) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(100, true, true, true, 1));
  ASSERT_EQ(Action::None, policy.update(5100, true, true, true, 1));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 20, 0)));
  EXPECT_EQ(15000U, policy.millisUntilTransition(5100));
  EXPECT_EQ(Action::None, policy.update(20099, true, true, true, 1));
  EXPECT_EQ(Action::Start, policy.update(20100, true, true, true, 1));
}

TEST(GpsDutyCycle, ShorteningAcquisitionPreservesTheActualAcquisitionStart) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(100, true, true, false, 0));
  ASSERT_EQ(Action::None, policy.update(10100, true, true, false, 0));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 900, 5)));
  EXPECT_EQ(State::Acquiring, policy.state());
  EXPECT_EQ(0U, policy.millisUntilTransition(10100));
  EXPECT_EQ(Action::Stop, policy.update(10100, true, true, false, 0));
}

TEST(GpsDutyCycle, LengtheningAcquisitionKeepsTheElapsedAttempt) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 900, 5)));
  ASSERT_EQ(Action::Start, policy.update(100, true, true, false, 0));
  ASSERT_EQ(Action::None, policy.update(4100, true, true, false, 0));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(10, 900, 5)));
  EXPECT_EQ(6000U, policy.millisUntilTransition(4100));
  EXPECT_EQ(Action::None, policy.update(10099, true, true, false, 0));
  EXPECT_EQ(Action::Stop, policy.update(10100, true, true, false, 0));
}

TEST(GpsDutyCycle, ChangingFixHoldKeepsTheFirstNewFixTime) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::None, policy.update(1000, true, true, true, 1));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(120, 900, 10)));
  EXPECT_EQ(Action::None, policy.update(10999, true, true, true, 10));
  EXPECT_EQ(Action::Stop, policy.update(11000, true, true, true, 11));
}

TEST(GpsDutyCycle, ShorteningFixHoldCanCompleteTheExistingFreshFix) {
  GpsDutyCyclePolicy policy;
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::None, policy.update(1000, true, true, true, 1));
  ASSERT_EQ(Action::None, policy.update(3000, true, true, true, 2));
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(120, 900, 1)));
  EXPECT_EQ(State::Acquiring, policy.state());
  EXPECT_EQ(0U, policy.millisUntilTransition(3000));
  EXPECT_EQ(Action::Stop, policy.update(3000, true, true, true, 2));
}

TEST(GpsDutyCycle, AcquisitionTimeoutCrossesMillisRolloverAtTheExactBoundary) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 1)));
  const uint32_t start = 0xfffff000U;
  ASSERT_EQ(Action::Start, policy.update(start, true, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(start + 4999U, true, true, false, 0));
  EXPECT_EQ(1U, policy.millisUntilTransition(start + 4999U));
  EXPECT_EQ(Action::Stop, policy.update(start + 5000U, true, true, false, 0));
}

TEST(GpsDutyCycle, RestPeriodCrossesMillisRolloverAtTheExactBoundary) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 0)));
  const uint32_t start = 0xfffff000U;
  ASSERT_EQ(Action::Start, policy.update(start, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(start + 1000U, true, true, true, 1));
  EXPECT_EQ(Action::None, policy.update(start + 10999U, true, true, true, 1));
  EXPECT_EQ(1U, policy.millisUntilTransition(start + 10999U));
  EXPECT_EQ(Action::Start, policy.update(start + 11000U, true, true, true, 1));
}

TEST(GpsDutyCycle, FixHoldCrossesMillisRolloverWithoutAcceptingCachedData) {
  GpsDutyCyclePolicy policy;
  const uint32_t start = 0xfffff000U;
  ASSERT_EQ(Action::Start, policy.update(start, true, true, true, 10));
  ASSERT_EQ(Action::None, policy.update(start + 1000U, true, true, true, 11));
  EXPECT_EQ(Action::None, policy.update(start + 5999U, true, true, true, 12));
  EXPECT_EQ(Action::Stop, policy.update(start + 6000U, true, true, true, 13));
}

TEST(GpsDutyCycle, FixSequenceRolloverStillIdentifiesANewFix) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 1, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, true, UINT32_MAX));
  EXPECT_EQ(Action::None, policy.update(1, true, true, true, UINT32_MAX));
  EXPECT_EQ(Action::Stop, policy.update(2, true, true, true, 0));
}

TEST(GpsDutyCycle, TelemetryAndStatusReadsNeverWakeAnOverdueSleepingReceiver) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 1, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  ASSERT_EQ(Action::Stop, policy.update(1, true, true, true, 1));
  const GpsDutyCyclePolicy& observation = policy;
  for (uint32_t now : {500U, 1001U, 100000U}) {
    EXPECT_EQ(State::Resting, observation.state());
    EXPECT_FALSE(observation.receiverOn());
    EXPECT_EQ(now < 1001U ? 1001U - now : 0U, observation.millisUntilTransition(now));
    EXPECT_EQ(1U, observation.config().rest_seconds);
  }
  EXPECT_EQ(State::Resting, policy.state());
  EXPECT_EQ(Action::Start, policy.update(100000, true, true, true, 1));
}

TEST(GpsDutyCycle, StatusReadsNeverStopAnAcquisitionWhenItsDeadlinePasses) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  EXPECT_EQ(0U, policy.millisUntilTransition(100000));
  EXPECT_EQ(State::Acquiring, policy.state());
  EXPECT_TRUE(policy.receiverOn());
  EXPECT_EQ(Action::Stop, policy.update(100000, true, true, false, 0));
}

TEST(GpsDutyCycle, ADelayedPollStartsRestAtTheActualPowerDownTime) {
  GpsDutyCyclePolicy policy;
  ASSERT_TRUE(policy.configure(GpsDutyCycleConfig(5, 10, 0)));
  ASSERT_EQ(Action::Start, policy.update(0, true, true, false, 0));
  EXPECT_EQ(Action::Stop, policy.update(100000, true, true, false, 0));
  EXPECT_EQ(10000U, policy.millisUntilTransition(100000));
  EXPECT_EQ(Action::None, policy.update(109999, true, true, false, 0));
  EXPECT_EQ(Action::Start, policy.update(110000, true, true, false, 0));
  EXPECT_EQ(Action::None, policy.update(110000, true, true, false, 0));
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
