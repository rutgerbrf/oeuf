// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#ifndef OEUF_LIBTMI8_KV1_INDEX_HPP
#define OEUF_LIBTMI8_KV1_INDEX_HPP

#include <unordered_map>

#include <boost/container_hash/hash.hpp>

#include <tmi8/kv1_types.hpp>

struct Kv1Index {
  Kv1Records *records;

  explicit Kv1Index(Kv1Records *records);

  std::unordered_map<
    Kv1OrganizationalUnit::Key,
    Kv1OrganizationalUnit *,
    boost::hash<Kv1OrganizationalUnit::Key>> organizational_units;
  std::unordered_map<
    Kv1HigherOrganizationalUnit::Key,
    Kv1HigherOrganizationalUnit *,
    boost::hash<Kv1HigherOrganizationalUnit::Key>> higher_organizational_units;
  std::unordered_map<
    Kv1UserStopPoint::Key,
    Kv1UserStopPoint *,
    boost::hash<Kv1UserStopPoint::Key>> user_stop_points;
  std::unordered_map<
    Kv1UserStopArea::Key,
    Kv1UserStopArea *,
    boost::hash<Kv1UserStopArea::Key>> user_stop_areas;
  std::unordered_map<
    Kv1TimingLink::Key,
    Kv1TimingLink *,
    boost::hash<Kv1TimingLink::Key>> timing_links;
  std::unordered_map<
    Kv1Link::Key,
    Kv1Link *,
    boost::hash<Kv1Link::Key>> links;
  std::unordered_map<
    Kv1Line::Key,
    Kv1Line *,
    boost::hash<Kv1Line::Key>> lines;
  std::unordered_map<
    Kv1Destination::Key,
    Kv1Destination *,
    boost::hash<Kv1Destination::Key>> destinations;
  std::unordered_map<
    Kv1JourneyPattern::Key,
    Kv1JourneyPattern *,
    boost::hash<Kv1JourneyPattern::Key>> journey_patterns;
  std::unordered_map<
    Kv1ConcessionFinancerRelation::Key,
    Kv1ConcessionFinancerRelation *,
    boost::hash<Kv1ConcessionFinancerRelation::Key>> concession_financer_relations;
  std::unordered_map<
    Kv1ConcessionArea::Key,
    Kv1ConcessionArea *,
    boost::hash<Kv1ConcessionArea::Key>> concession_areas;
  std::unordered_map<
    Kv1Financer::Key,
    Kv1Financer *,
    boost::hash<Kv1Financer::Key>> financers;
  std::unordered_map<
    Kv1JourneyPatternTimingLink::Key,
    Kv1JourneyPatternTimingLink *,
    boost::hash<Kv1JourneyPatternTimingLink::Key>> journey_pattern_timing_links;
  std::unordered_map<
    Kv1Point::Key,
    Kv1Point *,
    boost::hash<Kv1Point::Key>> points;
  std::unordered_map<
    Kv1PointOnLink::Key,
    Kv1PointOnLink *,
    boost::hash<Kv1PointOnLink::Key>> point_on_links;
  std::unordered_map<
    Kv1Icon::Key,
    Kv1Icon *,
    boost::hash<Kv1Icon::Key>> icons;
  std::unordered_map<
    Kv1Notice::Key,
    Kv1Notice *,
    boost::hash<Kv1Notice::Key>> notices;
  std::unordered_map<
    Kv1TimeDemandGroup::Key,
    Kv1TimeDemandGroup *,
    boost::hash<Kv1TimeDemandGroup::Key>> time_demand_groups;
  std::unordered_map<
    Kv1TimeDemandGroupRunTime::Key,
    Kv1TimeDemandGroupRunTime *,
    boost::hash<Kv1TimeDemandGroupRunTime::Key>> time_demand_group_run_times;
  std::unordered_map<
    Kv1PeriodGroup::Key,
    Kv1PeriodGroup *,
    boost::hash<Kv1PeriodGroup::Key>> period_groups;
  std::unordered_map<
    Kv1SpecificDay::Key,
    Kv1SpecificDay *,
    boost::hash<Kv1SpecificDay::Key>> specific_days;
  std::unordered_map<
    Kv1TimetableVersion::Key,
    Kv1TimetableVersion *,
    boost::hash<Kv1TimetableVersion::Key>> timetable_versions;
  std::unordered_map<
    Kv1PublicJourney::Key,
    Kv1PublicJourney *,
    boost::hash<Kv1PublicJourney::Key>> public_journeys;
  std::unordered_map<
    Kv1PeriodGroupValidity::Key,
    Kv1PeriodGroupValidity *,
    boost::hash<Kv1PeriodGroupValidity::Key>> period_group_validities;
  std::unordered_map<
    Kv1ExceptionalOperatingDay::Key,
    Kv1ExceptionalOperatingDay *,
    boost::hash<Kv1ExceptionalOperatingDay::Key>> exceptional_operating_days;
  std::unordered_map<
    Kv1ScheduleVersion::Key,
    Kv1ScheduleVersion *,
    boost::hash<Kv1ScheduleVersion::Key>> schedule_versions;
  std::unordered_map<
    Kv1PublicJourneyPassingTimes::Key,
    Kv1PublicJourneyPassingTimes *,
    boost::hash<Kv1PublicJourneyPassingTimes::Key>> public_journey_passing_times;
  std::unordered_map<
    Kv1OperatingDay::Key,
    Kv1OperatingDay *,
    boost::hash<Kv1OperatingDay::Key>> operating_days;

  size_t size() const;
};

void kv1LinkRecords(Kv1Index &index);

#endif // OEUF_LIBTMI8_KV1_INDEX_HPP
