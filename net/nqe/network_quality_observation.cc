// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/network_quality_observation.h"
#include "base/macros.h"

namespace net {

namespace nqe {

namespace internal {

Observation::Observation(int32_t value,
                         base::TimeTicks timestamp,
                         const base::Optional<int32_t>& signal_strength,
                         NetworkQualityObservationSource source)
    : Observation(value, timestamp, signal_strength, source, base::nullopt) {}

Observation::Observation(int32_t value,
                         base::TimeTicks timestamp,
                         const base::Optional<int32_t>& signal_strength,
                         NetworkQualityObservationSource source,
                         const base::Optional<IPHash>& host)
    : value_(value),
      timestamp_(timestamp),
      signal_strength_(signal_strength),
      source_(source),
      host_(host) {
  DCHECK(!timestamp_.is_null());
}

Observation::Observation(const Observation& other) = default;

Observation& Observation::operator=(const Observation& other) = default;

Observation::~Observation() {}

}  // namespace internal

}  // namespace nqe

}  // namespace net
