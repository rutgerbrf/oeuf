// vim:set sw=2 ts=2 sts et:

#include <cstdio>
#include <iostream>
#include <string_view>

#include "joparoute.hpp"

using namespace std::string_view_literals;

void jopaRoute(const Options &options, Kv1Records &records, Kv1Index &index) {
  FILE *out = stdout;
  if (options.output_file_path != "-"sv)
    out = fopen(options.output_file_path, "wb");
  if (!out) {
    fprintf(stderr, "Open %s: %s\n", options.output_file_path, strerrordesc_np(errno));
    exit(EXIT_FAILURE);
  }

  const std::string data_owner_code = "CXX";
  Kv1JourneyPattern::Key jopa_key(
    // Of course it is bad to hardcode this, but we really have no time to make
    // everything nice and dynamic. We're only working with CXX data anyway,
    // and provide no support for the 'Schedules and Passing Times' KV1
    // variant.
    data_owner_code,
    options.line_planning_number,
    options.journey_pattern_code);

  const Kv1JourneyPattern *jopa = index.journey_patterns[jopa_key];
  if (!jopa) {
    std::cerr << "Journey pattern not found" << std::endl;
    return;
  }
  const Kv1Line *line = jopa->p_line;
  
  struct Point {
    bool is_stop = false;
    const Kv1JourneyPatternTimingLink *jopatili = nullptr;
    const Kv1Link *link = nullptr;
    const Kv1Point *point = nullptr;
    double distance_since_start_of_link = 0;
    double distance_since_start_of_journey = 0;
  };
  std::vector<Point> points;

  for (size_t i = 0; i < records.journey_pattern_timing_links.size(); i++) {
    const Kv1JourneyPatternTimingLink *jopatili = &records.journey_pattern_timing_links[i];
    if (jopatili->key.line_planning_number == jopa->key.line_planning_number
     && jopatili->key.journey_pattern_code == jopa->key.journey_pattern_code) {
      const Kv1Link::Key link_key(data_owner_code, jopatili->user_stop_code_begin,
                                  jopatili->user_stop_code_end, line->transport_type);
      const Kv1Link *link = index.links[link_key];
      const Kv1UserStopPoint::Key link_begin_key(data_owner_code, jopatili->user_stop_code_begin);
      const Kv1UserStopPoint::Key link_end_key(data_owner_code, jopatili->user_stop_code_end);
      const Kv1UserStopPoint *link_begin = index.user_stop_points[link_begin_key];
      const Kv1UserStopPoint *link_end   = index.user_stop_points[link_end_key];

      points.emplace_back(true, jopatili, link, link_begin->p_point, 0);

      for (size_t j = 0; j < records.point_on_links.size(); j++) {
        Kv1PointOnLink *pool = &records.point_on_links[j];
        if (pool->key.user_stop_code_begin == jopatili->user_stop_code_begin
         && pool->key.user_stop_code_end   == jopatili->user_stop_code_end
         && pool->key.transport_type       == jopatili->p_line->transport_type) {
          points.emplace_back(false, jopatili, link, pool->p_point, pool->distance_since_start_of_link);
        }
      }

      points.emplace_back(true, jopatili, link, link_end->p_point, link->distance);
    }
  }

  std::sort(points.begin(), points.end(), [](Point &a, Point &b) {
    if (a.jopatili->key.timing_link_order != b.jopatili->key.timing_link_order)
      return a.jopatili->key.timing_link_order < b.jopatili->key.timing_link_order;
    return a.distance_since_start_of_link < b.distance_since_start_of_link;
  });

  double distance_since_start_of_journey = 0;
  for (size_t i = 0; i < points.size(); i++) {
    Point *p = &points[i];
    if (i > 0) {
      Point *prev = &points[i - 1];
      if (p->link != prev->link) {
        distance_since_start_of_journey += prev->link->distance;
      }
    }
    p->distance_since_start_of_journey = distance_since_start_of_journey + p->distance_since_start_of_link;
  }

  fputs("is_stop,link_usrstop_begin,link_usrstop_end,point_code,rd_x,rd_y,distance_since_start_of_link,distance_since_start_of_journey\n", out);
  for (const auto &point : points) {
    fprintf(out, "%s,%s,%s,%s,%f,%f,%f,%f\n",
      point.is_stop ? "true" : "false",
      point.jopatili->user_stop_code_begin.c_str(), point.jopatili->user_stop_code_end.c_str(),
      point.point->key.point_code.c_str(), point.point->location_x_ew, point.point->location_y_ns,
      point.distance_since_start_of_link, point.distance_since_start_of_journey);
  }

  if (options.output_file_path != "-"sv) fclose(out);
}
