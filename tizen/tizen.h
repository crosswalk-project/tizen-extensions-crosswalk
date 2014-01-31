// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_TIZEN_H_
#define TIZEN_TIZEN_H_

// WARNING! This list should be in sync with the equivalent list
// located at tizen_api.js. Remember to update tizen_api.js if you
// change something here.
enum WebApiAPIErrors {
  // NO_ERROR is not really a valid error, but can be used
  // to indicate that no error occoured instead of having an
  // extra field in the message protocol just for that.
  NO_ERROR = -1,

  UNKNOWN_ERR = 0,
  INDEX_SIZE_ERR = 1,
  DOMSTRING_SIZE_ERR = 2,
  HIERARCHY_REQUEST_ERR = 3,
  WRONG_DOCUMENT_ERR = 4,
  INVALID_CHARACTER_ERR = 5,
  NO_DATA_ALLOWED_ERR = 6,
  NO_MODIFICATION_ALLOWED_ERR = 7,
  NOT_FOUND_ERR = 8,
  NOT_SUPPORTED_ERR = 9,
  INUSE_ATTRIBUTE_ERR = 10,
  INVALID_STATE_ERR = 11,
  SYNTAX_ERR = 12,
  INVALID_MODIFICATION_ERR = 13,
  NAMESPACE_ERR = 14,
  INVALID_ACCESS_ERR = 15,
  VALIDATION_ERR = 16,
  TYPE_MISMATCH_ERR = 17,
  SECURITY_ERR = 18,
  NETWORK_ERR = 19,
  ABORT_ERR = 20,
  URL_MISMATCH_ERR = 21,
  QUOTA_EXCEEDED_ERR = 22,
  TIMEOUT_ERR = 23,
  INVALID_NODE_TYPE_ERR = 24,
  DATA_CLONE_ERR = 25,

  // Error codes for these errors are not really defined anywhere.
  INVALID_VALUES_ERR = 100,
  IO_ERR = 101,
  PERMISSION_DENIED_ERR = 102,
  SERVICE_NOT_AVAILABLE_ERR = 103,
  DATABASE_ERR = 104,
};

#define STR_MATCH_EXACTLY        "EXACTLY"
#define STR_MATCH_FULLSTRING     "FULLSTRING"
#define STR_MATCH_CONTAINS       "CONTAINS"
#define STR_MATCH_STARTSWITH     "STARTSWITH"
#define STR_MATCH_ENDSWITH       "ENDSWITH"
#define STR_MATCH_EXISTS         "EXISTS"

#define STR_SORT_ASC             "ASC"
#define STR_SORT_DESC            "DESC"

#define STR_FILTEROP_OR          "UNION"
#define STR_FILTEROP_AND         "INTERSECTION"

#endif  // TIZEN_TIZEN_H_
