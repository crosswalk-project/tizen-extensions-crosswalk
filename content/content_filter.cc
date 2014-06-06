// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/content_filter.h"
#include <map>

ContentFilter& ContentFilter::instance() {
  static ContentFilter instance;
  return instance;
}

namespace {
// TODO(spoussa): add lazy loading
const std::map<std::string, std::string>& attributeNameMap = {
  {"id",                    "MEDIA_ID"},
  {"type",                  "MEDIA_TYPE"},
  {"mimeType",              "MEDIA_MIME_TYPE"},
  {"name",                  "MEDIA_DISPLAY_NAME"},
  {"title",                 "MEDIA_TITLE"},
  {"contentURI",            "MEDIA_PATH"},
  {"thumbnailURIs",         "MEDIA_THUMBNAIL_PATH"},
  {"description",           "MEDIA_DESCRIPTION"},
  {"rating",                "MEDIA_RATING"},
  {"createdDate",           "MEDIA_ADDED_TIME"},
  {"releaseDate",           "MEDIA_DATETAKEN"},
  {"modifiedDate",          "MEDIA_MODIFIED_TIME"},
  {"geolocation.latitude",  "MEDIA_LATITUDE"},
  {"geolocation.longitude", "MEDIA_LONGITUDE"},
  {"duration",              "MEDIA_DURATION"},
  {"album",                 "MEDIA_ALBUM"},
  {"artists",               "MEDIA_ARTIST"},
  {"width",                 "MEDIA_WIDTH"},
  {"height",                "MEDIA_HEIGHT"},
  {"genres",                "MEDIA_GENRE"},
  {"size",                  "MEDIA_SIZE"},
};

const std::map<std::string, std::string>& opMap = {
  {"EXACTLY",    " = "},
  {"FULLSTRING", " = "},
  {"CONTAINS",   " LIKE "},
  {"STARTSWITH", " LIKE "},
  {"ENDSWITH",   " LIKE "},
  {"EXISTS",     " IS NOT NULL "},
};
}  // namespace

// TODO(spoussa): we only support the attribute filter now.
// Range and composite filters are missing.
std::string ContentFilter::convert(const picojson::value& jsonFilter) {
  std::string attributeName = jsonFilter.get("attributeName").to_str();
  std::string matchFlag = jsonFilter.get("matchFlag").to_str();
  std::string matchValue = jsonFilter.get("matchValue").to_str();

#ifdef DEBUG
  std::cout << "Filter IN: " <<
      attributeName << " " << matchFlag << " " << matchValue << std::endl;
#endif

  std::string query;
  if (attributeName.empty() || matchFlag.empty()) {
    std::cerr <<
        "Filter ERR: attribute or match flag missing" << std::endl;
    return query;
  }

  std::map<std::string, std::string>::const_iterator it;
  it = attributeNameMap.find(attributeName);
  if (it == attributeNameMap.end()) {
    std::cerr << "Filter ERR: unknown attributeName " <<
        attributeName << std::endl;
    return query;
  }
  std::string lValue = it->second;

  it = opMap.find(matchFlag);
  if (it == attributeNameMap.end()) {
    std::cerr << "Filter ERR: unknown matchFlag " << matchFlag << std::endl;
    return query;
  }
  std::string op = it->second;

  // Tizen requires this weird mapping on type
  if (attributeName == "type") {
    if (matchValue == "IMAGE") {
      matchValue = "0";
    } else if (matchValue == "VIDEO") {
      matchValue = "1";
    } else if (matchValue == "AUDIO") {
      matchValue = "3";
    } else if (matchValue == "OTHER") {
      matchValue = "4";
    } else {
      std::cerr << "Filter ERR: unknown media type " << matchValue << std::endl;
      return query;
    }
  }

  const std::string STR_QUOTE("'");
  const std::string STR_PERCENT("%");
  std::string rValue;
  if (matchFlag == "CONTAINS")
    rValue = STR_QUOTE + STR_PERCENT + matchValue + STR_PERCENT + STR_QUOTE;
  else if (matchFlag == "STARTSWITH")
    rValue = STR_QUOTE + matchValue + STR_PERCENT + STR_QUOTE;
  else if (matchFlag == "ENDSWITH")
    rValue = STR_QUOTE + STR_PERCENT + matchValue + STR_QUOTE;
  else if (matchFlag == "FULLSTRING")
    rValue = STR_QUOTE + matchValue + STR_QUOTE + " COLLATE NOCASE ";
  else if (matchFlag == "EXISTS")
    rValue = "";
  else
    rValue = STR_QUOTE + matchValue + STR_QUOTE;

  query = lValue + op + rValue;
#ifdef DEBUG
  std::cout << "Filter OUT: " << query << std::endl;
#endif
  return query;
}
