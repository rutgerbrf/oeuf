// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <boost/container_hash/hash.hpp>

#include <tmi8/kv1_types.hpp>

size_t Kv1Records::size() const {
  return organizational_units.size()
       + higher_organizational_units.size()
       + user_stop_points.size()
       + user_stop_areas.size()
       + timing_links.size()
       + links.size()
       + lines.size()
       + destinations.size()
       + journey_patterns.size()
       + concession_financer_relations.size()
       + concession_areas.size()
       + financers.size()
       + journey_pattern_timing_links.size()
       + points.size()
       + point_on_links.size()
       + icons.size()
       + notices.size()
       + notice_assignments.size()
       + time_demand_groups.size()
       + time_demand_group_run_times.size()
       + period_groups.size()
       + specific_days.size()
       + timetable_versions.size()
       + public_journeys.size()
       + period_group_validities.size()
       + exceptional_operating_days.size()
       + schedule_versions.size()
       + public_journey_passing_times.size()
       + operating_days.size();
}

Kv1OrganizationalUnit::Key::Key(
    std::string data_owner_code,
    std::string organizational_unit_code)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code))
{}

Kv1HigherOrganizationalUnit::Key::Key(
    std::string data_owner_code,
    std::string organizational_unit_code_parent,
    std::string organizational_unit_code_child,
    std::chrono::year_month_day valid_from)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code_parent(std::move(organizational_unit_code_parent)),
    organizational_unit_code_child(std::move(organizational_unit_code_child)),
    valid_from(valid_from)
{}

Kv1UserStopPoint::Key::Key(
    std::string data_owner_code,
    std::string user_stop_code)
  : data_owner_code(std::move(data_owner_code)),
    user_stop_code(std::move(user_stop_code))
{}

Kv1UserStopArea::Key::Key(
    std::string data_owner_code,
    std::string user_stop_area_code)
  : data_owner_code(std::move(data_owner_code)),
    user_stop_area_code(std::move(user_stop_area_code))
{}

Kv1TimingLink::Key::Key(
    std::string data_owner_code,
    std::string user_stop_code_begin,
    std::string user_stop_code_end)
  : data_owner_code(std::move(data_owner_code)),
    user_stop_code_begin(std::move(user_stop_code_begin)),
    user_stop_code_end(std::move(user_stop_code_end))
{}

Kv1Link::Key::Key(std::string data_owner_code,
                  std::string user_stop_code_begin,
                  std::string user_stop_code_end,
                  std::string transport_type)
  : data_owner_code(std::move(data_owner_code)),
    user_stop_code_begin(std::move(user_stop_code_begin)),
    user_stop_code_end(std::move(user_stop_code_end)),
    transport_type(std::move(transport_type))
{}

Kv1Line::Key::Key(std::string data_owner_code,
                  std::string line_planning_number)
  : data_owner_code(std::move(data_owner_code)),
    line_planning_number(std::move(line_planning_number))
{}

Kv1Destination::Key::Key(std::string data_owner_code,
                         std::string dest_code)
  : data_owner_code(std::move(data_owner_code)),
    dest_code(std::move(dest_code))
{}

Kv1JourneyPattern::Key::Key(std::string data_owner_code,
                            std::string line_planning_number,
                            std::string journey_pattern_code)
  : data_owner_code(std::move(data_owner_code)),
    line_planning_number(std::move(line_planning_number)),
    journey_pattern_code(std::move(journey_pattern_code))
{}

Kv1ConcessionFinancerRelation::Key::Key(std::string data_owner_code,
                                        std::string con_fin_rel_code)
  : data_owner_code(std::move(data_owner_code)),
    con_fin_rel_code(std::move(con_fin_rel_code))
{}

Kv1ConcessionArea::Key::Key(std::string data_owner_code,
                            std::string concession_area_code)
  : data_owner_code(std::move(data_owner_code)),
    concession_area_code(std::move(concession_area_code))
{}

Kv1Financer::Key::Key(std::string data_owner_code,
                      std::string financer_code)
  : data_owner_code(std::move(data_owner_code)),
    financer_code(std::move(financer_code))
{}

Kv1JourneyPatternTimingLink::Key::Key(std::string data_owner_code,
                                      std::string line_planning_number,
                                      std::string journey_pattern_code,
                                      short timing_link_order)
  : data_owner_code(std::move(data_owner_code)),
    line_planning_number(std::move(line_planning_number)),
    journey_pattern_code(journey_pattern_code),
    timing_link_order(timing_link_order)
{}

Kv1Point::Key::Key(std::string data_owner_code,
                   std::string point_code)
  : data_owner_code(std::move(data_owner_code)),
    point_code(std::move(point_code))
{}

Kv1PointOnLink::Key::Key(std::string data_owner_code,
                         std::string user_stop_code_begin,
                         std::string user_stop_code_end,
                         std::string point_data_owner_code,
                         std::string point_code,
                         std::string transport_type)
  : data_owner_code(std::move(data_owner_code)),
    user_stop_code_begin(std::move(user_stop_code_begin)),
    user_stop_code_end(std::move(user_stop_code_end)),
    point_data_owner_code(std::move(point_data_owner_code)),
    point_code(std::move(point_code)),
    transport_type(std::move(transport_type))
{}

Kv1Icon::Key::Key(std::string data_owner_code,
                  short icon_number)
  : data_owner_code(std::move(data_owner_code)),
    icon_number(icon_number)
{}

Kv1Notice::Key::Key(std::string data_owner_code,
                    std::string notice_code)
  : data_owner_code(std::move(data_owner_code)),
    notice_code(std::move(notice_code))
{}

Kv1TimeDemandGroup::Key::Key(std::string data_owner_code,
                             std::string line_planning_number,
                             std::string journey_pattern_code,
                             std::string time_demand_group_code)
  : data_owner_code(std::move(data_owner_code)),
    line_planning_number(std::move(line_planning_number)),
    journey_pattern_code(std::move(journey_pattern_code)),
    time_demand_group_code(std::move(time_demand_group_code))
{}

Kv1TimeDemandGroupRunTime::Key::Key(std::string data_owner_code,
                                    std::string line_planning_number,
                                    std::string journey_pattern_code,
                                    std::string time_demand_group_code,
                                    short timing_link_order)
  : data_owner_code(std::move(data_owner_code)),
    line_planning_number(std::move(line_planning_number)),
    journey_pattern_code(std::move(journey_pattern_code)),
    time_demand_group_code(std::move(time_demand_group_code)),
    timing_link_order(std::move(timing_link_order))
{}

Kv1PeriodGroup::Key::Key(std::string data_owner_code,
                         std::string period_group_code)
  : data_owner_code(std::move(data_owner_code)),
    period_group_code(std::move(period_group_code))
{}

Kv1SpecificDay::Key::Key(std::string data_owner_code,
                         std::string specific_day_code)
  : data_owner_code(std::move(data_owner_code)),
    specific_day_code(std::move(specific_day_code))
{}

Kv1TimetableVersion::Key::Key(std::string data_owner_code,
                              std::string organizational_unit_code,
                              std::string timetable_version_code,
                              std::string period_group_code,
                              std::string specific_day_code)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    timetable_version_code(std::move(timetable_version_code)),
    period_group_code(std::move(period_group_code)),
    specific_day_code(std::move(specific_day_code))
{}

Kv1PublicJourney::Key::Key(std::string data_owner_code,
                           std::string timetable_version_code,
                           std::string organizational_unit_code,
                           std::string period_group_code,
                           std::string specific_day_code,
                           std::string day_type,
                           std::string line_planning_number,
                           int journey_number)
  : data_owner_code(std::move(data_owner_code)),
    timetable_version_code(std::move(timetable_version_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    period_group_code(std::move(period_group_code)),
    specific_day_code(std::move(specific_day_code)),
    day_type(std::move(day_type)),
    line_planning_number(std::move(line_planning_number)),
    journey_number(journey_number)
{}

Kv1PeriodGroupValidity::Key::Key(std::string data_owner_code,
                                 std::string organizational_unit_code,
                                 std::string period_group_code,
                                 std::chrono::year_month_day valid_from)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    period_group_code(std::move(period_group_code)),
    valid_from(valid_from)
{}

Kv1ExceptionalOperatingDay::Key::Key(std::string data_owner_code,
                                     std::string organizational_unit_code,
                                     std::chrono::sys_seconds valid_date)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    valid_date(valid_date)
{}

Kv1ScheduleVersion::Key::Key(std::string data_owner_code,
                             std::string organizational_unit_code,
                             std::string schedule_code,
                             std::string schedule_type_code)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    schedule_code(std::move(schedule_code)),
    schedule_type_code(std::move(schedule_type_code))
{}

Kv1PublicJourneyPassingTimes::Key::Key(std::string data_owner_code,
                                       std::string organizational_unit_code,
                                       std::string schedule_code,
                                       std::string schedule_type_code,
                                       std::string line_planning_number,
                                       int journey_number,
                                       short stop_order)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    schedule_code(std::move(schedule_code)),
    schedule_type_code(std::move(schedule_type_code)),
    line_planning_number(std::move(line_planning_number)),
    journey_number(journey_number),
    stop_order(stop_order)
{}

Kv1OperatingDay::Key::Key(std::string data_owner_code,
                          std::string organizational_unit_code,
                          std::string schedule_code,
                          std::string schedule_type_code,
                          std::chrono::year_month_day valid_date)
  : data_owner_code(std::move(data_owner_code)),
    organizational_unit_code(std::move(organizational_unit_code)),
    schedule_code(std::move(schedule_code)),
    schedule_type_code(std::move(schedule_type_code)),
    valid_date(valid_date)
{}

bool operator==(const Kv1OrganizationalUnit::Key &a, const Kv1OrganizationalUnit::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code;
}

bool operator==(const Kv1HigherOrganizationalUnit::Key &a, const Kv1HigherOrganizationalUnit::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code_parent == b.organizational_unit_code_parent
      && a.organizational_unit_code_child == b.organizational_unit_code_child;
}

bool operator==(const Kv1UserStopPoint::Key &a, const Kv1UserStopPoint::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.user_stop_code == b.user_stop_code;
}

bool operator==(const Kv1UserStopArea::Key &a, const Kv1UserStopArea::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.user_stop_area_code == b.user_stop_area_code;
}

bool operator==(const Kv1TimingLink::Key &a, const Kv1TimingLink::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.user_stop_code_begin == b.user_stop_code_begin
      && a.user_stop_code_end == b.user_stop_code_end;
}

bool operator==(const Kv1Link::Key &a, const Kv1Link::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.user_stop_code_begin == b.user_stop_code_begin
      && a.user_stop_code_end == b.user_stop_code_end
      && a.transport_type == b.transport_type;
}

bool operator==(const Kv1Line::Key &a, const Kv1Line::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.line_planning_number == b.line_planning_number;
}

bool operator==(const Kv1Destination::Key &a, const Kv1Destination::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.dest_code == b.dest_code;
}

bool operator==(const Kv1JourneyPattern::Key &a, const Kv1JourneyPattern::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.line_planning_number == b.line_planning_number
      && a.journey_pattern_code == b.journey_pattern_code;
}

bool operator==(const Kv1ConcessionFinancerRelation::Key &a, const Kv1ConcessionFinancerRelation::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.con_fin_rel_code == b.con_fin_rel_code;
}

bool operator==(const Kv1ConcessionArea::Key &a, const Kv1ConcessionArea::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.concession_area_code == b.concession_area_code;
}

bool operator==(const Kv1Financer::Key &a, const Kv1Financer::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.financer_code == b.financer_code;
}

bool operator==(const Kv1JourneyPatternTimingLink::Key &a, const Kv1JourneyPatternTimingLink::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.line_planning_number == b.line_planning_number
      && a.journey_pattern_code == b.journey_pattern_code
      && a.timing_link_order == b.timing_link_order;
}

bool operator==(const Kv1Point::Key &a, const Kv1Point::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.point_code == b.point_code;
}

bool operator==(const Kv1PointOnLink::Key &a, const Kv1PointOnLink::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.user_stop_code_begin == b.user_stop_code_begin
      && a.user_stop_code_end == b.user_stop_code_end
      && a.point_data_owner_code == b.point_data_owner_code
      && a.point_code == b.point_code
      && a.transport_type == b.transport_type;
}

bool operator==(const Kv1Icon::Key &a, const Kv1Icon::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.icon_number == b.icon_number;
}

bool operator==(const Kv1Notice::Key &a, const Kv1Notice::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.notice_code == b.notice_code;
}

bool operator==(const Kv1TimeDemandGroup::Key &a, const Kv1TimeDemandGroup::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.line_planning_number == b.line_planning_number
      && a.journey_pattern_code == b.journey_pattern_code
      && a.time_demand_group_code == b.time_demand_group_code;
}

bool operator==(const Kv1TimeDemandGroupRunTime::Key &a, const Kv1TimeDemandGroupRunTime::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.line_planning_number == b.line_planning_number
      && a.journey_pattern_code == b.journey_pattern_code
      && a.time_demand_group_code == b.time_demand_group_code
      && a.timing_link_order == b.timing_link_order;
}

bool operator==(const Kv1PeriodGroup::Key &a, const Kv1PeriodGroup::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.period_group_code == b.period_group_code;
}

bool operator==(const Kv1SpecificDay::Key &a, const Kv1SpecificDay::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.specific_day_code == b.specific_day_code;
}

bool operator==(const Kv1TimetableVersion::Key &a, const Kv1TimetableVersion::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.timetable_version_code == b.timetable_version_code
      && a.period_group_code == b.period_group_code
      && a.specific_day_code == b.specific_day_code;
}

bool operator==(const Kv1PublicJourney::Key &a, const Kv1PublicJourney::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.timetable_version_code == b.timetable_version_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.period_group_code == b.period_group_code
      && a.specific_day_code == b.specific_day_code
      && a.day_type == b.day_type
      && a.line_planning_number == b.line_planning_number
      && a.journey_number == b.journey_number;
}

bool operator==(const Kv1PeriodGroupValidity::Key &a, const Kv1PeriodGroupValidity::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.period_group_code == b.period_group_code
      && a.valid_from == b.valid_from;
}

bool operator==(const Kv1ExceptionalOperatingDay::Key &a, const Kv1ExceptionalOperatingDay::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.valid_date == b.valid_date;
}

bool operator==(const Kv1ScheduleVersion::Key &a, const Kv1ScheduleVersion::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.schedule_code == b.schedule_code
      && a.schedule_type_code == b.schedule_type_code;
}

bool operator==(const Kv1PublicJourneyPassingTimes::Key &a, const Kv1PublicJourneyPassingTimes::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.schedule_code == b.schedule_code
      && a.schedule_type_code == b.schedule_type_code
      && a.line_planning_number == b.line_planning_number
      && a.journey_number == b.journey_number
      && a.stop_order == b.stop_order;
}

bool operator==(const Kv1OperatingDay::Key &a, const Kv1OperatingDay::Key &b) {
  return a.data_owner_code == b.data_owner_code
      && a.organizational_unit_code == b.organizational_unit_code
      && a.schedule_code == b.schedule_code
      && a.schedule_type_code == b.schedule_type_code
      && a.valid_date == b.valid_date;
}

namespace std::chrono {
  static size_t hash_value(const year_month_day &ymd) {
    size_t seed = 0;
    
    boost::hash_combine(seed, int(ymd.year()));
    boost::hash_combine(seed, unsigned(ymd.month()));
    boost::hash_combine(seed, unsigned(ymd.day()));
  
    return seed;
  }

  static size_t hash_value(const sys_seconds &s) {
    return boost::hash<seconds::rep>()(s.time_since_epoch().count());
  }
}

size_t hash_value(const Kv1OrganizationalUnit::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);

  return seed;
}

size_t hash_value(const Kv1HigherOrganizationalUnit::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code_parent);
  boost::hash_combine(seed, k.organizational_unit_code_child);
  boost::hash_combine(seed, k.valid_from);

  return seed;
}

size_t hash_value(const Kv1UserStopPoint::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.user_stop_code);

  return seed;
}

size_t hash_value(const Kv1UserStopArea::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.user_stop_area_code);

  return seed;
}

size_t hash_value(const Kv1TimingLink::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.user_stop_code_begin);
  boost::hash_combine(seed, k.user_stop_code_end);

  return seed;
}

size_t hash_value(const Kv1Link::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.user_stop_code_begin);
  boost::hash_combine(seed, k.user_stop_code_end);
  boost::hash_combine(seed, k.transport_type);

  return seed;
}

size_t hash_value(const Kv1Line::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  
  return seed;
}

size_t hash_value(const Kv1Destination::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.dest_code);

  return seed;
}

size_t hash_value(const Kv1JourneyPattern::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_pattern_code);

  return seed;
}

size_t hash_value(const Kv1ConcessionFinancerRelation::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.con_fin_rel_code);

  return seed;
}

size_t hash_value(const Kv1ConcessionArea::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.concession_area_code);

  return seed;
}

size_t hash_value(const Kv1Financer::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.financer_code);

  return seed;
}

size_t hash_value(const Kv1JourneyPatternTimingLink::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_pattern_code);
  boost::hash_combine(seed, k.timing_link_order);

  return seed;
}

size_t hash_value(const Kv1Point::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.point_code);
  
  return seed;
}

size_t hash_value(const Kv1PointOnLink::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.user_stop_code_begin);
  boost::hash_combine(seed, k.user_stop_code_end);
  boost::hash_combine(seed, k.point_data_owner_code);
  boost::hash_combine(seed, k.point_code);
  boost::hash_combine(seed, k.transport_type);

  return seed;
}

size_t hash_value(const Kv1Icon::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.icon_number);

  return seed;
}

size_t hash_value(const Kv1Notice::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.notice_code);

  return seed;
}

size_t hash_value(const Kv1TimeDemandGroup::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_pattern_code);
  boost::hash_combine(seed, k.time_demand_group_code);

  return seed;
}

size_t hash_value(const Kv1TimeDemandGroupRunTime::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_pattern_code);
  boost::hash_combine(seed, k.time_demand_group_code);
  boost::hash_combine(seed, k.timing_link_order);

  return seed;
}

size_t hash_value(const Kv1PeriodGroup::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.period_group_code);

  return seed;
}

size_t hash_value(const Kv1SpecificDay::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.specific_day_code);

  return seed;
}

size_t hash_value(const Kv1TimetableVersion::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.timetable_version_code);
  boost::hash_combine(seed, k.period_group_code);
  boost::hash_combine(seed, k.specific_day_code);

  return seed;
}

size_t hash_value(const Kv1PublicJourney::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.timetable_version_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.period_group_code);
  boost::hash_combine(seed, k.specific_day_code);
  boost::hash_combine(seed, k.day_type);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_number);

  return seed;
}

size_t hash_value(const Kv1PeriodGroupValidity::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.period_group_code);
  boost::hash_combine(seed, k.valid_from);

  return seed;
}

size_t hash_value(const Kv1ExceptionalOperatingDay::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.valid_date);

  return seed;
}

size_t hash_value(const Kv1ScheduleVersion::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.schedule_code);
  boost::hash_combine(seed, k.schedule_type_code);

  return seed;
}

size_t hash_value(const Kv1PublicJourneyPassingTimes::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.schedule_code);
  boost::hash_combine(seed, k.schedule_type_code);
  boost::hash_combine(seed, k.line_planning_number);
  boost::hash_combine(seed, k.journey_number);
  boost::hash_combine(seed, k.stop_order);

  return seed;
}

size_t hash_value(const Kv1OperatingDay::Key &k) {
  size_t seed = 0;

  boost::hash_combine(seed, k.data_owner_code);
  boost::hash_combine(seed, k.organizational_unit_code);
  boost::hash_combine(seed, k.schedule_code);
  boost::hash_combine(seed, k.schedule_type_code);
  boost::hash_combine(seed, k.valid_date);

  return seed;
}
