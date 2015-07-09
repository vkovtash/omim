#include "testing/testing.hpp"

#include "routing/turns_sound.hpp"
#include "routing/turns_sound_settings.hpp"

#include "platform/location.hpp"

namespace
{
using namespace location;
using namespace routing::turns;
using namespace routing::turns::sound;

Settings const settingsMeters(20 /* notificationTimeSeconds */,
                              200 /* minNotificationDistanceUnits */,
                              700 /* maxNotificationDistanceUnits */,
                              {100, 200, 300, 400, 500, 600, 700} /* soundedDistancesUnits */,
                              LengthUnits::Meters /* lengthUnits */);

Settings const settingsFeet(20 /* notificationTimeSeconds */,
                            500 /* minNotificationDistanceUnits */,
                            2000 /* maxNotificationDistanceUnits */,
                            {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                            LengthUnits::Feet /* lengthUnits */);

// A error to compare two double after conversion feet to meters.
double const kEps = 1.;
// A error to compare two doubles which are almost equal.
double const kSmallEps = .001;

UNIT_TEST(TurnNotificationSettingsMetersTest)
{
  Settings const & settings = settingsMeters;

  TEST(settings.IsValid(), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 20., kEps), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(my::AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 300., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(300 /* distanceInUnits */), 300, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 100, ());

  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(0. /* distanceInUnits */), 200., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(10. /* distanceInUnits */), 200., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(20. /* distanceInUnits */), 400., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(35. /* distanceInUnits */), 700., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(200. /* distanceInUnits */), 700., kSmallEps), ());
}

UNIT_TEST(TurnNotificationSettingsFeetTest)
{
  Settings const & settings = settingsFeet;

  TEST(settings.IsValid(), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 65., kEps), ());
  TEST(my::AlmostEqualAbs(
      settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(my::AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 91., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(500 /* distanceInUnits */), 600, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 200, ());

  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(0. /* distanceInUnits */), 500., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(10. /* distanceInUnits */), 500., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(30. /* distanceInUnits */), 600., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(40. /* distanceInUnits */), 800., kSmallEps), ());
  TEST(my::AlmostEqualAbs(settings.ComputeTurnDistance(200. /* distanceInUnits */), 2000., kSmallEps), ());
}

UNIT_TEST(TurnNotificationSettingsNotValidTest)
{
  Settings settings1(20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 800, 600, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Feet /* lengthUnits */);
  TEST(!settings1.IsValid(), ());

  Settings settings2(20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Undefined /* lengthUnits */);
  TEST(!settings2.IsValid(), ());

  Settings settings3(20 /* notificationTimeSeconds */, 5000 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */,
                     {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     LengthUnits::Meters /* lengthUnits */);
  TEST(!settings3.IsValid(), ());
}

UNIT_TEST(TurnsSoundMetersTest)
{
  TurnsSound turnSound;
  turnSound.Enable(true);
  turnSound.SetSettings(settingsMeters);
  turnSound.Reset();
  turnSound.SetSpeedMetersPerSecond(30.);

  TurnItem turnItem(5 /* idx */, TurnDirection::TurnRight);
  FollowingInfo followInfo;

  ASSERT(followInfo.m_turnNotifications.empty(), ());

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 1000. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 700 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 700. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 699. /* distanceToTurnMeters */);
  TEST_EQUAL(followInfo.m_turnNotifications.size(), 1, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_distanceUnits, 600, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_exitNum, 0, ());
  TEST(!followInfo.m_turnNotifications[0].m_useThenInsteadOfDistance, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_turnDir, TurnDirection::TurnRight, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_lengthUnits, LengthUnits::Meters, ());

  // 650 meters till the turn. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 650. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  turnSound.SetSpeedMetersPerSecond(32.);

  // 150 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 150. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 100 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 100. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 99. /* distanceToTurnMeters */);
  TEST_EQUAL(followInfo.m_turnNotifications.size(), 1, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_distanceUnits, 0, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_exitNum, 0, ());
  TEST(!followInfo.m_turnNotifications[0].m_useThenInsteadOfDistance, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_turnDir, TurnDirection::TurnRight, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_lengthUnits, LengthUnits::Meters, ());

  // 99 meters till the turn again. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 99. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 50 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 50. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 0 meters till the turn. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 0. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  TEST(turnSound.IsEnabled(), ());
}

UNIT_TEST(TurnsSoundFeetTest)
{
  TurnsSound turnSound;
  turnSound.Enable(true);
  turnSound.SetSettings(settingsFeet);
  turnSound.Reset();
  turnSound.SetSpeedMetersPerSecond(30.);

  TurnItem turnItem(7 /* idx */, TurnDirection::EnterRoundAbout, 3 /* exitNum */);
  FollowingInfo followInfo;

  ASSERT(followInfo.m_turnNotifications.empty(), ());

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 1000. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 700 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 700. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 699. /* distanceToTurnMeters */);
  TEST_EQUAL(followInfo.m_turnNotifications.size(), 1, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_distanceUnits, 2000, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_exitNum, 3, ());
  TEST(!followInfo.m_turnNotifications[0].m_useThenInsteadOfDistance, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_turnDir, TurnDirection::EnterRoundAbout, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_lengthUnits, LengthUnits::Feet, ());

  // 650 meters till the turn. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 650. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 150 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 150. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 100 meters till the turn. No sound notifications is required.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 100. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 99. /* distanceToTurnMeters */);
  TEST_EQUAL(followInfo.m_turnNotifications.size(), 1, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_distanceUnits, 0, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_exitNum, 3, ());
  TEST(!followInfo.m_turnNotifications[0].m_useThenInsteadOfDistance, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_turnDir, TurnDirection::EnterRoundAbout, ());
  TEST_EQUAL(followInfo.m_turnNotifications[0].m_lengthUnits, LengthUnits::Feet, ());

  // 99 meters till the turn again. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 99. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 50 meters till the turn. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 50. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  // 0 meters till the turn. No sound notifications is required.
  followInfo.m_turnNotifications.clear();
  turnSound.UpdateRouteFollowingInfo(followInfo, turnItem, 0. /* distanceToTurnMeters */);
  TEST(followInfo.m_turnNotifications.empty(), ());

  TEST(turnSound.IsEnabled(), ());
}
}  // namespace