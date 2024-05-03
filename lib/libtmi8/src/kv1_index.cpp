// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <tmi8/kv1_index.hpp>

Kv1Index::Kv1Index(Kv1Records *records) : records(records) {
  organizational_units.reserve(records->organizational_units.size());
  for (size_t i = 0; i < records->organizational_units.size(); i++)  {
    auto *it = &records->organizational_units[i];
    organizational_units[it->key] = it;
  }
  higher_organizational_units.reserve(records->higher_organizational_units.size());
  for (size_t i = 0; i < records->higher_organizational_units.size(); i++) {
    auto *it = &records->higher_organizational_units[i];
    higher_organizational_units[it->key] = it;
  }
  user_stop_points.reserve(records->user_stop_points.size());
  for (size_t i = 0; i < records->user_stop_points.size(); i++) {
    auto *it = &records->user_stop_points[i];
    user_stop_points[it->key] = it;
  }
  user_stop_areas.reserve(records->user_stop_areas.size());
  for (size_t i = 0; i < records->user_stop_areas.size(); i++) {
    auto *it = &records->user_stop_areas[i];
    user_stop_areas[it->key] = it;
  }
  timing_links.reserve(records->timing_links.size());
  for (size_t i = 0; i < records->timing_links.size(); i++) {
    auto *it = &records->timing_links[i];
    timing_links[it->key] = it;
  }
  links.reserve(records->links.size());
  for (size_t i = 0; i < records->links.size(); i++) {
    auto *it = &records->links[i];
    links[it->key] = it;
  }
  lines.reserve(records->lines.size());
  for (size_t i = 0; i < records->lines.size(); i++) {
    auto *it = &records->lines[i];
    lines[it->key] = it;
  }
  destinations.reserve(records->destinations.size());
  for (size_t i = 0; i < records->destinations.size(); i++) {
    auto *it = &records->destinations[i];
    destinations[it->key] = it;
  }
  journey_patterns.reserve(records->journey_patterns.size());
  for (size_t i = 0; i < records->journey_patterns.size(); i++) {
    auto *it = &records->journey_patterns[i];
    journey_patterns[it->key] = it;
  }
  concession_financer_relations.reserve(records->concession_financer_relations.size());
  for (size_t i = 0; i < records->concession_financer_relations.size(); i++) {
    auto *it = &records->concession_financer_relations[i];
    concession_financer_relations[it->key] = it;
  }
  concession_areas.reserve(records->concession_areas.size());
  for (size_t i = 0; i < records->concession_areas.size(); i++) {
    auto *it = &records->concession_areas[i];
    concession_areas[it->key] = it;
  }
  financers.reserve(records->financers.size());
  for (size_t i = 0; i < records->financers.size(); i++) {
    auto *it = &records->financers[i];
    financers[it->key] = it;
  }
  journey_pattern_timing_links.reserve(records->journey_pattern_timing_links.size());
  for (size_t i = 0; i < records->journey_pattern_timing_links.size(); i++) {
    auto *it = &records->journey_pattern_timing_links[i];
    journey_pattern_timing_links[it->key] = it;
  }
  points.reserve(records->points.size());
  for (size_t i = 0; i < records->points.size(); i++) {
    auto *it = &records->points[i];
    points[it->key] = it;
  }
  point_on_links.reserve(records->point_on_links.size());
  for (size_t i = 0; i < records->point_on_links.size(); i++) {
    auto *it = &records->point_on_links[i];
    point_on_links[it->key] = it;
  }
  icons.reserve(records->icons.size());
  for (size_t i = 0; i < records->icons.size(); i++) {
    auto *it = &records->icons[i];
    icons[it->key] = it;
  }
  notices.reserve(records->notices.size());
  for (size_t i = 0; i < records->notices.size(); i++) {
    auto *it = &records->notices[i];
    notices[it->key] = it;
  }
  time_demand_groups.reserve(records->time_demand_groups.size());
  for (size_t i = 0; i < records->time_demand_groups.size(); i++) {
    auto *it = &records->time_demand_groups[i];
    time_demand_groups[it->key] = it;
  }
  time_demand_group_run_times.reserve(records->time_demand_group_run_times.size());
  for (size_t i = 0; i < records->time_demand_group_run_times.size(); i++) {
    auto *it = &records->time_demand_group_run_times[i];
    time_demand_group_run_times[it->key] = it;
  }
  period_groups.reserve(records->period_groups.size());
  for (size_t i = 0; i < records->period_groups.size(); i++) {
    auto *it = &records->period_groups[i];
    period_groups[it->key] = it;
  }
  specific_days.reserve(records->specific_days.size());
  for (size_t i = 0; i < records->specific_days.size(); i++) {
    auto *it = &records->specific_days[i];
    specific_days[it->key] = it;
  }
  timetable_versions.reserve(records->timetable_versions.size());
  for (size_t i = 0; i < records->timetable_versions.size(); i++) {
    auto *it = &records->timetable_versions[i];
    timetable_versions[it->key] = it;
  }
  public_journeys.reserve(records->public_journeys.size());
  for (size_t i = 0; i < records->public_journeys.size(); i++) {
    auto *it = &records->public_journeys[i];
    public_journeys[it->key] = it;
  }
  period_group_validities.reserve(records->period_group_validities.size());
  for (size_t i = 0; i < records->period_group_validities.size(); i++) {
    auto *it = &records->period_group_validities[i];
    period_group_validities[it->key] = it;
  }
  exceptional_operating_days.reserve(records->exceptional_operating_days.size());
  for (size_t i = 0; i < records->exceptional_operating_days.size(); i++) {
    auto *it = &records->exceptional_operating_days[i];
    exceptional_operating_days[it->key] = it;
  }
  schedule_versions.reserve(records->schedule_versions.size());
  for (size_t i = 0; i < records->schedule_versions.size(); i++) {
    auto *it = &records->schedule_versions[i];
    schedule_versions[it->key] = it;
  }
  public_journey_passing_times.reserve(records->public_journey_passing_times.size());
  for (size_t i = 0; i < records->public_journey_passing_times.size(); i++) {
    auto *it = &records->public_journey_passing_times[i];
    public_journey_passing_times[it->key] = it;
  }
  operating_days.reserve(records->operating_days.size());
  for (size_t i = 0; i < records->operating_days.size(); i++) {
    auto *it = &records->operating_days[i];
    operating_days[it->key] = it;
  }
}

size_t Kv1Index::size() const {
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

void kv1LinkRecords(Kv1Index &index) {
  for (auto &orunorun : index.records->higher_organizational_units) {
    Kv1OrganizationalUnit::Key orun_parent_key(
      orunorun.key.data_owner_code,
      orunorun.key.organizational_unit_code_parent);
    Kv1OrganizationalUnit::Key orun_child_key(
      orunorun.key.data_owner_code,
      orunorun.key.organizational_unit_code_child);
    orunorun.p_organizational_unit_parent = index.organizational_units[orun_parent_key];
    orunorun.p_organizational_unit_child  = index.organizational_units[orun_child_key];
  }
  for (auto &usrstop : index.records->user_stop_points) {
    Kv1Point::Key point_key(
      usrstop.key.data_owner_code,
      usrstop.key.user_stop_code);
    usrstop.p_point = index.points[point_key];
    if (!usrstop.user_stop_area_code.empty()) {
      Kv1UserStopArea::Key usrstar_key(
        usrstop.key.data_owner_code,
        usrstop.user_stop_area_code);
      usrstop.p_user_stop_area = index.user_stop_areas[usrstar_key];
    }
  }
  for (auto &tili : index.records->timing_links) {
    Kv1UserStopPoint::Key usrstop_begin_key(
      tili.key.data_owner_code,
      tili.key.user_stop_code_begin);
    Kv1UserStopPoint::Key usrstop_end_key(
      tili.key.data_owner_code,
      tili.key.user_stop_code_end);
    tili.p_user_stop_begin = index.user_stop_points[usrstop_begin_key];
    tili.p_user_stop_end   = index.user_stop_points[usrstop_end_key];
  }
  for (auto &link : index.records->links) {
    Kv1UserStopPoint::Key usrstop_begin_key(
      link.key.data_owner_code,
      link.key.user_stop_code_begin);
    Kv1UserStopPoint::Key usrstop_end_key(
      link.key.data_owner_code,
      link.key.user_stop_code_end);
    link.p_user_stop_begin = index.user_stop_points[usrstop_begin_key];
    link.p_user_stop_end   = index.user_stop_points[usrstop_end_key];
  }
  for (auto &line : index.records->lines) {
    if (!line.line_icon)
      continue;
    Kv1Icon::Key icon_key(
      line.key.data_owner_code,
      *line.line_icon);
    line.p_line_icon = index.icons[icon_key];
  }
  for (auto &jopa : index.records->journey_patterns) {
    Kv1Line::Key line_key(
      jopa.key.data_owner_code,
      jopa.key.line_planning_number);
    jopa.p_line = index.lines[line_key];
  }
  for (auto &confinrel : index.records->concession_financer_relations) {
    Kv1ConcessionArea::Key conarea_key(
      confinrel.key.data_owner_code,
      confinrel.concession_area_code);
    confinrel.p_concession_area = index.concession_areas[conarea_key];
    if (!confinrel.financer_code.empty()) {
      Kv1Financer::Key financer_key(
        confinrel.key.data_owner_code,
        confinrel.financer_code);
      confinrel.p_financer = index.financers[financer_key];
    }
  }
  for (auto &jopatili : index.records->journey_pattern_timing_links) {
    Kv1Line::Key line_key(
      jopatili.key.data_owner_code,
      jopatili.key.line_planning_number);
    Kv1JourneyPattern::Key jopa_key(
      jopatili.key.data_owner_code,
      jopatili.key.line_planning_number,
      jopatili.key.journey_pattern_code);
    Kv1UserStopPoint::Key usrstop_begin_key(
      jopatili.key.data_owner_code,
      jopatili.user_stop_code_begin);
    Kv1UserStopPoint::Key usrstop_end_key(
      jopatili.key.data_owner_code,
      jopatili.user_stop_code_end);
    Kv1ConcessionFinancerRelation::Key confinrel_key(
      jopatili.key.data_owner_code,
      jopatili.con_fin_rel_code);
    Kv1Destination::Key dest_key(
      jopatili.key.data_owner_code,
      jopatili.dest_code);
    jopatili.p_line            = index.lines[line_key];
    jopatili.p_journey_pattern = index.journey_patterns[jopa_key];
    jopatili.p_user_stop_begin = index.user_stop_points[usrstop_begin_key];
    jopatili.p_user_stop_end   = index.user_stop_points[usrstop_end_key];
    jopatili.p_con_fin_rel     = index.concession_financer_relations[confinrel_key];
    jopatili.p_dest            = index.destinations[dest_key];
    if (jopatili.line_dest_icon) {
      Kv1Icon::Key icon_key{
        jopatili.key.data_owner_code,
        *jopatili.line_dest_icon,
      };
      jopatili.p_line_dest_icon = index.icons[icon_key];
    }
  }
  for (auto &pool : index.records->point_on_links) {
    Kv1UserStopPoint::Key usrstop_begin_key(
      pool.key.data_owner_code,
      pool.key.user_stop_code_begin);
    Kv1UserStopPoint::Key usrstop_end_key(
      pool.key.data_owner_code,
      pool.key.user_stop_code_end);
    Kv1Point::Key point_key(
      pool.key.point_data_owner_code,
      pool.key.point_code);
    pool.p_user_stop_begin = index.user_stop_points[usrstop_begin_key];
    pool.p_user_stop_end   = index.user_stop_points[usrstop_end_key];
    pool.p_point           = index.points[point_key];
  }
  for (auto &ntcassgnm : index.records->notice_assignments) {
    Kv1Notice::Key notice_key(
      ntcassgnm.data_owner_code,
      ntcassgnm.notice_code);
    ntcassgnm.p_notice = index.notices[notice_key];
  }
  for (auto &timdemgrp : index.records->time_demand_groups) {
    Kv1Line::Key line_key(
      timdemgrp.key.data_owner_code,
      timdemgrp.key.line_planning_number);
    Kv1JourneyPattern::Key jopa_key(
      timdemgrp.key.data_owner_code,
      timdemgrp.key.line_planning_number,
      timdemgrp.key.journey_pattern_code);
    timdemgrp.p_line            = index.lines[line_key];
    timdemgrp.p_journey_pattern = index.journey_patterns[jopa_key];
  }
  for (auto &timdemrnt : index.records->time_demand_group_run_times) {
    Kv1Line::Key line_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.key.line_planning_number);
    Kv1JourneyPattern::Key jopa_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.key.line_planning_number,
      timdemrnt.key.journey_pattern_code);
    Kv1TimeDemandGroup::Key timdemgrp_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.key.line_planning_number,
      timdemrnt.key.journey_pattern_code,
      timdemrnt.key.time_demand_group_code);
    Kv1UserStopPoint::Key usrstop_begin_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.user_stop_code_begin);
    Kv1UserStopPoint::Key usrstop_end_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.user_stop_code_end);
    Kv1JourneyPatternTimingLink::Key jopatili_key(
      timdemrnt.key.data_owner_code,
      timdemrnt.key.line_planning_number,
      timdemrnt.key.journey_pattern_code,
      timdemrnt.key.timing_link_order);
    timdemrnt.p_line                        = index.lines[line_key];
    timdemrnt.p_user_stop_end               = index.user_stop_points[usrstop_end_key];
    timdemrnt.p_user_stop_begin             = index.user_stop_points[usrstop_begin_key];
    timdemrnt.p_journey_pattern             = index.journey_patterns[jopa_key];
    timdemrnt.p_time_demand_group           = index.time_demand_groups[timdemgrp_key];
    timdemrnt.p_journey_pattern_timing_link = index.journey_pattern_timing_links[jopatili_key];
  }
  for (auto &tive : index.records->timetable_versions) {
    Kv1OrganizationalUnit::Key orun_key(
      tive.key.data_owner_code,
      tive.key.organizational_unit_code);
    Kv1PeriodGroup::Key pegr_key(
      tive.key.data_owner_code,
      tive.key.period_group_code);
    Kv1SpecificDay::Key specday_key(
      tive.key.data_owner_code,
      tive.key.specific_day_code);
    tive.p_organizational_unit = index.organizational_units[orun_key];
    tive.p_period_group        = index.period_groups[pegr_key];
    tive.p_specific_day        = index.specific_days[specday_key];
  }
  for (auto &pujo : index.records->public_journeys) {
    Kv1TimetableVersion::Key tive_key(
      pujo.key.data_owner_code,
      pujo.key.organizational_unit_code,
      pujo.key.timetable_version_code,
      pujo.key.period_group_code,
      pujo.key.specific_day_code);
    Kv1OrganizationalUnit::Key orun_key(
      pujo.key.data_owner_code,
      pujo.key.organizational_unit_code);
    Kv1PeriodGroup::Key pegr_key(
      pujo.key.data_owner_code,
      pujo.key.period_group_code);
    Kv1SpecificDay::Key specday_key(
      pujo.key.data_owner_code,
      pujo.key.specific_day_code);
    Kv1Line::Key line_key(
      pujo.key.data_owner_code,
      pujo.key.line_planning_number);
    Kv1TimeDemandGroup::Key timdemgrp_key(
      pujo.key.data_owner_code,
      pujo.key.line_planning_number,
      pujo.journey_pattern_code,
      pujo.time_demand_group_code);
    Kv1JourneyPattern::Key jopa_key(
      pujo.key.data_owner_code,
      pujo.key.line_planning_number,
      pujo.journey_pattern_code);
    pujo.p_timetable_version   = index.timetable_versions[tive_key];
    pujo.p_organizational_unit = index.organizational_units[orun_key];
    pujo.p_period_group        = index.period_groups[pegr_key];
    pujo.p_specific_day        = index.specific_days[specday_key];
    pujo.p_line                = index.lines[line_key];
    pujo.p_time_demand_group   = index.time_demand_groups[timdemgrp_key];
    pujo.p_journey_pattern     = index.journey_patterns[jopa_key];
  }
  for (auto &pegrval : index.records->period_group_validities) {
    Kv1OrganizationalUnit::Key orun_key(
      pegrval.key.data_owner_code,
      pegrval.key.organizational_unit_code);
    Kv1PeriodGroup::Key pegr_key(
      pegrval.key.data_owner_code,
      pegrval.key.period_group_code);
    pegrval.p_organizational_unit = index.organizational_units[orun_key];
    pegrval.p_period_group        = index.period_groups[pegr_key];
  }
  for (auto &excopday : index.records->exceptional_operating_days) {
    Kv1OrganizationalUnit::Key orun_key(
      excopday.key.data_owner_code,
      excopday.key.organizational_unit_code);
    Kv1SpecificDay::Key specday_key(
      excopday.key.data_owner_code,
      excopday.specific_day_code);
    Kv1PeriodGroup::Key pegr_key(
      excopday.key.data_owner_code,
      excopday.period_group_code);
    excopday.p_organizational_unit = index.organizational_units[orun_key];
    excopday.p_specific_day        = index.specific_days[specday_key];
    excopday.p_period_group        = index.period_groups[pegr_key];
  }
  for (auto &schedvers : index.records->schedule_versions) {
    Kv1OrganizationalUnit::Key orun_key(
      schedvers.key.data_owner_code,
      schedvers.key.organizational_unit_code);
    schedvers.p_organizational_unit = index.organizational_units[orun_key];
  }
  for (auto &pujopass : index.records->public_journey_passing_times) {
    Kv1OrganizationalUnit::Key orun_key(
      pujopass.key.data_owner_code,
      pujopass.key.organizational_unit_code);
    Kv1ScheduleVersion::Key schedvers_key(
      pujopass.key.data_owner_code,
      pujopass.key.organizational_unit_code,
      pujopass.key.schedule_code,
      pujopass.key.schedule_type_code);
    Kv1Line::Key line_key(
      pujopass.key.data_owner_code,
      pujopass.key.line_planning_number);
    Kv1JourneyPattern::Key jopa_key(
      pujopass.key.data_owner_code,
      pujopass.key.line_planning_number,
      pujopass.journey_pattern_code);
    Kv1UserStopPoint::Key usrstop_key(
      pujopass.key.data_owner_code,
      pujopass.user_stop_code);
    pujopass.p_organizational_unit = index.organizational_units[orun_key];
    pujopass.p_schedule_version    = index.schedule_versions[schedvers_key];
    pujopass.p_line                = index.lines[line_key];
    pujopass.p_journey_pattern     = index.journey_patterns[jopa_key];
    pujopass.p_user_stop           = index.user_stop_points[usrstop_key];
  }
  for (auto &operday : index.records->operating_days) {
    Kv1OrganizationalUnit::Key orun_key(
      operday.key.data_owner_code,
      operday.key.organizational_unit_code);
    Kv1ScheduleVersion::Key schedvers_key(
      operday.key.data_owner_code,
      operday.key.organizational_unit_code,
      operday.key.schedule_code,
      operday.key.schedule_type_code);
    operday.p_organizational_unit = index.organizational_units[orun_key];
    operday.p_schedule_version    = index.schedule_versions[schedvers_key];
  }
}
