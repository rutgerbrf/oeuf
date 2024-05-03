// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <iostream>
#include <string_view>

#include "journeyroute.hpp"

using namespace std::string_view_literals;

void journeyRoute(const Options &options, Kv1Records &records, Kv1Index &index) {
  FILE *out = stdout;
  if (options.output_file_path != "-"sv)
    out = fopen(options.output_file_path, "wb");
  if (!out) {
    fprintf(stderr, "Open %s: %s\n", options.output_file_path, strerrordesc_np(errno));
    exit(EXIT_FAILURE);
  }

  for (auto &pujo : records.public_journeys) {
    if (pujo.key.line_planning_number == options.line_planning_number && std::to_string(pujo.key.journey_number) == options.journey_number) {
      fprintf(stderr, "Got PUJO %s/%s:\n", options.line_planning_number, options.journey_number);
      fprintf(stderr, "  Day type: %s\n", pujo.key.day_type.c_str());
      auto &pegr = *pujo.p_period_group;
      fprintf(stderr, "  PEGR Code: %s\n", pegr.key.period_group_code.c_str());
      fprintf(stderr, "  PEGR Description: %s\n", pegr.description.c_str());
      fprintf(stderr, "  SPECDAY Code: %s\n", pujo.key.specific_day_code.c_str());
      auto &timdemgrp = *pujo.p_time_demand_group;

      for (auto &pegrval : records.period_group_validities) {
        if (pegrval.key.period_group_code == pegr.key.period_group_code) {
          fprintf(stderr, "Got PEGRVAL for PEGR %s\n", pegr.key.period_group_code.c_str());
          std::cerr << "  Valid from: " << pegrval.key.valid_from << std::endl;
          std::cerr << "  Valid thru: " << pegrval.valid_thru << std::endl;
        }
      }

      struct Point {
        Kv1JourneyPatternTimingLink *jopatili = nullptr;
        Kv1TimeDemandGroupRunTime *timdemrnt = nullptr;
        double distance_since_start_of_link = 0;
        double rd_x = 0;
        double rd_y = 0;
        double total_time_s = 0;
      };
      std::vector<Point> points;

      for (size_t i = 0; i < records.time_demand_group_run_times.size(); i++) {
        Kv1TimeDemandGroupRunTime *timdemrnt = &records.time_demand_group_run_times[i];
        if (timdemrnt->key.line_planning_number   == timdemgrp.key.line_planning_number
         && timdemrnt->key.journey_pattern_code   == timdemgrp.key.journey_pattern_code
         && timdemrnt->key.time_demand_group_code == timdemgrp.key.time_demand_group_code) {
          Kv1JourneyPatternTimingLink *jopatili = timdemrnt->p_journey_pattern_timing_link;
          for (auto &pool : records.point_on_links) {
            if (pool.key.user_stop_code_begin == timdemrnt->user_stop_code_begin
             && pool.key.user_stop_code_end   == timdemrnt->user_stop_code_end
             && pool.key.transport_type       == jopatili->p_line->transport_type) {
              points.emplace_back(
                jopatili,
                timdemrnt,
                pool.distance_since_start_of_link,
                pool.p_point->location_x_ew,
                pool.p_point->location_y_ns
              );
            }
          }
        }
      }

      std::sort(points.begin(), points.end(), [](Point &a, Point &b) {
        if (a.jopatili->key.timing_link_order != b.jopatili->key.timing_link_order)
          return a.jopatili->key.timing_link_order < b.jopatili->key.timing_link_order;
        return a.distance_since_start_of_link < b.distance_since_start_of_link;
      });

      double total_time_s = 0;
      for (size_t i = 0; i < points.size(); i++) {
        Point *p = &points[i];
        p->total_time_s = total_time_s;
        if (i > 0) {
          Point *prev = &points[i - 1];
          if (p->timdemrnt != prev->timdemrnt) {
            total_time_s += prev->timdemrnt->total_drive_time_s;
            prev->total_time_s = total_time_s;
          }
        }
      }

      fputs("rd_x,rd_y,total_time_s,is_timing_stop\n", out);
      for (const auto &point : points) {
        fprintf(out, "%f,%f,%f,%d\n", point.rd_x, point.rd_y, point.total_time_s, point.jopatili->is_timing_stop);
      }
    }
  }

  if (options.output_file_path != "-"sv) fclose(out);
}
