// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/metrics.h"

#include <string>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/strings/stringprintf.h"
#include "components/rappor/public/rappor_utils.h"

namespace ntp_tiles {
namespace metrics {

namespace {

// Maximum number of tiles to record in histograms.
const int kMaxNumTiles = 12;

const int kLastTitleSource = static_cast<int>(TileTitleSource::LAST);

// Identifiers for the various tile sources.
const char kHistogramClientName[] = "client";
const char kHistogramServerName[] = "server";
const char kHistogramPopularName[] = "popular_fetched";
const char kHistogramBakedInName[] = "popular_baked_in";
const char kHistogramWhitelistName[] = "whitelist";
const char kHistogramHomepageName[] = "homepage";

// Suffixes for the various icon types.
const char kTileTypeSuffixIconColor[] = "IconsColor";
const char kTileTypeSuffixIconGray[] = "IconsGray";
const char kTileTypeSuffixIconReal[] = "IconsReal";
const char kTileTypeSuffixThumbnail[] = "Thumbnail";
const char kTileTypeSuffixThumbnailFailed[] = "ThumbnailFailed";

void LogUmaHistogramAge(const std::string& name, const base::TimeDelta& value) {
  // Log the value in number of seconds.
  base::UmaHistogramCustomCounts(name, value.InSeconds(), 5,
                                 base::TimeDelta::FromDays(14).InSeconds(), 20);
}

std::string GetSourceHistogramName(TileSource source) {
  switch (source) {
    case TileSource::TOP_SITES:
      return kHistogramClientName;
    case TileSource::POPULAR_BAKED_IN:
      return kHistogramBakedInName;
    case TileSource::POPULAR:
      return kHistogramPopularName;
    case TileSource::WHITELIST:
      return kHistogramWhitelistName;
    case TileSource::SUGGESTIONS_SERVICE:
      return kHistogramServerName;
    case TileSource::HOMEPAGE:
      return kHistogramHomepageName;
  }
  NOTREACHED();
  return std::string();
}

const char* GetTileTypeSuffix(TileVisualType type) {
  switch (type) {
    case TileVisualType::ICON_COLOR:
      return kTileTypeSuffixIconColor;
    case TileVisualType::ICON_DEFAULT:
      return kTileTypeSuffixIconGray;
    case TileVisualType::ICON_REAL:
      return kTileTypeSuffixIconReal;
    case THUMBNAIL:
      return kTileTypeSuffixThumbnail;
    case THUMBNAIL_FAILED:
      return kTileTypeSuffixThumbnailFailed;
    case TileVisualType::NONE:                     // Fall through.
    case TileVisualType::UNKNOWN_TILE_TYPE:
      break;
  }
  return nullptr;
}

}  // namespace

void RecordPageImpression(int number_of_tiles) {
  UMA_HISTOGRAM_SPARSE_SLOWLY("NewTabPage.NumberOfTiles", number_of_tiles);
}

void RecordTileImpression(const NTPTileImpression& impression,
                          rappor::RapporService* rappor_service) {
  UMA_HISTOGRAM_ENUMERATION("NewTabPage.SuggestionsImpression",
                            impression.index, kMaxNumTiles);

  std::string source_name = GetSourceHistogramName(impression.source);
  base::UmaHistogramExactLinear(
      base::StringPrintf("NewTabPage.SuggestionsImpression.%s",
                         source_name.c_str()),
      impression.index, kMaxNumTiles);

  if (!impression.data_generation_time.is_null()) {
    const base::TimeDelta age =
        base::Time::Now() - impression.data_generation_time;
    LogUmaHistogramAge("NewTabPage.SuggestionsImpressionAge", age);
    LogUmaHistogramAge(
        base::StringPrintf("NewTabPage.SuggestionsImpressionAge.%s",
                           source_name.c_str()),
        age);
  }

  UMA_HISTOGRAM_ENUMERATION("NewTabPage.TileTitle",
                            static_cast<int>(impression.title_source),
                            kLastTitleSource + 1);
  base::UmaHistogramExactLinear(
      base::StringPrintf("NewTabPage.TileTitle.%s",
                         GetSourceHistogramName(impression.source).c_str()),
      static_cast<int>(impression.title_source), kLastTitleSource + 1);

  if (impression.visual_type > LAST_RECORDED_TILE_TYPE) {
    return;
  }

  UMA_HISTOGRAM_ENUMERATION("NewTabPage.TileType", impression.visual_type,
                            LAST_RECORDED_TILE_TYPE + 1);

  base::UmaHistogramExactLinear(
      base::StringPrintf("NewTabPage.TileType.%s", source_name.c_str()),
      impression.visual_type, LAST_RECORDED_TILE_TYPE + 1);

  const char* tile_type_suffix = GetTileTypeSuffix(impression.visual_type);
  if (tile_type_suffix) {
    if (!impression.url_for_rappor.is_empty()) {
      // Note: This handles a null |rappor_service|.
      rappor::SampleDomainAndRegistryFromGURL(
          rappor_service,
          base::StringPrintf("NTP.SuggestionsImpressions.%s", tile_type_suffix),
          impression.url_for_rappor);
    }

    base::UmaHistogramExactLinear(
        base::StringPrintf("NewTabPage.SuggestionsImpression.%s",
                           tile_type_suffix),
        impression.index, kMaxNumTiles);

    if (impression.icon_type != favicon_base::IconType::kInvalid) {
      base::UmaHistogramExactLinear(
          base::StringPrintf("NewTabPage.TileFaviconType.%s", tile_type_suffix),
          favicon_base::GetUmaFaviconType(impression.icon_type),
          favicon_base::GetUmaFaviconType(favicon_base::IconType::kMax) + 1);
    }
  }

  if (impression.icon_type != favicon_base::IconType::kInvalid) {
    base::UmaHistogramExactLinear(
        "NewTabPage.TileFaviconType",
        favicon_base::GetUmaFaviconType(impression.icon_type),
        favicon_base::GetUmaFaviconType(favicon_base::IconType::kMax) + 1);
  }
}

void RecordTileClick(const NTPTileImpression& impression) {
  UMA_HISTOGRAM_ENUMERATION("NewTabPage.MostVisited", impression.index,
                            kMaxNumTiles);

  std::string source_name = GetSourceHistogramName(impression.source);
  base::UmaHistogramExactLinear(
      base::StringPrintf("NewTabPage.MostVisited.%s", source_name.c_str()),
      impression.index, kMaxNumTiles);

  if (!impression.data_generation_time.is_null()) {
    const base::TimeDelta age =
        base::Time::Now() - impression.data_generation_time;
    LogUmaHistogramAge("NewTabPage.MostVisitedAge", age);
    LogUmaHistogramAge(
        base::StringPrintf("NewTabPage.MostVisitedAge.%s", source_name.c_str()),
        age);
  }

  const char* tile_type_suffix = GetTileTypeSuffix(impression.visual_type);
  if (tile_type_suffix) {
    base::UmaHistogramExactLinear(
        base::StringPrintf("NewTabPage.MostVisited.%s", tile_type_suffix),
        impression.index, kMaxNumTiles);

    if (impression.icon_type != favicon_base::IconType::kInvalid) {
      base::UmaHistogramExactLinear(
          base::StringPrintf("NewTabPage.TileFaviconTypeClicked.%s",
                             tile_type_suffix),
          favicon_base::GetUmaFaviconType(impression.icon_type),
          favicon_base::GetUmaFaviconType(favicon_base::IconType::kMax) + 1);
    }
  }

  if (impression.icon_type != favicon_base::IconType::kInvalid) {
    base::UmaHistogramExactLinear(
        "NewTabPage.TileFaviconTypeClicked",
        favicon_base::GetUmaFaviconType(impression.icon_type),
        favicon_base::GetUmaFaviconType(favicon_base::IconType::kMax) + 1);
  }

  UMA_HISTOGRAM_ENUMERATION("NewTabPage.TileTitleClicked",
                            static_cast<int>(impression.title_source),
                            kLastTitleSource + 1);
  base::UmaHistogramExactLinear(
      base::StringPrintf("NewTabPage.TileTitleClicked.%s",
                         GetSourceHistogramName(impression.source).c_str()),
      static_cast<int>(impression.title_source), kLastTitleSource + 1);

  if (impression.visual_type <= LAST_RECORDED_TILE_TYPE) {
    UMA_HISTOGRAM_ENUMERATION("NewTabPage.TileTypeClicked",
                              impression.visual_type,
                              LAST_RECORDED_TILE_TYPE + 1);

    base::UmaHistogramExactLinear(
        base::StringPrintf("NewTabPage.TileTypeClicked.%s",
                           GetSourceHistogramName(impression.source).c_str()),
        impression.visual_type, LAST_RECORDED_TILE_TYPE + 1);
  }
}

}  // namespace metrics
}  // namespace ntp_tiles
