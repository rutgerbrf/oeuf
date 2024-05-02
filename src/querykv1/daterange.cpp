// vim:set sw=2 ts=2 sts et:

#include "daterange.hpp"

static std::chrono::year_month_day nextDay(std::chrono::year_month_day ymd) {
  return std::chrono::sys_days(ymd) + std::chrono::days(1);
}

// DateRange expresses the date range [from, thru].
DateRange::Iterator &DateRange::Iterator::operator++() {
  ymd_ = nextDay(ymd_);
  return *this;
}

std::chrono::year_month_day DateRange::Iterator::operator*() const {
  return ymd_;
}

std::chrono::year_month_day DateRange::Iterator::ymd() const {
  return ymd_;
}

DateRange::Iterator::Iterator(std::chrono::year_month_day ymd) : ymd_(ymd) {}

DateRange::DateRange(std::chrono::year_month_day from, std::chrono::year_month_day thru)
 : from_(from), thru_(thru)
{}

DateRange::Iterator DateRange::begin() const {
  return DateRange::Iterator(from_);
}

DateRange::Iterator DateRange::end() const {
  return DateRange::Iterator(nextDay(thru_));
}

bool DateRange::valid() const {
  return from_ <= thru_;
}

std::chrono::year_month_day DateRange::from() const {
  return from_;
}

std::chrono::year_month_day DateRange::thru() const {
  return thru_;
}

bool operator==(const DateRange::Iterator a, const DateRange::Iterator b) {
  return *a == *b;
}

DateRangeSeq::DateRangeSeq(std::initializer_list<DateRange> ranges)
  : DateRangeSeq(ranges.begin(), ranges.end())
{}

DateRangeSeq DateRangeSeq::clampFrom(std::chrono::year_month_day from) const {
  std::vector<DateRange> new_ranges;
  new_ranges.reserve(ranges_.size());
  for (const DateRange range : ranges_) {
    if (range.from() < from) {
      if (range.thru() < from)
        continue;
      new_ranges.emplace_back(from, range.thru());
    }
    new_ranges.push_back(range);
  }
  return DateRangeSeq(new_ranges.begin(), new_ranges.end());
}

DateRangeSeq DateRangeSeq::clampThru(std::chrono::year_month_day thru) const {
  std::vector<DateRange> new_ranges;
  new_ranges.reserve(ranges_.size());
  for (const DateRange range : ranges_) {
    if (range.thru() > thru) {
      if (range.from() > thru)
        continue;
      new_ranges.emplace_back(range.from(), thru);
    }
    new_ranges.push_back(range);
  }
  return DateRangeSeq(new_ranges.begin(), new_ranges.end());
}

std::vector<DateRange>::const_iterator DateRangeSeq::begin() const {
  return ranges_.begin();
}

std::vector<DateRange>::const_iterator DateRangeSeq::end() const {
  return ranges_.end();
}
