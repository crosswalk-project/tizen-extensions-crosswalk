// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the IPC implementation between the extension and runtime,
// and the glue code to Tizen specific backend. Specification:
// https://developer.tizen.org/dev-guide/2.2.1/org.tizen.web.device.apireference/tizen/callhistory.html

#include "callhistory/callhistory.h"

#include <contacts.h>

#include <memory>
#include <sstream>

namespace {

// Wrapper for logging; currently cout/cerr is used in Tizen extensions.
#define LOG_ERR(msg) std::cerr << "\n[Error] " << msg

// Global call log view URI variable used from Contacts.
#define CALLH_VIEW_URI        _contacts_phone_log._uri

// CallHistoryEntry attribute id's from Contacts global call log view var.
#define CALLH_ATTR_UID        _contacts_phone_log.id
#define CALLH_ATTR_PERSONID   _contacts_phone_log.person_id
#define CALLH_ATTR_DIRECTION  _contacts_phone_log.log_type
#define CALLH_ATTR_ADDRESS    _contacts_phone_log.address
#define CALLH_ATTR_STARTTIME  _contacts_phone_log.log_time
#define CALLH_ATTR_DURATION   _contacts_phone_log.extra_data1

#define CALLH_FILTER_NONE    -1
#define CALLH_FILTER_AND      CONTACTS_FILTER_OPERATOR_AND
#define CALLH_FILTER_OR       CONTACTS_FILTER_OPERATOR_OR

inline bool check(int err) {
  return err == CONTACTS_ERROR_NONE;
}

// Map contacts errors to JS errors.
int MapContactErrors(int err) {
  //             Tizen Contacts error code          Tizen WRT error code.
  return (err == CONTACTS_ERROR_NONE              ? NO_ERROR          :
          err == CONTACTS_ERROR_INVALID_PARAMETER ? VALIDATION_ERR    :
          err == CONTACTS_ERROR_DB                ? DATABASE_ERR      :
          err == CONTACTS_ERROR_INTERNAL          ? NOT_SUPPORTED_ERR :
          err == CONTACTS_ERROR_NO_DATA           ? NOT_FOUND_ERR     :
          err == CONTACTS_ERROR_PERMISSION_DENIED ? SECURITY_ERR      :
          err == CONTACTS_ERROR_IPC_NOT_AVALIABLE ? TIMEOUT_ERR       :
          err == CONTACTS_ERROR_IPC               ? TIMEOUT_ERR       :
                                                    UNKNOWN_ERR);
}

// Converting strings from API queries to contacts view identifiers.
// See contacts_views.h ( _contacts_phone_log ).
int MapAttrName(std::string &att) {
  // the order matters, more frequent query attributes come first
  if (att == kCallDirection)
    return CALLH_ATTR_DIRECTION;

  if (att == kExtRemoteParty || att == kRemoteParty)
    return CALLH_ATTR_ADDRESS;

  if (att == kStartTime)
    return CALLH_ATTR_STARTTIME;

  if (att == kEntryID)
    return CALLH_ATTR_UID;

  if (att == kCallDuration)
    return CALLH_ATTR_DURATION;

  return 0;
}

// Helper class for scope destruction of Contact types (opaque pointers).
template <typename T>
class ScopeGuard {
 public:
  ScopeGuard() { var_ = reinterpret_cast<T>(nullptr); }
  ~ScopeGuard() { LOG_ERR("ScopeGuard: type not supported"); }
  T* operator&() { return &var_; }  // NOLINT "unary & is dangerous"
  void operator=(T& var) { var_ = var; }
 private:
  T var_;
};

// Specialized destructors for all supported Contact types.
template<>
inline ScopeGuard<contacts_filter_h>::~ScopeGuard() {
  if (var_ && !check(contacts_filter_destroy(var_)))
    LOG_ERR("ScopeGuard: failed to destroy contacts_filter_h");
}

template<>
inline ScopeGuard<contacts_list_h>::~ScopeGuard() {
  if (var_ && !check(contacts_list_destroy(var_, true)))
    LOG_ERR("ScopeGuard: failed to destroy contacts_list_h");
}

template<>
inline ScopeGuard<contacts_record_h>::~ScopeGuard() {
  if (var_ && !check(contacts_record_destroy(var_, true)))
    LOG_ERR("ScopeGuard: failed to destroy contacts_record_h");
}

template<>
inline ScopeGuard<contacts_query_h>::~ScopeGuard() {
  if (var_ && !check(contacts_query_destroy(var_)))
    LOG_ERR("ScopeGuard: failed to destroy contacts_query_h");
}

// Macros interfacing with C code from Contacts API.
#define CHK(fnc) do { int _er = (fnc); \
  if (!check(_er)) { LOG_ERR(#fnc " failed"); return _er;} } while (0)

#define CHK_MAP(fnc) do { int _er = (fnc); \
  if (!check(_er)) { LOG_ERR(#fnc " failed"); \
    return MapContactErrors(_er); } } while (0)


picojson::value JsonFromInt(int val) {
  return picojson::value(static_cast<double>(val));
}

picojson::value JsonFromTime(time_t val) {
  char timestr[40];
  // Instead "struct tm* tms = localtime(&val);" use the reentrant version.
  time_t tme = time(nullptr);
  struct tm tm_s = {0};
  localtime_r(&tme, &tm_s);
  struct tm* tms = &tm_s;

  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S.%06u GMT%z", tms);
  return picojson::value(std::string(timestr));
}

picojson::value JsonTimeFromInt(int val) {
  time_t tval = static_cast<time_t>(val);
  return JsonFromTime(tval);
}

// Needed because v.get() asserts if type is wrong, this one fails gracefully.
bool IntFromJson(const picojson::value& v, int* result) {
  if (!result || !v.is<double>())
    return false;
  *result = static_cast<int>(v.get<double>());
  return true;
}

// Read Contacts record, and prepare JSON array element for the "result"
// property used in setMessageListener() JS function in callhistory_api.js.
int SerializeEntry(contacts_record_h record, picojson::value::object& o) {
  int int_val;
  char* string_val;

  o["type"] = picojson::value("TEL");  // for now, only "TEL" is supported

  CHK(contacts_record_get_int(record, CALLH_ATTR_UID, &int_val));
  o["uid"] = JsonFromInt(int_val);

  CHK(contacts_record_get_int(record, CALLH_ATTR_DURATION, &int_val));
  o["duration"] = JsonFromInt(int_val);

  CHK(contacts_record_get_int(record, CALLH_ATTR_STARTTIME, &int_val));
  o["startTime"] = JsonTimeFromInt(int_val);

  picojson::value::array features;
  features.push_back(picojson::value("CALL"));  // common to all

  CHK(contacts_record_get_int(record, CALLH_ATTR_DIRECTION, &int_val));
  switch (int_val) {
    case CONTACTS_PLOG_TYPE_VIDEO_INCOMMING:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("RECEIVED");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_INCOMMING:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("RECEIVED");
      break;
    case CONTACTS_PLOG_TYPE_VIDEO_OUTGOING:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("DIALED");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_OUTGOING:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("DIALED");
      break;
    case CONTACTS_PLOG_TYPE_VIDEO_INCOMMING_UNSEEN:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("MISSEDNEW");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_INCOMMING_UNSEEN:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("MISSEDNEW");
      break;
    case CONTACTS_PLOG_TYPE_VIDEO_INCOMMING_SEEN:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("MISSED");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_INCOMMING_SEEN:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("MISSED");
      break;
    case CONTACTS_PLOG_TYPE_VIDEO_REJECT:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("REJECTED");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_REJECT:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("REJECTED");
      break;
    case CONTACTS_PLOG_TYPE_VIDEO_BLOCKED:
      features.push_back(picojson::value("VIDEOCALL"));
      o["direction"] = picojson::value("BLOCKED");
      break;
    case CONTACTS_PLOG_TYPE_VOICE_BLOCKED:
      features.push_back(picojson::value("VOICECALL"));
      o["direction"] = picojson::value("BLOCKED");
      break;
    default:
      LOG_ERR("SerializeEntry(): invalid 'direction'");
      break;
  }

  o["features"] = picojson::value(features);

  picojson::value::array rp_list;
  picojson::value::object r_party;
  CHK(contacts_record_get_str_p(record, CALLH_ATTR_ADDRESS, &string_val));
  r_party["remoteParty"] = picojson::value(string_val);

  CHK(contacts_record_get_int(record, CALLH_ATTR_PERSONID, &int_val));
  r_party["personId"] = JsonFromInt(int_val);

  rp_list.push_back(picojson::value(r_party));
  o["remoteParties"] = picojson::value(rp_list);

  return CONTACTS_ERROR_NONE;
}

/*
Setting up native filters
3 levels of methods are used, which all add partial filters to a common
contacts filter:
- ParseFilter() for dispatching,
- add[Attribute|Range|Composite]Filter, for parsing and integrating,
- map[Attribute|Range]Filter to map to contacts filters.

Examples for full JSON commands:
{ "cmd":"find",
  "filter": {
    "attributeName":"direction",
    "matchFlag":"EXACTLY",
    "matchValue":"DIALED"
  },
  "sortMode": {
    "attributeName":"startTime",
    "order":"DESC"
  },
  "limit":null,
  "offset":null,
  "reply_id":3
}

{ "cmd":"find",
  "filter": {
    "attributeName":"startTime",
    "initialValue": "2013-12-30T15:18:22.077Z",
    "endValue":     "2012-12-30T15:18:22.077Z"
  },
  "sortMode":{
   "attributeName":"startTime",
   "order":"DESC"
  },
  "limit":null,
  "offset":null,
  "reply_id":4
}

{ "cmd":"find",
  "filter": {
    "type":"INTERSECTION",
    "filters":[
      { "attributeName":"remoteParties",
        "matchFlag":"CONTAINS",
        "matchValue":"John"
      },
      { "attributeName":"direction",
        "matchFlag":"EXACTLY",
        "matchValue":"RECEIVED"
      }
    ]},
  "sortMode": {
    "attributeName":"startTime",
    "order":"DESC"
  },
  "limit":100,
  "offset":0,
  "reply_id":1
}
*/

// Map an attribute filter to an existing contacts filter.
int MapAttributeFilter(contacts_filter_h& filter,
                       const std::string& attr,
                       const std::string& match_flag,
                       const picojson::value& value) {
  unsigned int prop_id;
  // Valid match flags: "EXACTLY",    "FULLSTRING", "CONTAINS",
  //                    "STARTSWITH", "ENDSWITH",   "EXISTS".
  contacts_match_str_flag_e mflag = CONTACTS_MATCH_EXACTLY;

  // More frequently used attributes are in the front.
  if (attr == kCallDirection) {
    // Valid matchflags: EXACTLY, EXISTS.
    // Values: "DIALED","RECEIVED","MISSEDNEW","MISSED","BLOCKED","REJECTED"
    prop_id = CALLH_ATTR_DIRECTION;

    if (match_flag == STR_MATCH_EXISTS)
      return CONTACTS_ERROR_NONE;  // no extra filter is set up
    else if (match_flag != STR_MATCH_EXACTLY)
      return CONTACTS_ERROR_DB;  // error

    int voice = 0, video = 0;
    std::string dir = value.to_str();

    if (dir == kDialedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_OUTGOING;  // should be DIALED
      video = CONTACTS_PLOG_TYPE_VIDEO_OUTGOING;  // here, too
    } else if (dir == kReceivedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_INCOMMING;  // should be INCOMING
      video = CONTACTS_PLOG_TYPE_VIDEO_INCOMMING;  // here, too
    } else if (dir == kUnseenMissedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_INCOMMING_UNSEEN;  // here, too
      video = CONTACTS_PLOG_TYPE_VIDEO_INCOMMING_UNSEEN;  // here, too
    } else if (dir == kMissedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_INCOMMING_SEEN;  // here, too
      video = CONTACTS_PLOG_TYPE_VIDEO_INCOMMING_SEEN;  // here, too
    } else if (dir == kRejectedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_REJECT;  // should be REJECTED
      video = CONTACTS_PLOG_TYPE_VIDEO_REJECT;  // here, too
    } else if (dir == kBlockedCall) {
      voice = CONTACTS_PLOG_TYPE_VOICE_BLOCKED;
      video = CONTACTS_PLOG_TYPE_VIDEO_BLOCKED;
    }

    CHK(contacts_filter_add_int(filter, prop_id, CONTACTS_MATCH_EQUAL, voice));
    CHK(contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_OR));
    CHK(contacts_filter_add_int(filter, prop_id, CONTACTS_MATCH_EQUAL, video));
  } else if (attr == kCallFeatures) {
    // Valid matchflags: EXACTLY, EXISTS.
    // values: "CALL", "VOICECALL", "VIDEOCALL", "EMERGENCYCALL"
    return CONTACTS_ERROR_INTERNAL;  // not supported now
  } else if (attr == kEntryID) {
    // matchflags: EXACTLY, EXISTS, map from string to int
    prop_id = CALLH_ATTR_UID;

    if (match_flag != STR_MATCH_EXACTLY)
      return CONTACTS_ERROR_INVALID_PARAMETER;

    int val;
    if (!IntFromJson(value, &val))
      return CONTACTS_ERROR_DB;

    CHK(contacts_filter_add_int(filter, prop_id, CONTACTS_MATCH_EQUAL, val));
  } else if (attr == kCallType) {
    // Valid matchflags: EXACTLY, EXISTS.
    // values: "TEL", "XMPP", "SIP"
    // the implementation of contacts doesn't support it
    return CONTACTS_ERROR_INTERNAL;
  } else if (attr == kExtRemoteParty) {  // only for exact match
    // Valid matchflags: EXACTLY.
    prop_id = CALLH_ATTR_ADDRESS;

    if (match_flag != STR_MATCH_EXACTLY)
      return CONTACTS_ERROR_INVALID_PARAMETER;

    CHK(contacts_filter_add_str(filter, prop_id, mflag,
                                value.to_str().c_str()));
  } else if (attr == kRemoteParties || attr == kRemoteParty) {
    // Valid matchflags: all flags.
    prop_id = CALLH_ATTR_ADDRESS;

    if (match_flag == STR_MATCH_EXACTLY)
      mflag = CONTACTS_MATCH_EXACTLY;
    else if (match_flag == STR_MATCH_FULLSTRING)
      mflag = CONTACTS_MATCH_FULLSTRING;
    else if (match_flag == STR_MATCH_CONTAINS)
      mflag = CONTACTS_MATCH_CONTAINS;
    else if (match_flag == STR_MATCH_STARTSWITH)
      mflag = CONTACTS_MATCH_STARTSWITH;
    else if (match_flag == STR_MATCH_ENDSWITH)
      mflag = CONTACTS_MATCH_ENDSWITH;
    else if (match_flag == STR_MATCH_EXISTS)
      mflag = CONTACTS_MATCH_EXISTS;

    CHK(contacts_filter_add_str(filter, prop_id, mflag,
                                value.to_str().c_str()));
  } else {
    LOG_ERR("MapAttributeFilter " << attr << " not supported");
    return CONTACTS_ERROR_INTERNAL;
  }

  return CONTACTS_ERROR_NONE;
}

// map an attribute range filter to an existing contacts filter
int MapRangeFilter(contacts_filter_h& filter,
                   const std::string& attr,
                   const picojson::value& start_value,
                   const picojson::value& end_value) {
  unsigned int prop_id = 0;
  int sv, ev;
  bool is_start, is_end;

  is_start = start_value.is<picojson::null>();
  is_end = end_value.is<picojson::null>();

  if (attr == kStartTime) {
    prop_id = CALLH_ATTR_STARTTIME;
  } else if (attr == kCallDuration) {
    prop_id = CALLH_ATTR_DURATION;
  }
  // No other attribute supports range filtering.

  if (is_start) {
    if (!IntFromJson(start_value, &sv))
      return CONTACTS_ERROR_DB;

    CHK(contacts_filter_add_int(filter, prop_id, \
      CONTACTS_MATCH_GREATER_THAN_OR_EQUAL, sv));
  }

  if (is_end) {
    if (!IntFromJson(end_value, &ev))
      return CONTACTS_ERROR_DB;

    if (is_start)
      CHK(contacts_filter_add_operator(filter, CONTACTS_FILTER_OPERATOR_AND));

    CHK(contacts_filter_add_int(filter, prop_id, \
      CONTACTS_MATCH_LESS_THAN_OR_EQUAL, ev));
  }

  return  CONTACTS_ERROR_NONE;
}

int ParseFilter(contacts_filter_h& filter,
                const picojson::value& js_filter,
                int filter_op,
                bool& is_empty);

// Parse a composite filter and add to the existing contacts filter.
int AddCompositeFilter(contacts_filter_h& filter,
                       const picojson::value& jsf,
                       int filter_op,
                       bool& is_empty) {
  std::string comp_filt_type = jsf.get("type").to_str();
  contacts_filter_operator_e inner_op;
  if (comp_filt_type == "INTERSECTION")
    inner_op = CALLH_FILTER_AND;
  else if (comp_filt_type == "UNION")
    inner_op = CALLH_FILTER_OR;
  else
    return CONTACTS_ERROR_INVALID_PARAMETER;

  ScopeGuard<contacts_filter_h> filt;
  contacts_filter_h* pfilt = nullptr;
  if (filter_op == CALLH_FILTER_NONE || is_empty) {
    pfilt = &filter;
  } else if (filter_op == CALLH_FILTER_OR || filter_op == CALLH_FILTER_AND) {
    CHK(contacts_filter_create(CALLH_VIEW_URI, &filt));
    pfilt = &filt;
  }

  // For each filter in 'filters', add it to '*pfilt'.
  picojson::array a = jsf.get("filters").get<picojson::array>();
  for (picojson::array::iterator it = a.begin(); it != a.end(); ++it) {
    CHK(ParseFilter(*pfilt, *it, inner_op, is_empty));
  }

  // Add the composite filter to 'filter'.
  if (pfilt != &filter && !is_empty) {
    contacts_filter_operator_e fop = (contacts_filter_operator_e) filter_op;
    CHK(contacts_filter_add_operator(filter, fop));
    CHK(contacts_filter_add_filter(filter, *pfilt));
  }
  return CONTACTS_ERROR_NONE;
}

// Parse an attribute filter and add to the existing contacts filter.
int AddAttributeFilter(contacts_filter_h& filter,
                       const picojson::value& jsf,
                       int filter_op,
                       bool& is_empty) {
  ScopeGuard<contacts_filter_h> filt;
  contacts_filter_h* pfilt = nullptr;
  if (filter_op == CALLH_FILTER_NONE || is_empty) {
    pfilt = &filter;
  } else if (filter_op == CALLH_FILTER_OR || filter_op == CALLH_FILTER_AND) {
    CHK(contacts_filter_create(CALLH_VIEW_URI, &filt));
    pfilt = &filt;
  }

  std::string attr = jsf.get("attributeName").to_str();
  std::string mflag = jsf.get("matchFlag").to_str();
  picojson::value mvalue = jsf.get("matchValue");

  CHK(MapAttributeFilter(*pfilt, attr, mflag, mvalue));

  if (pfilt != &filter && !is_empty) {
    contacts_filter_operator_e fop = (contacts_filter_operator_e) filter_op;
    CHK(contacts_filter_add_operator(filter, fop));
    CHK(contacts_filter_add_filter(filter, *pfilt));
  }

  is_empty = false;  // 'filter' was changed, from now on add to it
  return CONTACTS_ERROR_NONE;
}

// Parse an attribute range filter and add to the existing contacts filter.
int AddRangeFilter(contacts_filter_h& filter,
                   const picojson::value& jsf,
                   int filter_op,
                   bool& is_empty) {
  ScopeGuard<contacts_filter_h> filt;
  contacts_filter_h* pfilt = nullptr;
  if (filter_op == CALLH_FILTER_NONE || is_empty) {
    pfilt = &filter;
  } else if (filter_op == CALLH_FILTER_OR || filter_op == CALLH_FILTER_AND) {
    CHK(contacts_filter_create(CALLH_VIEW_URI, &filt));
    pfilt = &filt;
  }

  std::string attr = jsf.get("attributeName").to_str();
  picojson::value start_val = jsf.get("startValue");
  picojson::value end_val = jsf.get("endValue");

  CHK(MapRangeFilter(*pfilt, attr, start_val, end_val));

  if (pfilt != &filter) {
    contacts_filter_operator_e fop = (contacts_filter_operator_e) filter_op;
    CHK(contacts_filter_add_operator(filter, fop));
    CHK(contacts_filter_add_filter(filter, *pfilt));
  }

  is_empty = false;  // 'filter' was changed, from now on add to it
  return CONTACTS_ERROR_NONE;
}

// Determine the type of a JSON filter and dispatch to the right method.
int ParseFilter(contacts_filter_h& filter,
                const picojson::value& js_filter,
                int filter_op,
                bool& is_empty) {
  if (!js_filter.is<picojson::null>()) {  // this check is redundant
    if (js_filter.contains("type")) {
      return AddCompositeFilter(filter, js_filter, filter_op, is_empty);
    } else if (js_filter.contains("matchFlag")) {
      return AddAttributeFilter(filter, js_filter, filter_op, is_empty);
    } else if (js_filter.contains("initialValue")) {
      return AddRangeFilter(filter, js_filter, filter_op, is_empty);
    }
  }
  return CONTACTS_ERROR_NONE;
}

// Prepare JSON for setMessageListener() JS function in callhistory_api.js.
int HandleFindResults(contacts_list_h list, picojson::value::object& resp) {
  int err = CONTACTS_ERROR_DB;
  unsigned int total = 0;

  contacts_list_get_count(list, &total);
  picojson::value::array result;

  for (unsigned int i = 0; i < total; i++) {
    contacts_record_h record = nullptr;
    CHK(contacts_list_get_current_record_p(list, &record));
    if (record) {  // read the fields and create JSON attributes
      picojson::value::object o;
      CHK(SerializeEntry(record, o));
      result.push_back(picojson::value(o));
    }

    err = contacts_list_next(list);  // move the current record
    if (err != CONTACTS_ERROR_NONE && err != CONTACTS_ERROR_NO_DATA) {
      LOG_ERR("HandleFindResults: iterator error");
      return CONTACTS_ERROR_DB;
    }
  }
  resp["result"] = picojson::value(result);  // as used in callhistory_api.js
  return CONTACTS_ERROR_NONE;
}

// Handling database notifications through Contacts API;
// 'changes' is a string, and yes, we need to PARSE it...
void NotifyDatabaseChange(const char* view, char* changes, void* user_data) {
  CallHistoryInstance* chi = static_cast<CallHistoryInstance*>(user_data);
  if (!chi->IsValid()) {
    LOG_ERR("CallHistory: invalid notification callback");
    return;
  }

  picojson::value::object out;  // output JSON object
  picojson::value::array added;  // full records
  picojson::value::array changed;  // full records
  picojson::value::array deleted;  // only id's

  char  delim[] = ",:";
  char* rest;
  char* chtype  = nullptr;
  char* chid    = nullptr;
  int changetype, uid, err = NO_ERROR;
  bool ins = false;

  chtype = strtok_r(changes, delim, &rest);
  while (chtype) {
    changetype = atoi((const char*)chtype);
    chid = strtok_r(nullptr, delim, &rest);
    uid = atoi((const char*)chid);
    switch (changetype) {
      case CONTACTS_CHANGE_INSERTED:
        ins = true;
      case CONTACTS_CHANGE_UPDATED:
        contacts_record_h record = nullptr;
        if (check(contacts_db_get_record(CALLH_VIEW_URI, uid, &record))) {
          picojson::value::object val;
          if (check(SerializeEntry(record, val))) {
            if (ins)
              added.push_back(picojson::value(val));
            else
              changed.push_back(picojson::value(val));
          }
          contacts_record_destroy(record, true);
        }
        break;
      case CONTACTS_CHANGE_DELETED:
        deleted.push_back(JsonFromInt(uid));
        break;
      default:
        LOG_ERR("CallHistory: invalid database change: " << chtype);
        err = DATABASE_ERR;
        break;
    }
    chtype = strtok_r(nullptr, delim, &rest);
  }

  out["cmd"] = picojson::value("notif");
  out["errorCode"] = picojson::value(static_cast<double>(err));
  out["added"]   = picojson::value(added);
  out["changed"] = picojson::value(changed);
  out["deleted"] = picojson::value(deleted);
  picojson::value v(out);

  chi->PostMessage(v.serialize().c_str());
}

}  // namespace

bool CallHistoryInstance::CheckBackend() {
  if (backendConnected_)
    return true;
  if (check(contacts_connect2())) {
    backendConnected_ = true;
    return true;
  }
  return false;
}

bool CallHistoryInstance::ReleaseBackend() {
  if (!backendConnected_)
    return true;  // Already disconnected.
  if (check(contacts_disconnect2()))
    backendConnected_ = false;
  else
    LOG_ERR("ReleaseBackend(): could not close DB");
  return !backendConnected_;
}

// Register a single native listener for all JS ones; dispatch at JS side.
int CallHistoryInstance::HandleAddListener() {
  if (!listenerCount_) {  // Do actual registration only on first request.
    int err = contacts_db_add_changed_cb_with_info(CALLH_VIEW_URI,
                                                   NotifyDatabaseChange,
                                                   this);
    if (check(err))
      listenerCount_++;
    return MapContactErrors(err);
  }
  listenerCount_++;
  return NO_ERROR;
}

int CallHistoryInstance::HandleRemoveListener() {
  if (!listenerCount_) {
    return UNKNOWN_ERR;
  }
  --listenerCount_;
  if (!listenerCount_) {
    return UnregisterListener();
  }
  return NO_ERROR;
}

int CallHistoryInstance::UnregisterListener() {
  int err = contacts_db_remove_changed_cb_with_info(CALLH_VIEW_URI,
                                                    NotifyDatabaseChange,
                                                    this);
  return MapContactErrors(err);
}

// Take a JSON query, translate to contacts query, collect the results, and
// return a JSON string via the callback.
int CallHistoryInstance::HandleFind(const picojson::value& input,
                                    picojson::value::object& reply) {
  int limit = 0;
  IntFromJson(input.get("limit"), &limit);  // no change on error

  int offset = 0;
  IntFromJson(input.get("offset"), &offset);

  // check sort mode
  bool asc = false;
  std::string sortAttr(kStartTime);

  picojson::value sortmode = input.get("sortMode");
  if (!sortmode.is<picojson::null>()) {
    picojson::value sa = sortmode.get("attributeName");
    if (!sa.is<picojson::null>())
      sortAttr = sa.to_str();
    sa = sortmode.get("order");
    if (!sa.is<picojson::null>() && (sa.to_str() == STR_SORT_ASC))
      asc = true;
  }

  // set up filter
  ScopeGuard<contacts_filter_h> filter;
  ScopeGuard<contacts_query_h> query;
  ScopeGuard<contacts_list_h> list;
  picojson::value js_filter = input.get("filter");  // object or null
  if (js_filter.is<picojson::null>()) {
    CHK_MAP(contacts_db_get_all_records(CALLH_VIEW_URI, offset, \
                                          limit, &list));
  } else {
    bool filter_empty = true;
    CHK_MAP(contacts_filter_create(CALLH_VIEW_URI, &filter));
    contacts_filter_h* pfilt = &filter;
    CHK_MAP(ParseFilter(*pfilt, js_filter, CALLH_FILTER_NONE, filter_empty));
    CHK_MAP(contacts_query_create(CALLH_VIEW_URI, &query));
    contacts_query_h* pquery = &query;
    CHK_MAP(contacts_query_set_filter(*pquery, *pfilt));
    CHK_MAP(contacts_query_set_sort(*pquery, MapAttrName(sortAttr), asc));
    CHK_MAP(contacts_db_get_records_with_query(*pquery, offset, limit, &list));
  }
  contacts_list_h* plist = &list;
  CHK_MAP(HandleFindResults(*plist, reply));
  return NO_ERROR;
}

int CallHistoryInstance::HandleRemove(const picojson::value& msg) {
  int uid = -1;

  picojson::value v = msg.get("uid");
  if (!IntFromJson(v, &uid)) {
    return TYPE_MISMATCH_ERR;
  }

  return MapContactErrors(contacts_db_delete_record(CALLH_VIEW_URI, uid));
}

int CallHistoryInstance::HandleRemoveBatch(const picojson::value& msg) {
  picojson::value arr = msg.get("uids");
  if (!arr.is<picojson::array>()) {
    return TYPE_MISMATCH_ERR;
  }

  picojson::array json_arr = arr.get<picojson::array>();
  int id;
  int count = json_arr.size();
  std::unique_ptr<int[]> ids(new int[count]);
  #if !NODEBUG
    std::ostringstream uidlist;
    uidlist.str("");
  #endif

  for (int i = 0, j = 0; i < count; i++) {
      picojson::value v = json_arr[i];
      if (IntFromJson(v, &id)) {
        ids[j++] = id;
        #if !NODEBUG
          uidlist << id << ", ";
        #endif
      } else {
        return TYPE_MISMATCH_ERR;
      }
  }

  int err = contacts_db_delete_records(CALLH_VIEW_URI, ids, count);
  return MapContactErrors(err);
}

// Tizen Contacts server API doesn't expose any method for removing all
// elements in one operation. Need to fetch all the records, fetch id's
// and remove them one by one, or in batches
int CallHistoryInstance::HandleRemoveAll(const picojson::value& msg) {
  contacts_record_h rec  = nullptr;  // should not be destroyed by us;
  ScopeGuard<contacts_list_h> list;
  contacts_list_h *plist;
  int limit = 200;  // initial batch size
  unsigned int count, i, j;
  #if !NODEBUG
    std::ostringstream uidlist;
    uidlist.str("");
  #endif

  do {  // remove batches until done
    CHK_MAP(contacts_list_create(&list));
    CHK_MAP(contacts_db_get_all_records(CALLH_VIEW_URI, 0, limit, &list));
    plist = &list;
    CHK_MAP(contacts_list_get_count(*plist, &count));
    if (!count) {
      return NO_ERROR;
    }

    // now we have an array of id's to be removed
    std::unique_ptr<int[]> ids(new int[count]);
    CHK_MAP(contacts_record_create(CALLH_VIEW_URI, &rec));
    CHK_MAP(contacts_list_first(*plist));

    for (i = 0, j = 0; i < count; i++) {
      CHK_MAP(contacts_list_get_current_record_p(*plist, &rec));
      int id = -1;  // fetch the id from each record
      CHK_MAP(contacts_record_get_int(rec, CALLH_ATTR_UID , &id));
      if (id >= 0) {
        ids[j++] = id;
        #if !NODEBUG
          uidlist << id << ", ";
        #endif
      }
      if (!check(contacts_list_next(*plist)))
        break;
    }
    CHK_MAP(contacts_db_delete_records(CALLH_VIEW_URI, ids, j));
  } while (count == static_cast<unsigned int>(limit));
  return NO_ERROR;
}
