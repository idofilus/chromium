// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_NORMALIZER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_NORMALIZER_H_

#include <string>

#include "base/callback_forward.h"
#include "third_party/libaddressinput/chromium/chrome_address_validator.h"

namespace autofill {

class AutofillProfile;

// A class used to normalize addresses.
class AddressNormalizer : public autofill::LoadRulesListener {
 public:
  using NormalizationCallback =
      base::OnceCallback<void(bool /*success*/,
                              const AutofillProfile& /*normalized_profile*/)>;

  // Start loading the validation rules for the specified |region_code|.
  virtual void LoadRulesForRegion(const std::string& region_code) = 0;

  // Returns whether the rules for the specified |region_code| have finished
  // loading.
  virtual bool AreRulesLoadedForRegion(const std::string& region_code) = 0;

  // Normalize |profile| asynchronously based on the |region_code|. If the
  // normalization is not completed in |timeout_seconds|, |callback| will be
  // called with success=false. If |timeout_seconds| is 0, |callback| is called
  // immediately, and may have success=false if the rules had not already been
  // loaded. Will start loading the rules for the |region_code| if they had not
  // started loading. The phone number gets normalized to the E.164 format,
  // which is notably compatible with Payment Request. See documentation of
  // PhoneNumberFormat::E164 for details.
  virtual void NormalizeAddressAsync(const AutofillProfile& profile,
                                     const std::string& region_code,
                                     int timeout_seconds,
                                     NormalizationCallback callback) = 0;

  // Normalizes |profile| and returns whether it was successful. Callers should
  // call |AreRulesLoadedForRegion| to ensure success.
  virtual bool NormalizeAddressSync(AutofillProfile* profile,
                                    const std::string& region_code) = 0;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_ADDRESS_NORMALIZER_H_
