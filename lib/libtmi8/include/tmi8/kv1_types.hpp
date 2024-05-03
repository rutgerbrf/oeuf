// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#ifndef OEUF_LIBTMI8_KV1_TYPES_HPP
#define OEUF_LIBTMI8_KV1_TYPES_HPP

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

struct Kv1OrganizationalUnit;
struct Kv1HigherOrganizationalUnit;
struct Kv1UserStopPoint;
struct Kv1UserStopArea;
struct Kv1TimingLink;
struct Kv1Link;
struct Kv1Line;
struct Kv1Destination;
struct Kv1JourneyPattern;
struct Kv1ConcessionFinancerRelation;
struct Kv1ConcessionArea;
struct Kv1Financer;
struct Kv1JourneyPatternTimingLink;
struct Kv1Point;
struct Kv1PointOnLink;
struct Kv1Icon;
struct Kv1Notice;
struct Kv1NoticeAssignment;
struct Kv1TimeDemandGroup;
struct Kv1TimeDemandGroupRunTime;
struct Kv1PeriodGroup;
struct Kv1SpecificDay;
struct Kv1TimetableVersion;
struct Kv1PublicJourney;
struct Kv1PeriodGroupValidity;
struct Kv1ExceptionalOperatingDay;
struct Kv1ScheduleVersion;
struct Kv1PublicJourneyPassingTimes;
struct Kv1OperatingDay;

struct Kv1Records {
  std::vector<Kv1OrganizationalUnit>         organizational_units;
  std::vector<Kv1HigherOrganizationalUnit>   higher_organizational_units;
  std::vector<Kv1UserStopPoint>              user_stop_points;
  std::vector<Kv1UserStopArea>               user_stop_areas;
  std::vector<Kv1TimingLink>                 timing_links;
  std::vector<Kv1Link>                       links;
  std::vector<Kv1Line>                       lines;
  std::vector<Kv1Destination>                destinations;
  std::vector<Kv1JourneyPattern>             journey_patterns;
  std::vector<Kv1ConcessionFinancerRelation> concession_financer_relations;
  std::vector<Kv1ConcessionArea>             concession_areas;
  std::vector<Kv1Financer>                   financers;
  std::vector<Kv1JourneyPatternTimingLink>   journey_pattern_timing_links;
  std::vector<Kv1Point>                      points;
  std::vector<Kv1PointOnLink>                point_on_links;
  std::vector<Kv1Icon>                       icons;
  std::vector<Kv1Notice>                     notices;
  std::vector<Kv1NoticeAssignment>           notice_assignments;
  std::vector<Kv1TimeDemandGroup>            time_demand_groups;
  std::vector<Kv1TimeDemandGroupRunTime>     time_demand_group_run_times;
  std::vector<Kv1PeriodGroup>                period_groups;
  std::vector<Kv1SpecificDay>                specific_days;
  std::vector<Kv1TimetableVersion>           timetable_versions;
  std::vector<Kv1PublicJourney>              public_journeys;
  std::vector<Kv1PeriodGroupValidity>        period_group_validities;
  std::vector<Kv1ExceptionalOperatingDay>    exceptional_operating_days;
  std::vector<Kv1ScheduleVersion>            schedule_versions;
  std::vector<Kv1PublicJourneyPassingTimes>  public_journey_passing_times;
  std::vector<Kv1OperatingDay>               operating_days;

  size_t size() const;
};

// These definitions implement TMI8, KV1 Dienstregeling (Timetable) version
// 8.3.0.2 (release), published by BISON on January 8, 2020.
// (Filename: tmi8 dienstregeling (kv 1) v8.3.0.2, release.docx)
//
// This specification and other BISON specifications, as well as other
// supplementary information, can be found on BISON's website:
//   https://bison.dova.nu/ 
//
// The specification that was used to create these definitions was downloaded
// from the following address:
//   https://bison.dova.nu/sites/default/files/bestanden/tmi8_dienstregeling_kv_1_v8.3.0.2_release.pdf
//
// The KV1 table structure and the corresponding documentation describing the
// relevant tables and fields, as presented here, is derived from the original
// specification. Most documentation is a manually translated version of the
// documentation as present in the specification. The specification is licensed
// under CC BY-ND 3.0. The exact text of this license can be found on
// https://creativecommons.org/licenses/by-nd/3.0/nl/.

// KV1 Table 1: Organizational Unit [ORUN] (MANDATORY)
// 
// A collection of trips with the same validity features. An organizational
// unit can be part of a 'higher' unit.
//
// An organizational unit is defined as a unity vor which the planning of trips
// is compiled. When defining the organizational units, it is important that
// all trips within the package have a homogeneous validity (school holidays,
// shopping Sundays, foreign bank holidays).
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1OrganizationalUnit {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code);
  };
  
  Key key;
  // Mandatory, at most 50 characters.
  std::string name;
  // Mandatory, at most 10 characters.
  std::string organizational_unit_type;
  // Optional, at most 255 characters.
  std::string description;
};

// KV1 Table 2: Higher Organizational Unit [ORUNORUN] (OPTIONAL)
// 
// An in the hierarchy higher-ordered organizational unit for the purpose of
// (among others) recording of (deviating) validities on the high level.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1HigherOrganizationalUnit {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Parent, higher organizational unit
    // that is referred to.
    std::string organizational_unit_code_parent;
    // Mandatory (key), at most 10 characters. Child, lower organizational unit.
    std::string organizational_unit_code_child;
    // Mandatory (key), at most 10 characters. [YYYY-MM-DD] Starting date of the
    // hierarchical relation (can be a fixed value, e.g. 2006-12-31).
    std::chrono::year_month_day valid_from;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code_parent,
                 std::string organizational_unit_code_child,
                 std::chrono::year_month_day valid_from);
  };
 
  Key key;

  Kv1OrganizationalUnit *p_organizational_unit_parent = nullptr;
  Kv1OrganizationalUnit *p_organizational_unit_child  = nullptr;
};

// KV1 Table 3: User Stop Point [USRSTOP]
// 
// Stop or other point (e.g. Bridge, functioning as info for the bridge keeper)
// for which times are recorded in the planning system of the transit operator.
// 
// Coordinates of a UserStopPoint are recorded as Point. When defining
// UserStopPoints, it is important that the coordinates can be unambiguously
// and verifiably recorded. For a stop, the coordinates of the stop sign are
// recorded. If there is no stop sign, the end of the bus stop (where the bus
// normally halts) is recorded as the coordinate of the stop.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1UserStopPoint {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Stop number in domain of operator.
    std::string user_stop_code;

    explicit Key(std::string data_owner_code,
                 std::string user_stop_code);
  };

  Key key;
  // Optional, at most 10 characters. Stop number in domain of integrator,
  // (initially) equal to UserStopCode.
  std::string timing_point_code;
  // Mandatory, at most 5 characters. Boolean indicator whether USRSTOP is used
  // as boarding stop, true by default. False for e.g. dummy stop for bridge
  // keeper.
  bool get_in = true;
  // Mandatory, at most 5 characters. Boolean indicator whether USRSTOP is used
  // as alighting stop.
  bool get_out = false;
  // Mandatory, at most 50 characters. Stop name.
  std::string name;
  // Mandatory, at most 50 characters. Town name.
  std::string town;
  // Optional, at most 10 characters. Reference to StopArea of which the
  // UserStop is part.
  std::string user_stop_area_code;
  // Mandatory, at most 10 characters. Platform indication/letter. The '-'
  // value is used to indication that this is not applicable.
  std::string stop_side_code;
  // Mandatory, at most 5 digits. Minimal stop duration for boarding and
  // alighting, zero by default. In seconds.
  double minimal_stop_time_s = 0;
  // Optional, at most 3 digits. Length of stop platform.
  std::optional<double> stop_side_length;
  // Optional, at most 255 characters.
  std::string description;
  // Mandatory, at most 10 characters. USRSTOPTYPE. Indicates the stop kind.
  std::string user_stop_type;
  // Optional, at most 30 characters. Nationally unique stop number.
  std::string quay_code;

  Kv1UserStopArea *p_user_stop_area = nullptr;
  Kv1Point        *p_point          = nullptr;
};

// KV1 Table 4: User Stop Area [USRSTAR]
//
// A StopArea is a collection of stops, which have the same name for passengers
// and logically belong together. (E.g. a bus station of transfer point.) Stops
// lying opposite each other can also form a StopArea.
//
// Used for display of all stops in a stop area on an overview display and for
// announcement of stop names (stops on both sides of the street share the same
// name).
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1UserStopArea {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Code of StopArea following coding
    // of operator, e.g. PlaceCode.
    std::string user_stop_area_code;

    explicit Key(std::string data_owner_code,
                 std::string user_stop_area_code);
  };

  Key key;
  // Mandatory, at most 50 characters.
  std::string name;
  // Mandatory, at most 50 characters.
  std::string town;
  // Mandatory, at most 255 characters.
  std::string description;
};

// KV1 Table 5: Timing Link [TILI]
// 
// Link between two points which have the feature 'stop' or 'timing point'. A
// Timing Link is set between all stops and other timing points (e.g. for the
// bridge) which make part of a journey pattern.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1TimingLink {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Stop number in the domain of
    // DataOwner (here: the operator).
    std::string user_stop_code_begin;
    // Mandatory (key), at most 10 characters. Stop number in the domain of
    // DataOwner (here: the operator).
    std::string user_stop_code_end;

    explicit Key(std::string data_owner_code,
                 std::string user_stop_code_begin,
                 std::string user_stop_code_end);
  };

  Key key;
  // Optional, at most 5 digits. Minimal trip time (in seconds).
  std::optional<double> minimal_drive_time_s;
  // Optional, at most 255 characters.
  std::string description;

  Kv1UserStopPoint *p_user_stop_begin = nullptr;
  Kv1UserStopPoint *p_user_stop_end   = nullptr;
};

// KV1 Table 6: Link [LINK]
//
// A route link describes the connection between to points on the physical path
// of a route.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Link {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Stop code in the domain of
    // DataOwner (here: the operator).
    std::string user_stop_code_begin;
    // Mandatory (key), at most 10 characters. Stop code in the domain of
    // DataOwner (here: the operator).
    std::string user_stop_code_end;
    // Mandatory (key), at most 5 characters. Modality for which the distance
    // applies, see BISON enumeration E9.
    // TODO: Check if BISON enumeration E9 can be put into an enum.
    std::string transport_type;

    explicit Key(std::string data_owner_code,
                 std::string user_stop_code_begin,
                 std::string user_stop_code_end,
                 std::string transport_type);
  };

  Key key;
  // Mandatory, at most 6 digits. Length of the link (in meters).
  double distance = 0;
  // Optional, at most 255 characters.
  std::string description;

  Kv1UserStopPoint *p_user_stop_begin = nullptr;
  Kv1UserStopPoint *p_user_stop_end   = nullptr;
};

struct RgbColor {
  uint8_t r, g, b = 0;
};

// KV1 Table 7: Line [LINE]
//
// A line is a collection of routes/journey patterns which is publically known
// under a shared number.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Line {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Unique system line number in the
    // domain of DataOwner.
    std::string line_planning_number;

    explicit Key(std::string data_owner_code,
                 std::string line_planning_number);
  };

  Key key;
  // Mandatory, at most 4 characters. Line number for the public, incl. S/N
  // indications.
  std::string line_public_number;
  // Mandatory, at most 50 characters.
  std::string line_name;
  // Mandatory, at most three digits. Should be in the range [0, 400).
  // Only processing Connexxion's KV1 export, however, shows us that this range
  // constrained is not honored in practice. That is why we also don't care.
  short line_ve_tag_number = 0;
  // Optional, at most 255 characters.
  std::string description;
  // Mandatory, at most 5 characters. Modality, see BISON enumeration E9.
  // TODO: Check if BISON enumeration E9 can be put into an enum.
  std::string transport_type;
  // Optional, at most 4 digits. Symbol / image for the line. Reference to ICON
  // table.
  std::optional<short> line_icon;
  // Optional, at most four characters. Background color for the line.
  // Hexadecimal representation following RGB coding. Always six characters
  // (RRGGBB), only numbers and/or capital letters.
  std::optional<RgbColor> line_color;
  // Optional, at most four characters. Foreground color for the line.
  // Hexadecimal representation following RGB coding. Always six characters
  // (RRGGBB), only numbers and/or capital letters.
  std::optional<RgbColor> line_text_color;

  Kv1Icon *p_line_icon = nullptr;
};

// KV1 Table 8: Destination [DEST]
//
// A destination shows the place/district/description of the route for the
// passenger. Intermediate and detail destinations of a journey pattern are
// shown under a single desination code, together with the primary destination.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Destination {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string dest_code;

    explicit Key(std::string data_owner_code,
                 std::string dest_code);
  };

  Key key;
  // Mandatory, at most 50 characters. Full destination (e.g. compiled from
  // primary, detail or intermediate destination).
  std::string dest_name_full;
  // Mandatory, at most 24 characters. Primary / intermediate destination in
  // enumeration / final destination if 1 line is used.
  std::string dest_name_main;
  // Optional, at most 24 characters. Detail/secondary or intermediate
  // destination for primary desination, final destination (for intermediate
  // destination on line 1).
  std::string dest_name_detail;
  // Mandatory, at most 5 characters. Boolean which indcates whether
  // DestNameDetail must always be shown (e.g. because this contains an
  // important intermediate destination.)
  bool relevant_dest_name_detail = false;
  // Mandatory, at most 21 characters. Primary destination in 21 characters.
  std::string dest_name_main_21;
  // Optional, at most 21 characters. Detail/secondary/intermediate destination
  // in 21 characters.
  std::string dest_name_detail_21;
  // Mandatory, at most 19 characters. Primary destination in 19 characters.
  std::string dest_name_main_19;
  // Optional, at most 19 characters. Detail/secondary/intermediate destination
  // in 19 characters.
  std::string dest_name_detail_19;
  // Mandatory, at most 16 characters. Primary destination in 16 characters.
  std::string dest_name_main_16;
  // Optional, at most 16 characters. Detail/secondary/intermediate destination
  // in 16 characters.
  std::string dest_name_detail_16;
  // Optional, at most 4 digits. Symbol/image for the destination. Reference to
  // the ICON table.
  std::optional<short> dest_icon;
  // Optional, at most 6 characters. Background color for the destination.
  // Hexadecimal representation following RGB coding. Always six characters
  // (RRGGBB), only six digits and/or capital letters.
  std::optional<RgbColor> dest_color;
  // Optional, at most 30 characters (WTF?). Foreground color for the
  // destination. Hexadecimal representation following RGB coding. Always six
  // characters (RRGGBB), only six digits and/or capital letters.
  std::optional<RgbColor> dest_text_color;
};

// KV1 Table 9: Journey Pattern [JOPA]
//
// The journey pattern describes the route from start to end point as a ordered
// list of stops and links between stops/timing points.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1JourneyPattern {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 10 characters.
    std::string journey_pattern_code;

    explicit Key(std::string data_owner_code,
                 std::string line_planning_number,
                 std::string journey_pattern_code);
  };

  Key key;
  // Mandatory, at most 10 characters. Refers to a journey pattern type
  // (JOPATYPE).
  std::string journey_pattern_type;
  // Mandatory, at most 1 character. One of [1, 2, A, B].
  char direction = 0;
  // Optional, at most 255 characters.
  std::string description;

  Kv1Line *p_line = nullptr;
};

// KV1 Table 10: Concession Financer Relation [CONFINREL]
//
// Concession financer relation (mainly parcel). Smallest unit for which data
// about a concession can be captured in relation to a financer and/or
// concession.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1ConcessionFinancerRelation {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Parcel code.
    std::string con_fin_rel_code;

    explicit Key(std::string data_owner_code,
                 std::string con_fin_rel_code);
  };

  Key key;
  // Mandatory, at most 10 characters. Concession code.
  std::string concession_area_code;
  // Optional, at most 10 characters. Code of financer/client of the parcel.
  std::string financer_code;

  Kv1ConcessionArea *p_concession_area = nullptr;
  Kv1Financer       *p_financer        = nullptr;
};

// KV1 Table 11: Concession Area [CONAREA]
//
// Concession (area).
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1ConcessionArea {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Code of the concession.
    std::string concession_area_code;

    explicit Key(std::string data_owner_code,
                 std::string concession_area_code);
  };

  Key key;
  // Mandatory, at most 255 characters.
  std::string description;
};

// KV1 Table 12: Financer [FINANCER] (OPTIONAL)
//
// Financer of a parcel.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Financer {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string financer_code;

    explicit Key(std::string data_owner_code,
                 std::string financer_code);
  };

  Key key;
  // Mandatory, at most 255 characters.
  std::string description;
};

// KV1 Table 13: Journey Pattern Timing Link [JOPATILI]
//
// Compilation of journey pattern from logical links (between pairs of
// stops/timing points). Features such as the destination code, the public line
// number, the concession financer relation (parcel) and product formula are
// set per connection. Moreover, a color and/or image linked to the line
// destination and the use of the (first) stop as boarding/alighting stop can
// be set per link.
//
// Timing Link: A timing link is a stop, set by the transit operator, where a
// bus / public transit vehicle may never depart earlier than set in the
// timetable.
//
// A logical link may never occur more than once in a journey pattern.
// Therefore, the combination of LinePlanningNumber, JourneyPatternCode,
// UserStopCodeBegin and UserStopCodeEnd must be unique in JOPATILI.
//
// The value of GetIn and GetOut are normally copied from the corresponding
// stop in the USRSTOP table, but can be overruled per journey pattern if so
// desired.
//
// A Icon or (Text)Color set here overrules the general value of the
// corresponding line (Line) or destination (Destination).
//
// A value of ShowFlexibleTrip or ProductFormulaType in PUJO or PUJOPASS
// overrules the value in JOPATILI.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1JourneyPatternTimingLink {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 10 characters.
    std::string journey_pattern_code;
    // Mandatory (key), at most 3 digits.
    short timing_link_order = 0;

    explicit Key(std::string data_owner_code,
                 std::string line_planning_number,
                 std::string journey_pattern_code,
                 short timing_link_order);
  };

  Key key;
  // Mandatory, at most 10 characters. Stop number in the domain of the
  // DataOwner (here: the transit operator).
  std::string user_stop_code_begin;
  // Mandatory, at most 10 characters. Stop number in the domain of the
  // DataOwner (here: the transit operator).
  std::string user_stop_code_end;
  // Mandatory, at most 10 characters. Concession financer relation / parcel
  // (smallest unit).
  std::string con_fin_rel_code;
  // Mandatory, at most 10 characters. The destination (incl. intermediat
  // destinations) as these are shown at the first stop of the journey pattern
  // link.
  std::string dest_code;
  // Mandatory, at most 5 characters. Boolean which indicates whether the first
  // stop of the connection is a timing stop. Indicator is at least "true" at
  // first stop of a line and at waiting stops.
  bool is_timing_stop = false;
  // Optional, at most 4 characters. Public line number which must be shown on
  // displays from the first stop of the journey pattern link (e.g. Line number
  // + S). This is important when a deviating public line number applies from a
  // certain point on forward. Normally, the public line number of the
  // corresponding line is shown.
  std::string display_public_line;
  // Optional, at most 4 digits. Enumeration E10 (see section 2.5). A public
  // transit service which distinguishes itself by a set of unique features,
  // that is offered to the passenger as distinct (a marketing aspect).
  // TODO: Check if we can turn BISON enumeration E10 into an enum
  std::optional<short> product_formula_type;
  // Mandatory, at most 5 characters. Boolean indicator whether UserStopBegin
  // is used as a boarding stop in this journey pattern. Usually equal to the
  // value of the corresponding USRSTOP.
  bool get_in = false;
  // Mandatory, at most 5 characters. Boolean indicator whether UserStopBegin
  // is used as an alighting stop in this journey pattern. Usually equal to the
  // value of the corresponding USRSTOP.
  bool get_out = false;
  // Optional, at most 8 characters. Indicates whether the transit operator
  // wants a not explicitly planned trip (i.e. a trip that only operates after
  // reservation such as a 'call bus' (belbus), 'line taxi' (lijntaxi) etc.) to
  // be shown on displays. Values according enumeration E21: TRUE (always),
  // FALSE (never), REALTIME (only when tracking trip).
  // TODO: Check if we can turn BISON enumeration E21 into an enum
  std::string show_flexible_trip;
  // Optional, at most 4 digits. Symbol / image for display of the line
  // destination at the journey stop passing. Reference to the ICON table.
  std::optional<short> line_dest_icon;
  // Optional, at most 6 characters. Background color for display of the line
  // destination at a journey stop passing. Hexadecimal representation
  // following RGB coding. Always six characters (RRGGBB), only numbers and/or
  // capital letters.
  std::optional<RgbColor> line_dest_color;
  // Optional, at most 6 characters. Foreground color for display of the line
  // destination at a journey stop passing. Hexadecimal representation
  // following RGB coding. Always six characters (RRGGBB), only numbers and/or
  // capital letters.
  std::optional<RgbColor> line_dest_text_color;

  Kv1Line                       *p_line            = nullptr;
  Kv1JourneyPattern             *p_journey_pattern = nullptr;
  Kv1UserStopPoint              *p_user_stop_begin = nullptr;
  Kv1UserStopPoint              *p_user_stop_end   = nullptr;
  Kv1ConcessionFinancerRelation *p_con_fin_rel     = nullptr;
  Kv1Destination                *p_dest            = nullptr;
  Kv1Icon                       *p_line_dest_icon  = nullptr;
};

// KV1 Table 14: Point [POINT]
//
// A point is the smallest location which can be reffered to within the public
// transit network. Every stop (USRSTOP) is a point.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Point {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string point_code;

    explicit Key(std::string data_owner_code,
                 std::string point_code);
  };

  Key key;
  // Mandatory, at most 10 characters. Refers to the POINTTYPE table.
  std::string point_type;
  // Mandatory, at most 10 characters. Refers to the GEOSYSTYPE table. Only
  // allowed to have the value "RD" (rijkdsdriehoekstelsel; the national Dutch
  // coordinate system).
  std::string coordinate_system_type;
  // Mandatory, at most 15 characters. X position in the RD coordinate system,
  // in meters (at least 6 digits).
  double location_x_ew = 0;
  // Mandatory, at most 15 characters. Y position in the RD coordinate system,
  // in meters (at least 6 digits).
  double location_y_ns = 0;
  // Optional, at most 15 characters.
  // NOTE: the standart (presumeably wrongly) indicates this field as having
  // alphanumeric contents.
  std::optional<double> location_z;
  // Optional, at most 255 characters.
  std::string description;
};

// KV1 Table 15: Point on Link [POOL]
//
// A point that is used to geographically describe the trajectory between two
// stops.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1PointOnLink {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Stop number in the domain of the
    // DataOwner (here: transit operator).
    std::string user_stop_code_begin;
    // Mandatory (key), at most 10 characters. Stop number in the domain of the
    // DataOwner (here: transit operator).
    std::string user_stop_code_end;
    // Mandatory (key), at most 10 characters. Code from the road manager for KAR
    // points. For curve points of the DataOwner (often the transit operator).
    std::string point_data_owner_code;
    // Mandatory (key), at most 10 charcters.
    std::string point_code;
    // Mandatory (key), at most 5 characters. Modality for which the distance
    // applies, see BISON enumeration E9.
    std::string transport_type;

    explicit Key(std::string data_owner_code,
                 std::string user_stop_code_begin,
                 std::string user_stop_code_end,
                 std::string point_data_owner_code,
                 std::string point_code,
                 std::string transport_type);
  };

  Key key;
  // Mandatory, at most 5 digits. Distance in meters relative to the start of
  // the link.
  double distance_since_start_of_link = 0;
  // Optional, at most 4 digits. Crossing speed for a public transit vehicle
  // from the previous point (on a link) in m/s.
  std::optional<double> segment_speed_mps = 0;
  // Optional, at most 4 digits. Comfort speed for a public transit vehicle on
  // the curve point.
  std::optional<double> local_point_speed_mps = 0;
  // Optional, at most 255 characters.
  std::string description;

  Kv1UserStopPoint *p_user_stop_begin = nullptr;
  Kv1UserStopPoint *p_user_stop_end   = nullptr;
  Kv1Point         *p_point           = nullptr;
};

// KV1 Table 16: Icon [ICON]
//
// Table with images which can be referred to from DEST.DestIcon, LINE.LineIcon
// and JOPATILI.LineDestIcon to load the correct image.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Icon {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 4 digits. Reference from other tables for the
    // requested image.
    short icon_number = 0;

    explicit Key(std::string data_owner_code,
                 short icon_number);
  };

  Key key;
  // Mandatory, at most 1024 characters. Absolute URI to a publically available
  // location from which the image can be loaded. The extension of the file
  // indicates the image type.
  // Supported file types are: GIF (.gif), JPEG (.jpg, .jpeg),
  //                           PNG (.png), SVG (.svg)
  // Supported protocols are: HTTP, HTTPS, FTP
  // Prefer to not use any capital letters. Examples:
  //   - http://bison.dova.nu/images/logo.png
  //   - https://bison.dova.nu/images/logo.png
  //   - ftp://ftp.dova.nu/images/logo.png
  std::string icon_uri;
};

// KV1 Table 17: Notice [NOTICE] (OPTIONAL)
//
// A (reusable) text with supplementary information about exceptions /
// clarifications for a line, journey pattern etc.
// 
// Usage is optional; when there are no clarifying texts, the NOTICE table does
// not need to be provided in a KV1 set.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1Notice {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 20 characters. Identification of Notice (remark,
    // clarifying text).
    std::string notice_code;

    explicit Key(std::string data_owner_code,
                 std::string notice_code);
  };

  Key key;
  // Mandatory, at most 1024 characters. Content, text. Contains contact
  // information such as telephone number, web address and reservation time for
  // 'call buses' (belbussen) and other demand-based transit.
  std::string notice_content;
};

// KV1 Table 18: Notice Assignment [NTCASSGNM] (OPTIONAL)
//
// Linking table in which Notice (remark, clarfiying text) is assigned to a
// line, journey pattern, stops within a journey pattern, journey etc. Notice
// Assignment contains all logical key elements of the corresponding objects to
// which a Notice can be assigned.
//
// Different attributes are required for the Notice Assignment, depending on
// the type object to which the Notice is assigned. In the following table
// structure, this is indicated as 'Only relevant for ...'. This means that
// fields for other object types in the Notice Assignment can be ignored.
//
// Moreover, it can also occur that not all key fields of the linked table are
// of interest (content-wise) for recording the Notice.
//
// Both matters are summarised in this overview:
//
//   --------------------------------------------------------
//   AssignedObject          PUJO   PUJOPASS   LINE  JOPATILI
//   --------------------------------------------------------
//   DataOwnerCode........... x ...... x ...... x ..... x ...
//   TimetableVersionCode ... o .............................
//   OrganizationalUnitCode . o ...... o ....................
//   ScheduleCode .................... o ....................
//   ScheduleTypeCode ................ o ....................
//   PeriodGroupCode ........ o .............................
//   SpecificDayCode ........ o .............................
//   DayType ................ o .............................
//   LinePlanningNumber ..... x ...... x ...... x ..... x ...
//   JourneyNumber .......... x ...... x ....................
//   StopOrder ....................... o .............. o ...
//   JourneyPatternCode ............................... x ...
//   TimingLinkOrder .................................. o ...
//   UserStopCode .................... o .............. o ...
//   --------------------------------------------------------
//
//   Legend:
//     x - Mandatory. The Notice for this object type is always depndent on the
//         value of the attribute.
//     o - Optional. The Notice can be independent of the value of this
//         attribute for this object type.
//     <empty> - Attribute is no key field for this object type and can be
//         ignored when processed.
//
// Usage of Notice Assignment is optional in KV1. If there are no clarifying
// texts, then the Notice Assignment table is not required to be present in the
// provided KV1 set.
//
// This table is part of the core data tables, which are common for all KV1
// variants.
struct Kv1NoticeAssignment {
  // Mandatory, at most 10 characters. Transport operator (from list as
  // defined in BISON enumeration E1).
  std::string data_owner_code;
  // Mandatory, at most 20 characters. Notice that is assigned.
  std::string notice_code;
  // Mandatory, at most 8 characters. Object type to which Notice is assigned.
  std::string assigned_object;
  // Optional, at most 10 characters. Only relevant for PUJO.
  std::string timetable_version_code;
  // Optional, at most 10 characters. Only relevant for PUJO and PUJOPASS.
  std::string organizational_unit_code;
  // Optional, at most 10 characters. Only relevant for PUJOPASS.
  std::string schedule_code;
  // Optional, at most 10 characters. Only relevant for PUJOPASS.
  std::string schedule_type_code;
  // Optional, at most 10 characters. Only relevant for PUJO.
  std::string period_group_code;
  // Optional, at most 10 characters. Only relevant for PUJO.
  std::string specific_day_code;
  // Optional, at most 10 characters. Only relevant for PUJO.
  // [0|1][0|2][0|3][0|4][0|5][0|6][0|7] for Mon, Tue, Wed, Thu, Fri, Sat, Sun.
  // E.g. 1234500 means Mon, Tue, Wed, Thu, Fri but not Sat, Sun.
  std::string day_type;
  // Mandatory, at most 10 characters. Mandatory for all object types.
  std::string line_planning_number;
  // Optional (for all object types except PUJO and PUJOPASS), at most 6
  // digits. Only relevant for PUJO and PUJOPASS. Must be in the range
  // [0-1000000).
  std::optional<int> journey_number;
  // Optional, at most 4 digits. Only relevant for PUJOPASS and JOPATILI.
  std::optional<int> stop_order;
  // Optional (for all object types except JOPATILI), at most 4 digits. Only
  // relevant for JOPATILI.
  std::string journey_pattern_code;
  // Optional (at most 3 digits). Only relevant for JOPATILI.
  std::optional<short> timing_link_order;
  // Optional (at most 10 characters). Only relevant for PUJOPASS and JOPATILI.
  // For JOPATILI, this correspond to the first stop of the link.
  std::string user_stop_code;

  Kv1Notice *p_notice = nullptr;
};

// KV1 Table 19: Time Demand Group [TIMDEMGRP]
//
// A time demand group is a grouping of the run time distribution from stop to
// stop, for a journey pattern (from start to end point).
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1TimeDemandGroup {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 10 characters. Refers to the JOPATILI table.
    std::string journey_pattern_code;
    // Mandatory (key), at most 10 characters. Defines the code for the time
    // demand group. (NOTE: this is not entirely made clear by the specification.
    // This claim must be verified.)
    std::string time_demand_group_code;

    explicit Key(std::string data_owner_code,
                 std::string line_planning_number,
                 std::string journey_pattern_code,
                 std::string time_demand_group_code);
  };

  Key key;

  Kv1Line           *p_line            = nullptr;
  Kv1JourneyPattern *p_journey_pattern = nullptr;
};

// KV1 Table 20: Time Demand Group Run Time [TIMDEMRNT]
//
// The run time structure/distribution for all timing links of a journey
// pattern or a time demand group.
//
// Optional run time elements are, when these are present, used to more
// accurately calculate expected departure times based on punctuality
// deviations.
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1TimeDemandGroupRunTime {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 10 characters. Refers to the JOPATILI table.
    std::string journey_pattern_code;
    // Mandatory (key), at most 10 characters. Refers to the TIMDEMGRP table.
    std::string time_demand_group_code;
    // Mandatory (key), at most 3 digits. Reference number of a link within the
    // journey pattern (a link can occur more than once within a journey
    // pattern).
    short timing_link_order = 0;

    explicit Key(std::string data_owner_code,
                 std::string line_planning_number,
                 std::string journey_pattern_code,
                 std::string time_demand_group_code,
                 short timing_link_order);
  };

  Key key;
  // Mandatory, at most 10 characters. Refers to the first stop of the link.
  std::string user_stop_code_begin;
  // Mandatory, at most 10 characters. Refers to the last stop of the link.
  std::string user_stop_code_end;
  // Mandatory, at most 5 digits. Planned total run time on link for time
  // demand group: (Departure time end stop - departure time begin stop)
  // corresponding to the time demand group. In seconds.
  double total_drive_time_s = 0;
  // Mandatory, at most 5 digits. Planned minimal run time on link for time
  // demand group. Often calculated as: (Arrival time end stop - arrival time
  // begin stop) corresponding to the time demand group. In seconds.
  double drive_time_s = 0;
  // Optional, at most 5 digits. Expected/planned delay/congestion on link for
  // time demand group. In seconds.
  std::optional<double> expected_delay_s;
  // Optional, at most 5 digits. Layover/catch-up time. Gives play in the
  // timetable. In seconds.
  //   LayOverTime = TotDriveTime - DriveTime + ExpectedDelay - StopWaitTime. 
  std::optional<double> layover_time;
  // Mandatory, at most 5 digits. Planned stop waiting time at the final stop
  // of the link for the time demand group. Determined based on the difference
  // between the departure time and arrival time at this stop. Is zero when no
  // waiting time is planned for this stop. In seconds.
  double stop_wait_time = 0;
  // Optional, at most 5 digits. Planned minimal stop time for
  // boarding/alighting of passengers at the final stop of the link for the
  // time demand group. Application: at hub stops with a planned waiting time,
  // the difference between the planned waiting time and the minimum stop time
  // is the layover/catch-up time. In seconds.
  std::optional<double> minimum_stop_time;

  Kv1Line                     *p_line                        = nullptr;
  Kv1UserStopPoint            *p_user_stop_begin             = nullptr;
  Kv1UserStopPoint            *p_user_stop_end               = nullptr;
  Kv1JourneyPattern           *p_journey_pattern             = nullptr;
  Kv1TimeDemandGroup          *p_time_demand_group           = nullptr;
  Kv1JourneyPatternTimingLink *p_journey_pattern_timing_link = nullptr;
};

// KV1 Table 21: Period Group [PEGR]
//
// Period group is an indication of a 'homogeneous period' during the year,
// i.e. a period in which the schedule has the same composition w.r.t.
// frequencies and run times.
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1PeriodGroup {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string period_group_code;

    explicit Key(std::string data_owner_code,
                 std::string period_group_code);
  };

  Key key;
  // Optional, at most 255 characters.
  std::string description;
};

// KV1 Table 22: Specific Day [SPECDAY]
//
// A specific day is a feature of a day for which a deviating service level is
// provided, respective to a normal day of the week.
//
// E.g. shopping Sundays (koopzondagen, if not every Sunday), New Year's Eve
// (oudejaarsdag), foreign bank holidays (as applicable).
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1SpecificDay {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Default: "NORMAL".
    std::string specific_day_code;

    explicit Key(std::string data_owner_code,
                 std::string specific_day_code);
  };

  Key key;
  // Mandatory, at most 50 characters.
  std::string name;
  // Optional, at most 255 characters.
  std::string description;
};

// KV1 Table 23: Timetable Version [TIVE]
//
// A timetable version budles all planned activities for an organizational
// unit. For the public schedule, these are trips, routes, run times etc.
//
// When processing a new Timetable Version, it is checked if another TIVE with
// the same key has already been processed. If this is the case, ValidFrom must
// be equal to the starting date of the previously provided set. The new set
// replaces the older one. A package with a new starting date is only processed
// if another TimetableVersionCode is used.
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1TimetableVersion {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters.
    std::string timetable_version_code;
    // Mandatory (key), at most 10 charactes.
    std::string period_group_code;
    // Mandatory (key), at most 10 characters. Default: "NORMAL".
    std::string specific_day_code;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::string timetable_version_code,
                 std::string period_group_code,
                 std::string specific_day_code);
  };

  Key key;
  // Mandatory, at most 10 characters. Datum on which the timetable goes into
  // effect, following the YYYY-MM-DD format.
  std::chrono::year_month_day valid_from;
  // Mandatory, at most 10 characters. Value: "PUBT".
  std::string timetable_version_type;
  // Optional, at most 10 characters. Datum on which the timetable goes out of
  // effect, following the YYYY-MM-DD format.
  std::optional<std::chrono::year_month_day> valid_thru;
  // Optional, at most 255 characters. Should be null/empty.
  std::string description;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1PeriodGroup        *p_period_group        = nullptr;
  Kv1SpecificDay        *p_specific_day        = nullptr;
};

// KV1 Table 24: Public Journey [PUJO]
//
// Public journeys are journeys that are operated by a public transit
// organization and are accessible to the passenger.
//
// Business rules:
//   - If ShowFlexibleTrip or ProductFormulaType is set in a record of this
//     table, this takes precedence over the value as in the corresponding
//     JOPATILI entry.
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1PublicJourney {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string timetable_version_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters.
    std::string period_group_code;
    // Mandatory (key), at most 10 characters.
    std::string specific_day_code;
    // Mandatory (key), at most 7 characters.
    // [0|1][0|2][0|3][0|4][0|5][0|6][0|7] for Mon, Tue, Wed, Thu, Fri, Sat, Sun.
    // E.g. 1234500 means Mon, Tue, Wed, Thu, Fri but not Sat, Sun.
    // TODO: See if we can make this into a more concrete type
    std::string day_type;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 6 digits. Must be in the range [0-1000000).
    int journey_number = 0;

    explicit Key(std::string data_owner_code,
                 std::string timetable_version_code,
                 std::string organizational_unit_code,
                 std::string period_group_code,
                 std::string specific_day_code,
                 std::string day_type,
                 std::string line_planning_number,
                 int journey_number);
  };

  Key key;
  // Mandatory, at most 10 characters.
  std::string time_demand_group_code;
  // Mandatory, at most 10 characters.
  std::string journey_pattern_code;
  // Mandatory, at most 8 characters. Format: "HH:MM:SS".
  std::chrono::hh_mm_ss<std::chrono::seconds> departure_time;
  // Mandatory, at most 13 characters. Values as in BISON enumeration E3.
  // Allowed are: "ACCESSIBLE", "NOTACCESSIBLE" and "UNKNOWN".
  // TODO: See if we can fit BISON enumeration E3 into an enum
  std::string wheelchair_accessible;
  // Mandatory, at most 5 characters. Boolean. Value "true": journey is
  // operator by DataOwner. Value "false": journey is operator by a different
  // DataOwner. Indicator is meant for a line that is operated jointly by
  // multiple transit operators. The indicator is used to be able to match the
  // journey operation (KV6, KV19 etc.); only journeys for which the indicator
  // is "true" can be expected to have corresponding current/real-time
  // information, although "true" doesn't necessarily mean that this
  // current/real-time information will (always) become available.
  bool data_owner_is_operator = false;
  // Mandatory, at most 5 characters. Boolean. Indicates whether
  // current/real-time journey information may be expected for the
  // corresponding journey ("true" or "false").
  bool planned_monitored = false;
  // Optional, at most 4 digits. BISON enumeration E10. Intended to allow
  // capturing transit mode features at the journey level.
  // TODO: See if we can make BISON enumeration E10 into an enum
  std::optional<short> product_formula_type;
  // Optional, at most 8 characters. Indicates whether the transit operator
  // wants that a not-explicitly planned trip (i.e. a journey that only runs on
  // reservation, e.g. 'call bus' (belbus), 'line taxi' (lijntaxi) etc.) to be
  // shown on displays. Values following BISON enumeration E21: TRUE (always),
  // FALSE (never), REALTIME (only when journey is tracked).
  // TODO: See if we can make BISON enumeration E21 into an enum
  std::string show_flexible_trip;

  Kv1TimetableVersion   *p_timetable_version   = nullptr;
  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1PeriodGroup        *p_period_group        = nullptr;
  Kv1SpecificDay        *p_specific_day        = nullptr;
  Kv1Line               *p_line                = nullptr;
  Kv1TimeDemandGroup    *p_time_demand_group   = nullptr;
  Kv1JourneyPattern     *p_journey_pattern     = nullptr;
};

// KV1 Table 25: Period Group Validity [PEGRVAL]
//
// Validities (multiple from-thru data) of a period group.
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1PeriodGroupValidity {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters.
    std::string period_group_code;
    // Mandatory (key), at most 10 characters. Date of the start of the validity
    // period. Format: "YYYY-MM-DD".
    std::chrono::year_month_day valid_from;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::string period_group_code,
                 std::chrono::year_month_day valid_from);
  };

  Key key;
  // Mandatory, at most 10 characters. Date of the end of the validity period.
  // Format: "YYYY-MM-DD".
  std::chrono::year_month_day valid_thru;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1PeriodGroup        *p_period_group        = nullptr;
};

// KV1 Table 26: Exceptional Operating Day [EXCOPDAY]
//
// Contains exceptional validity dates, for which the service runs following a
// different day type (such as another day of the week or a different period).
//
// This table is part of the KV1 variant "validities and time demand groups".
struct Kv1ExceptionalOperatingDay {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters. Organization unit for which an
    // exceptional day validity applies.
    std::string organizational_unit_code;
    // Mandatory (key), at most 23 characters. Date (+ time) for which the
    // exceptional validity applies. Format: "YYYYMMDDThh:mm:ssTZD".
    std::chrono::sys_seconds valid_date;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::chrono::sys_seconds valid_date);
  };

  Key key;
  // Mandatory, at most 7 characters. The exceptional day type that applies on
  // a calendar day: [0|1][0|2][0|3][0|4][0|5][0|6][0|7] for Mon, Tue, Wed,
  // Thu, Fri, Sat.
  // E.g. 1234500 means Mon, Tue, Wed, Thu, Fri but not Sat, Sun.
  // TODO: See if we can make this into a more concrete type
  std::string day_type_as_on;
  // Mandatory, at most 10 characters. Specific day service level to which the
  // exceptional day validity refers.
  std::string specific_day_code;
  // Optional, at most 10 characters. An exceptional day validity can be
  // related to the service level of another period (e.g. the school holiday
  // schedule). This exceptional period reference is set here.
  // 
  // E.g. on Good Friday or the day after Ascension day, transit runs according
  // to the holiday season schedule, while transit runs following the winter
  // package in the surrounding days.
  std::string period_group_code;
  // Optional, at most 255 characters.
  std::string description;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1SpecificDay        *p_specific_day        = nullptr;
  Kv1PeriodGroup        *p_period_group        = nullptr;
};

// KV1 Table 27: Schedule Version [SCHEDVERS]
//
// A schedule version bundles the planned activities for an organisation unit
// per day type. The journeys with passing times and corresponding routes are
// for the public timetable.
//
// When processing a new Schedule Version, it is checked if another SCHEDVERS
// with the same key has already been processed. If this is the case, ValidFrom
// must be equal to the starting date of the previously provided set. The new
// set replaces the older one. A package with a new starting date is only
// processed if another Schedule Code is used.
//
// This table is part of the KV1 variant "schedules and passing times".
struct Kv1ScheduleVersion {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters. A unique code in combination with
    // the ScheduleTypeCode of the package within the ORUN.
    std::string schedule_code;
    // Mandatory (key), at most 10 characters. Code for the Schedule Type (Day Type).
    std::string schedule_type_code;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::string schedule_code,
                 std::string schedule_type_code);
  };

  Key key;
  // Mandatory, at most 10 characters. Date on which the schedule goes into
  // effect. Format: "YYYY-MM-DD".
  std::chrono::year_month_day valid_from;
  // Optional, at most 10 characters. Date on which the schedule goes out of
  // effect. Format: "YYYY-MM-DD".
  std::optional<std::chrono::year_month_day> valid_thru;
  // Optional, at most 255 characters. Should be empty/null.
  std::string description;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
};

// KV1 Table 28: Public Journey Passing Times [PUJOPASS]
//
// Public journey with arrival and departure times at all stops (and other
// timing points).
//
// Business rules:
//   - If ShowFlexibleTrip or ProductFormulaType is set here, then this takes
//     precedence over the value in the corresponding JOPATILI record.
//   - All stop passings of a public journey refer to the same journey pattern
//     (JOPA)!
//
// This table is part of the KV1 variant "schedules and passing times".
struct Kv1PublicJourneyPassingTimes {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters. A unique code in combination with
    // the ScheduleTypeCode of the package within the ORUN.
    std::string schedule_code;
    // Mandatory (key), at most 10 characters. Code for the Schedule Type (e.g.
    // Day Type).
    std::string schedule_type_code;
    // Mandatory (key), at most 10 characters.
    std::string line_planning_number;
    // Mandatory (key), at most 6 digits. Must be in the range [0-1000000).
    int journey_number = 0;
    // Mandatory (key), at most 4 digits.
    short stop_order = 0;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::string schedule_code,
                 std::string schedule_type_code,
                 std::string line_planning_number,
                 int journey_number,
                 short stop_order);
  };

  Key key;
  // Mandatory, at most 10 characters.
  std::string journey_pattern_code;
  // Mandatory, at most 10 characters.
  std::string user_stop_code;
  // Mandatory (except for the first stop of a journey), at most 8 digits. Not
  // compulsory for the first stop of a journey. Format: "HH:MM:SS".
  std::optional<std::chrono::hh_mm_ss<std::chrono::seconds>> target_arrival_time;
  // Mandatory (expect for the last stop of a journey), at most 8 digits. Not
  // compulsory for the last stop of a journey. Format: "HH:MM:SS".
  std::optional<std::chrono::hh_mm_ss<std::chrono::seconds>> target_departure_time;
  // Mandatory, at most 13 characters. Values as in BISON enumeration E3.
  // Allowed are: "ACCESSIBLE", "NOTACCESSIBLE" and "UNKNOWN".
  // TODO: See if we can fit BISON enumeration E3 into an enum
  std::string wheelchair_accessible;
  // Mandatory, at most 5 characters. Boolean. Value "true": journey is
  // operator by DataOwner. Value "false": journey is operator by a different
  // DataOwner. Indicator is meant for a line that is operated jointly by
  // multiple transit operators. The indicator is used to be able to match the
  // journey operation (KV6, KV19 etc.); only journeys for which the indicator
  // is "true" can be expected to have corresponding current/real-time
  // information, although "true" doesn't necessarily mean that this
  // current/real-time information will (always) become available.
  bool data_owner_is_operator = false;
  // Mandatory, at most 5 characters. Boolean. Indicates whether
  // current/real-time journey information may be expected for the
  // corresponding journey ("true" or "false").
  bool planned_monitored = false;
  // Optional, at most 4 digits. BISON enumeration E10. Intended to allow
  // capturing transit mode features at the journey level.
  // TODO: See if we can make BISON enumeration E10 into an enum
  std::optional<short> product_formula_type;
  // Optional, at most 8 characters. Indicates whether the transit operator
  // wants that a not-explicitly planned trip (i.e. a journey that only runs on
  // reservation, e.g. 'call bus' (belbus), 'line taxi' (lijntaxi) etc.) to be
  // shown on displays. Values following BISON enumeration E21: TRUE (always),
  // FALSE (never), REALTIME (only when journey is tracked).
  // TODO: See if we can make BISON enumeration E21 into an enum
  std::string show_flexible_trip;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1ScheduleVersion    *p_schedule_version    = nullptr;
  Kv1Line               *p_line                = nullptr;
  Kv1JourneyPattern     *p_journey_pattern     = nullptr;
  Kv1UserStopPoint      *p_user_stop           = nullptr;
};

// KV1 Table 29: Operating Day [OPERDAY]
//
// Contains the operational calendar. Which package (schedule version) applies
// is specified per day, per organisation unit.
//
// This table is part of the KV1 variant "schedules and passing times".
struct Kv1OperatingDay {
  struct Key {
    // Mandatory (key), at most 10 characters. Transport operator (from list as
    // defined in BISON enumeration E1).
    std::string data_owner_code;
    // Mandatory (key), at most 10 characters.
    std::string organizational_unit_code;
    // Mandatory (key), at most 10 characters.
    std::string schedule_code;
    // Mandatory (key), at most 10 characters.
    std::string schedule_type_code;
    // Mandatory (key), at most 10 characters. Date on which the package
    // (schedule version) applies. Format: "YYYY-MM-DD".
    std::chrono::year_month_day valid_date;

    explicit Key(std::string data_owner_code,
                 std::string organizational_unit_code,
                 std::string schedule_code,
                 std::string schedule_type_code,
                 std::chrono::year_month_day valid_date);
  };

  Key key;
  // Optional, at most 255 characters.
  std::string description;

  Kv1OrganizationalUnit *p_organizational_unit = nullptr;
  Kv1ScheduleVersion    *p_schedule_version    = nullptr;
};

bool operator==(const Kv1OrganizationalUnit::Key &a, const Kv1OrganizationalUnit::Key &b);
bool operator==(const Kv1HigherOrganizationalUnit::Key &a, const Kv1HigherOrganizationalUnit::Key &b);
bool operator==(const Kv1UserStopPoint::Key &a, const Kv1UserStopPoint::Key &b);
bool operator==(const Kv1UserStopArea::Key &a, const Kv1UserStopArea::Key &b);
bool operator==(const Kv1TimingLink::Key &a, const Kv1TimingLink::Key &b);
bool operator==(const Kv1Link::Key &a, const Kv1Link::Key &b);
bool operator==(const Kv1Line::Key &a, const Kv1Line::Key &b);
bool operator==(const Kv1Destination::Key &a, const Kv1Destination::Key &b);
bool operator==(const Kv1JourneyPattern::Key &a, const Kv1JourneyPattern::Key &b);
bool operator==(const Kv1ConcessionFinancerRelation::Key &a, const Kv1ConcessionFinancerRelation::Key &b);
bool operator==(const Kv1ConcessionArea::Key &a, const Kv1ConcessionArea::Key &b);
bool operator==(const Kv1Financer::Key &a, const Kv1Financer::Key &b);
bool operator==(const Kv1JourneyPatternTimingLink::Key &a, const Kv1JourneyPatternTimingLink::Key &b);
bool operator==(const Kv1Point::Key &a, const Kv1Point::Key &b);
bool operator==(const Kv1PointOnLink::Key &a, const Kv1PointOnLink::Key &b);
bool operator==(const Kv1Icon::Key &a, const Kv1Icon::Key &b);
bool operator==(const Kv1Notice::Key &a, const Kv1Notice::Key &b);
bool operator==(const Kv1TimeDemandGroup::Key &a, const Kv1TimeDemandGroup::Key &b);
bool operator==(const Kv1TimeDemandGroupRunTime::Key &a, const Kv1TimeDemandGroupRunTime::Key &b);
bool operator==(const Kv1PeriodGroup::Key &a, const Kv1PeriodGroup::Key &b);
bool operator==(const Kv1SpecificDay::Key &a, const Kv1SpecificDay::Key &b);
bool operator==(const Kv1TimetableVersion::Key &a, const Kv1TimetableVersion::Key &b);
bool operator==(const Kv1PublicJourney::Key &a, const Kv1PublicJourney::Key &b);
bool operator==(const Kv1PeriodGroupValidity::Key &a, const Kv1PeriodGroupValidity::Key &b);
bool operator==(const Kv1ExceptionalOperatingDay::Key &a, const Kv1ExceptionalOperatingDay::Key &b);
bool operator==(const Kv1ScheduleVersion::Key &a, const Kv1ScheduleVersion::Key &b);
bool operator==(const Kv1PublicJourneyPassingTimes::Key &a, const Kv1PublicJourneyPassingTimes::Key &b);
bool operator==(const Kv1OperatingDay::Key &a, const Kv1OperatingDay::Key &b);

size_t hash_value(const Kv1OrganizationalUnit::Key &k);
size_t hash_value(const Kv1HigherOrganizationalUnit::Key &k);
size_t hash_value(const Kv1UserStopPoint::Key &k);
size_t hash_value(const Kv1UserStopArea::Key &k);
size_t hash_value(const Kv1TimingLink::Key &k);
size_t hash_value(const Kv1Link::Key &k);
size_t hash_value(const Kv1Line::Key &k);
size_t hash_value(const Kv1Destination::Key &k);
size_t hash_value(const Kv1JourneyPattern::Key &k);
size_t hash_value(const Kv1ConcessionFinancerRelation::Key &k);
size_t hash_value(const Kv1ConcessionArea::Key &k);
size_t hash_value(const Kv1Financer::Key &k);
size_t hash_value(const Kv1JourneyPatternTimingLink::Key &k);
size_t hash_value(const Kv1Point::Key &k);
size_t hash_value(const Kv1PointOnLink::Key &k);
size_t hash_value(const Kv1Icon::Key &k);
size_t hash_value(const Kv1Notice::Key &k);
size_t hash_value(const Kv1TimeDemandGroup::Key &k);
size_t hash_value(const Kv1TimeDemandGroupRunTime::Key &k);
size_t hash_value(const Kv1PeriodGroup::Key &k);
size_t hash_value(const Kv1SpecificDay::Key &k);
size_t hash_value(const Kv1TimetableVersion::Key &k);
size_t hash_value(const Kv1PublicJourney::Key &k);
size_t hash_value(const Kv1PeriodGroupValidity::Key &k);
size_t hash_value(const Kv1ExceptionalOperatingDay::Key &k);
size_t hash_value(const Kv1ScheduleVersion::Key &k);
size_t hash_value(const Kv1PublicJourneyPassingTimes::Key &k);
size_t hash_value(const Kv1OperatingDay::Key &k);

#endif // OEUF_LIBTMI8_KV1_TYPES_HPP
