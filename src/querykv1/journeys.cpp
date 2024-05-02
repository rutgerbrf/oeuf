// vim:set sw=2 ts=2 sts et:

#include <iostream>
#include <map>
#include <string_view>
#include <unordered_set>

#include "journeys.hpp"

using namespace std::string_view_literals;

void journeys(const Options &options, Kv1Records &records, Kv1Index &index) {
  const std::string_view want_begin_stop_code(options.begin_stop_code);
  const std::string_view want_end_stop_code(options.end_stop_code);

  FILE *out = stdout;
  if (options.output_file_path != "-"sv)
    out = fopen(options.output_file_path, "wb");
  if (!out) {
    fprintf(stderr, "Open %s: %s\n", options.output_file_path, strerrordesc_np(errno));
    exit(EXIT_FAILURE);
  }

  std::cerr << "Generating journeys for " << options.line_planning_number << ", going from stop "
            << options.begin_stop_code << " to " << options.end_stop_code << std::endl;

  std::unordered_map<std::string, const Kv1UserStopPoint *> usrstops;
  for (size_t i = 0; i < records.user_stop_points.size(); i++) {
    const Kv1UserStopPoint *usrstop = &records.user_stop_points[i];
    usrstops[usrstop->key.user_stop_code] = usrstop;
  }

  std::unordered_set<std::string> journey_pattern_codes;
  for (const auto &jopa : records.journey_patterns) {
    if (jopa.key.line_planning_number != options.line_planning_number)
      continue;
    journey_pattern_codes.insert(jopa.key.journey_pattern_code);
  }

  std::unordered_map<std::string, std::vector<const Kv1JourneyPatternTimingLink *>> jopatilis;
  for (size_t i = 0; i < records.journey_pattern_timing_links.size(); i++) {
    const Kv1JourneyPatternTimingLink *jopatili = &records.journey_pattern_timing_links[i];
    if (jopatili->key.line_planning_number != options.line_planning_number
     || !journey_pattern_codes.contains(jopatili->key.journey_pattern_code))
      continue;
    jopatilis[jopatili->key.journey_pattern_code].push_back(jopatili);
  }

  std::unordered_set<std::string> valid_jopas;
  for (auto &[journey_pattern_code, timing_links] : jopatilis) {
    std::sort(timing_links.begin(), timing_links.end(), [](auto a, auto b) -> bool {
      return a->key.timing_link_order < b->key.timing_link_order;
    });
    auto begin_stop = timing_links.front()->user_stop_code_begin;
    auto end_stop   = timing_links.back()->user_stop_code_end;

    const auto *begin = usrstops[begin_stop];
    const auto *end   = usrstops[end_stop];

    bool begin_stop_ok = false;
    if (want_begin_stop_code.starts_with("stop:"))
      begin_stop_ok = want_begin_stop_code.substr(5) == begin_stop;
    else if (want_begin_stop_code.starts_with("star:"))
      begin_stop_ok = want_begin_stop_code.substr(5) == begin->user_stop_area_code;

    bool end_stop_ok = false;
    if (want_end_stop_code.starts_with("stop:"))
      end_stop_ok = want_end_stop_code.substr(5) == end_stop;
    else if (want_end_stop_code.starts_with("star:"))
      end_stop_ok = want_end_stop_code.substr(5) == end->user_stop_area_code;

    if (begin_stop_ok && end_stop_ok) {
      valid_jopas.insert(journey_pattern_code);
    }
  }

  std::map<int, std::pair<std::string, std::string>> valid_journeys;
  for (const auto &pujo : records.public_journeys) {
    if (pujo.key.line_planning_number == options.line_planning_number
     && valid_jopas.contains(pujo.journey_pattern_code)) {
      valid_journeys[pujo.key.journey_number] = {
        pujo.time_demand_group_code,
        pujo.journey_pattern_code,
      };
    }
  }

  fputs("journey_number,time_demand_group_code,journey_pattern_code\n", out);
  for (const auto &[journey_number, timdemgrp_jopa] : valid_journeys) {
    const auto &[time_demand_group_code, journey_pattern_code] = timdemgrp_jopa;
    fprintf(out, "%d,%s,%s\n", journey_number, time_demand_group_code.c_str(), journey_pattern_code.c_str());
  }

  if (options.output_file_path != "-"sv) fclose(out);
}
