// Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "datasync/datasync_instance.h"

#include "datasync/datasync_log.h"
#include "datasync/datasync_serialization.h"

#include "tizen/tizen.h"

namespace datasync {

DatasyncInstance::DatasyncInstance(DatasyncExtension& extension)
    : extension_(extension) {
}

DatasyncInstance::~DatasyncInstance() {
  extension_.manager().UnregisterInstanceCallbacks(this);
}

void DatasyncInstance::HandleMessage(const char* msg) {
  // this is stub, no async messages
  LogError("No handling of async message: " << msg);
}

void DatasyncInstance::HandleSyncMessage(const char* msg) {
  LogDebug("HandleSyncMessage: " << msg);
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    LogError("Ignoring message");
    return;
  }

  int callback_id = -1;
  std::string cmd = v.get("cmd").to_str();
  picojson::value arg = v.get("arg");

  LogDebug("cmd:" << cmd << " arg: " << arg.to_str());

  if (v.contains("callback_key")) {
    callback_id = static_cast<int>(v.get("callback_key").get<double>());
  }

  if (cmd == "getMaxProfilesNum") {
    HandleGetMaxProfilesNum(arg);
  } else if (cmd == "getProfilesNum") {
    HandleGetProfilesNum(arg);
  } else if (cmd == "get") {
    HandleGet(arg);
  } else if (cmd == "getAll") {
    HandleGetAll(arg);
  } else if (cmd == "getLastSyncStatistics") {
    HandleGetLastSyncStatistics(arg);
  } else if (cmd == "add") {
    HandleAdd(arg);
  } else if (cmd == "update") {
    HandleUpdate(arg);
  } else if (cmd == "remove") {
    HandleRemove(arg);
  } else if (cmd == "startSync") {
    HandleStartSync(arg, callback_id);
  } else if (cmd == "stopSync") {
    HandleStopSync(arg);
  } else {
    LogError("Unknown command");
  }
}

void DatasyncInstance::ReplySyncAnswer(const picojson::value& value) {
  picojson::object answer;
  answer["answer"] = value;

  LogDebug("ReplySyncAnswer: " << picojson::value(answer).serialize().c_str());

  SendSyncReply(picojson::value(answer).serialize().c_str());
}

void DatasyncInstance::ReplySyncUndefinedAnswer() {
  picojson::object answer;
  std::string message = picojson::value(answer).serialize();
  LogDebug("ReplySyncUndefinedAnswer: " << message.c_str());

  SendSyncReply(message.c_str());
}

void DatasyncInstance::ReplySyncException(unsigned code,
    const std::string& name,
    const std::string& message) {
  picojson::object exception;
  picojson::object error;
  error["code"] = picojson::value(static_cast<double>(code));
  error["name"] = picojson::value(name);
  error["message"] = picojson::value(message);
  exception["exception"] = picojson::value(error);

  std::string ret = picojson::value(exception).serialize();
  LogDebug("ReplySyncException: " << ret.c_str());

  SendSyncReply(ret.c_str());
}

void DatasyncInstance::MakeExceptionAndReply(const Error& e) {
  ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, e.name(), e.message());
}

void DatasyncInstance::ReplyAsyncAnswer(int key, const std::string& name,
    const picojson::value& ret) {
  picojson::object toSend;
  toSend["callback_key"] = picojson::value(static_cast<double>(key));
  toSend["callback_name"] = picojson::value(name);
  toSend["answer"] = ret;

  LogDebug("ReplyAsync: " << picojson::value(toSend).serialize().c_str());

  PostMessage(picojson::value(toSend).serialize().c_str());
}

void DatasyncInstance::ReplyAsyncOnCompleted(int key,
    const std::string& profile_id) {
  picojson::object ret;
  ret["profileId"] = picojson::value(profile_id);
  ReplyAsyncAnswer(key, "oncompleted", picojson::value(ret));
}

void DatasyncInstance::ReplyAsyncOnStopped(int key,
    const std::string& profile_id) {
  picojson::object ret;
  ret["profileId"] = picojson::value(profile_id);
  ReplyAsyncAnswer(key, "onstopped", picojson::value(ret));
}

void DatasyncInstance::ReplyAsyncOnFailed(int key,
    const std::string& profile_id,
    int error_code,
    const std::string& name,
    const std::string& message) {
  picojson::object ret;
  picojson::object error;

  error["code"] = picojson::value(static_cast<double>(error_code));
  error["name"] = picojson::value(name);
  error["message"] = picojson::value(message);

  ret["error"] = picojson::value(error);
  ret["profileId"] = picojson::value(profile_id);

  ReplyAsyncAnswer(key, "onfailed", picojson::value(ret));
}

void DatasyncInstance::ReplyAsyncOnProgress(int key,
    const std::string& profile_id,
    bool is_from_server,
    int synced_per_db,
    int total_per_db,
    SyncServiceInfo::SyncServiceType type) {
  picojson::object ret;
  ret["profileId"] = picojson::value(profile_id);

  ret["isFromServer"] = picojson::value(is_from_server);
  ret["syncedPerService"] = picojson::value(static_cast<double>(synced_per_db));
  ret["totalPerService"] = picojson::value(static_cast<double>(total_per_db));

  ret["serviceType"] =
      picojson::value(SyncServiceInfo::SyncServiceTypeToString(type));

  ReplyAsyncAnswer(key, "onprogress", picojson::value(ret));
}

void DatasyncInstance::HandleGetMaxProfilesNum(const picojson::value&) {
  extension_.manager().GetMaxProfilesNum().Success([this](unsigned max){
    ReplySyncAnswer(serialization::ToJson(max));
  });
}

void DatasyncInstance::HandleGetProfilesNum(const picojson::value&) {
  extension_.manager().GetProfilesNum().Success([this](unsigned num) {
    ReplySyncAnswer(serialization::ToJson(num));
  }).Failure([this](const Error& e) {
    MakeExceptionAndReply(e);
  });
}

void DatasyncInstance::HandleGet(const picojson::value& arg) {
  std::unique_ptr<std::string> profile_id =
      serialization::FromJson<std::string>(arg);
  if (profile_id) {
    extension_.manager().Get(*profile_id).Success(
        [this](SyncProfileInfoPtr profileInfo) {
      ReplySyncAnswer(serialization::ToJson(*profileInfo));
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during SyncProfile Construction");
  }
}

void DatasyncInstance::HandleGetAll(const picojson::value&) {
  extension_.manager().GetAll()
      .Success([this](SyncProfileInfoListPtr profileInfoList) {
    picojson::array array;

    for (const auto& element : *profileInfoList) {
      array.push_back(serialization::ToJson(*element));
    }

    ReplySyncAnswer(serialization::ToJson(array));
  }).Failure([this](const Error& e) {
    MakeExceptionAndReply(e);
  });
}

void DatasyncInstance::HandleGetLastSyncStatistics(const picojson::value& arg) {
  std::unique_ptr<std::string> profile_id =
      serialization::FromJson<std::string>(arg);
  if (profile_id) {
    extension_.manager().GetLastSyncStatistics(*profile_id).Success(
        [this](SyncStatisticsListPtr statisticsList) {
      picojson::array array;

      for (const auto& element : *statisticsList) {
        array.push_back(serialization::ToJson(*element));
      }

      ReplySyncAnswer(serialization::ToJson(array));
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

void DatasyncInstance::HandleAdd(const picojson::value& arg) {
  std::unique_ptr<SyncProfileInfo> profile =
      serialization::FromJson<SyncProfileInfo>(arg);
  if (profile) {
    extension_.manager().Add(*profile).Success([this](std::string id) {
      ReplySyncAnswer(serialization::ToJson(id));
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

void DatasyncInstance::HandleUpdate(const picojson::value& arg) {
  std::unique_ptr<SyncProfileInfo> profile =
      serialization::FromJson<SyncProfileInfo>(arg);
  if (profile) {
    extension_.manager().Update(*profile).Success([this]() {
      ReplySyncUndefinedAnswer();
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

void DatasyncInstance::HandleRemove(const picojson::value& arg) {
  std::unique_ptr<std::string> profile_id =
      serialization::FromJson<std::string>(arg);
  if (profile_id) {
    extension_.manager().Remove(*profile_id).Success([this]() {
      ReplySyncUndefinedAnswer();
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

void DatasyncInstance::HandleStartSync(
    const picojson::value& arg, int callback_id) {
  std::unique_ptr<std::string> profile_id =
      serialization::FromJson<std::string>(arg);
  if (profile_id) {
    extension_.manager().StartSync(*profile_id, callback_id, this).Success(
        [this]() {
      ReplySyncUndefinedAnswer();
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

void DatasyncInstance::HandleStopSync(const picojson::value& arg) {
  std::unique_ptr<std::string> profile_id =
      serialization::FromJson<std::string>(arg);
  if (profile_id) {
    extension_.manager().StopSync(*profile_id).Success([this]() {
      ReplySyncUndefinedAnswer();
    }).Failure([this](const Error& e) {
      MakeExceptionAndReply(e);
    });
  } else {
    ReplySyncException(WebApiAPIErrors::UNKNOWN_ERR, "PlatformException",
                       "Error during object construction");
  }
}

}  // namespace datasync
