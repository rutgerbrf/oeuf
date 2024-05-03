// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#ifndef OEUF_QUERYKV1_DATERANGE_HPP
#define OEUF_QUERYKV1_DATERANGE_HPP

#include <cassert>
#include <chrono>
#include <concepts>
#include <iterator>
#include <utility>
#include <vector>

// DateRange expresses the date range [from, thru].
class DateRange {
 public:
  class Iterator {
    friend class DateRange;

   public:
    Iterator &operator++();

    std::chrono::year_month_day operator*() const;
    std::chrono::year_month_day ymd() const;

   private:
    explicit Iterator(std::chrono::year_month_day ymd);

    std::chrono::year_month_day ymd_;
  };

  explicit DateRange(std::chrono::year_month_day from, std::chrono::year_month_day thru);

  Iterator begin() const;
  Iterator end() const;
  bool valid() const;
  std::chrono::year_month_day from() const;
  std::chrono::year_month_day thru() const;

 private:
  std::chrono::year_month_day from_;
  std::chrono::year_month_day thru_;
};

bool operator==(const DateRange::Iterator a, const DateRange::Iterator b);

template<typename Tp, typename T>
concept DerefsTo = requires(Tp p) {
  { *p } -> std::convertible_to<T>;
};

class DateRangeSeq {
  // The way LE and GE are ordered makes a difference for how the sorting
  // (insertion based on lower_bound) works. Do not carelessly reorder this.
  enum LeGe {
    GE,  // >=
    LE,  // <=
  };

  std::vector<DateRange> ranges_;

 public:
  template<std::input_iterator InputIt>
    requires DerefsTo<InputIt, DateRange>
  explicit DateRangeSeq(InputIt begin, InputIt end) {
    // We convert every inclusive date range [x, y] into (x, >=) and (y, <=)
    // and put these into a list, using binary search to make sure that these
    // stay ordered. We then reduce this list, removing tautological
    // predicates, giving us a final list of ranges that do not overlap.

    std::vector<std::pair<std::chrono::year_month_day, LeGe>> preds;

    size_t n = 0;
    for (auto it = begin; it != end; it++) {
      auto &range = *it;
      if (!range.valid()) continue;
     
      auto a = std::make_pair(range.from(), GE);
      auto b = std::make_pair(range.thru(), LE);
      preds.insert(std::lower_bound(preds.begin(), preds.end(), a), a);
      preds.insert(std::lower_bound(preds.begin(), preds.end(), b), b);

      n++;
    }

    if (preds.empty())
      return;

    assert(preds.size() >= 2);
    assert(preds.front().second == GE);
    assert(preds.back().second == LE);

    std::chrono::year_month_day begin_ymd = preds[0].first;
    for (size_t i = 1; i < preds.size(); i++) {
      if (preds[i].second == LE && (i + 1 == preds.size() || preds[i + 1].second == GE)) {
        std::chrono::year_month_day end_ymd = preds[i].first;
        if (!ranges_.empty() && ranges_.back().thru() == begin_ymd)
          ranges_.back() = DateRange(ranges_.back().from(), end_ymd);
        else
          ranges_.push_back(DateRange(begin_ymd, end_ymd));
        if (i + 1 != preds.size()) {
          begin_ymd = preds[i + 1].first;
          i++;
        }
      }
    }
  }

  explicit DateRangeSeq(std::initializer_list<DateRange> ranges);

  DateRangeSeq clampFrom(std::chrono::year_month_day from) const;
  DateRangeSeq clampThru(std::chrono::year_month_day thru) const;

 public:
  std::vector<DateRange>::const_iterator begin() const;
  std::vector<DateRange>::const_iterator end() const;
};

#endif // OEUF_QUERYKV1_DATERANGE_HPP
