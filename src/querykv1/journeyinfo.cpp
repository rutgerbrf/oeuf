// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <iostream>

#include "journeyinfo.hpp"

void journeyInfo(const Options &options, Kv1Records &records, Kv1Index &index) {
  std::cout << "Info for journey " << options.line_planning_number
            << "/" << options.journey_number << std::endl;

  std::unordered_map<std::string, const Kv1UserStopPoint *> usrstops;
  for (size_t i = 0; i < records.user_stop_points.size(); i++) {
    const Kv1UserStopPoint *usrstop = &records.user_stop_points[i];
    usrstops[usrstop->key.user_stop_code] = usrstop;
  }

  for (const auto &pujo : records.public_journeys) {
    if (pujo.key.line_planning_number != options.line_planning_number
     || std::to_string(pujo.key.journey_number) != options.journey_number)
      continue;

    std::vector<const Kv1JourneyPatternTimingLink *> timing_links;
    for (size_t i = 0; i < records.journey_pattern_timing_links.size(); i++) {
      const Kv1JourneyPatternTimingLink *jopatili = &records.journey_pattern_timing_links[i];
      if (jopatili->key.line_planning_number != options.line_planning_number
       || jopatili->key.journey_pattern_code != pujo.journey_pattern_code)
        continue;
      timing_links.push_back(jopatili);
    }
  
    std::sort(timing_links.begin(), timing_links.end(), [](auto a, auto b) -> bool {
      return a->key.timing_link_order < b->key.timing_link_order;
    });
    auto begin_stop = timing_links.front()->user_stop_code_begin;
    auto end_stop   = timing_links.back()->user_stop_code_end;

    const auto *begin = usrstops[begin_stop];
    const auto *end   = usrstops[end_stop];

    std::cout << "  Journey pattern:  " << pujo.key.line_planning_number
              << "/" << pujo.journey_pattern_code << std::endl
              << "  Begin stop:       " << begin_stop
              << "; name: " << std::quoted(begin->name)
              << "; town: " << std::quoted(begin->town) << std::endl
              << "  End stop:         " << end_stop
              << "; name: " << std::quoted(end->name)
              << "; town: " << std::quoted(end->town) << std::endl;

    const auto *begin_star = begin->p_user_stop_area;
    const auto *end_star = end->p_user_stop_area;
    if (begin_star)
      std::cout << "  Begin stop area:  " << begin_star->key.user_stop_area_code
                << "; name: " << std::quoted(begin_star->name)
                << ", town: " << std::quoted(begin_star->town)
                << std::endl;
    if (end_star)
      std::cout << "  End stop area:    " << end_star->key.user_stop_area_code
                << "; name: " << std::quoted(end_star->name)
                << ", town: " << std::quoted(end_star->town)
                << std::endl;

    break;
  }
}
