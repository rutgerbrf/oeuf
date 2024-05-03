// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#ifndef OEUF_QUERYKV1_SCHEDULE_HPP
#define OEUF_QUERYKV1_SCHEDULE_HPP

#include <tmi8/kv1_types.hpp>
#include <tmi8/kv1_index.hpp>

#include "cliopts.hpp"

void schedule(const Options &options, Kv1Records &records, Kv1Index &index);

#endif // OEUF_QUERYKV1_SCHEDULE_HPP
