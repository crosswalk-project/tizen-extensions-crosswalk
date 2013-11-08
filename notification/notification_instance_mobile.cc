// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification/notification_instance_mobile.h"

#include <iostream>
#include <vector>
#include "common/picojson.h"
#include "notification/notification_parameters.h"
#include "notification/picojson_helpers.h"

namespace {
const unsigned kMaxThumbnailLength = 4;

void NotificationSetText(notification_h notification,
                         notification_text_type_e type,
                         const std::string& text) {
  notification_set_text(
      notification, type, text.c_str(), NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
}

bool SetImage(notification_h n, notification_image_type_e type,
              const std::string& imagePath) {
  char* oldImgPath = NULL;
  if (notification_get_image(n, type, &oldImgPath) != NOTIFICATION_ERROR_NONE)
    return false;

  if (oldImgPath && imagePath == oldImgPath)
      return true;

  if (notification_set_image(n, type, imagePath.c_str())
      != NOTIFICATION_ERROR_NONE)
    return false;
  return true;
}

bool SetThumbnail(notification_h n, const std::vector<std::string>& thumbs) {
  const notification_image_type_e thumbnailList[] = {
    NOTIFICATION_IMAGE_TYPE_LIST_1,
    NOTIFICATION_IMAGE_TYPE_LIST_2,
    NOTIFICATION_IMAGE_TYPE_LIST_3,
    NOTIFICATION_IMAGE_TYPE_LIST_4 };

  unsigned thumbsLength = thumbs.size();
  if (thumbsLength > kMaxThumbnailLength)
    thumbsLength = kMaxThumbnailLength;

  for (unsigned i = 0; i < thumbsLength; i++) {
    char* oldThumb = NULL;
    if (notification_get_image(n, thumbnailList[i], &oldThumb)
        != NOTIFICATION_ERROR_NONE)
      return false;

    if (oldThumb && thumbs[i] == oldThumb)
        continue;

    if (notification_set_image(n, thumbnailList[i], thumbs[i].c_str())
        != NOTIFICATION_ERROR_NONE)
      return false;
  }
  return true;
}

bool FillNotificationHandle(notification_h n, const NotificationParameters& p) {
  NotificationSetText(n, NOTIFICATION_TEXT_TYPE_TITLE, p.title);
  NotificationSetText(n, NOTIFICATION_TEXT_TYPE_CONTENT, p.content);

  if (!p.icon_path.empty()) {
    if (!SetImage(n, NOTIFICATION_IMAGE_TYPE_ICON, p.icon_path))
      return false;
  }

  if (p.status_type == "PROGRESS") {
    if (p.progress_type == "PERCENTAGE") {
      notification_set_progress(n, p.progress_value / 100.0);
    } else if (p.progress_type == "BYTE") {
      notification_set_size(n, p.progress_value);
    }
  }

  if (!p.sub_icon_path.empty()) {
    if (!SetImage(n, NOTIFICATION_IMAGE_TYPE_ICON_SUB, p.sub_icon_path))
      return false;
  }

  if (!p.background_image_path.empty()) {
    if (!SetImage(n, NOTIFICATION_IMAGE_TYPE_BACKGROUND,
                  p.background_image_path))
      return false;
  }

  if (!p.thumbnails.empty()) {
    if (!SetThumbnail(n, p.thumbnails))
      return false;
  }

  return true;
}

const char kSerializedNull[] = "null";

}  // namespace

NotificationInstanceMobile::NotificationInstanceMobile(
    NotificationManager* manager)
    : manager_(manager) {}

NotificationInstanceMobile::~NotificationInstanceMobile() {
  manager_->DetachClient(this);
}

void NotificationInstanceMobile::HandleMessage(const char* message) {
  picojson::value v = ParseJSONMessage(message);
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "NotificationRemove")
    HandleRemove(v);
  else
    std::cerr << "Notification: received invalid command '" << cmd << "'\n";
}

void NotificationInstanceMobile::HandleSyncMessage(const char* message) {
  picojson::value v = ParseJSONMessage(message);
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "NotificationPost")
    HandlePost(v);
  else if (cmd == "NotificationUpdate")
    HandleUpdate(v);
  else
    std::cerr << "Notification: received invalid command '" << cmd << "'\n";
}

void NotificationInstanceMobile::HandlePost(const picojson::value& msg) {
  NotificationParameters params = ReadNotificationParameters(msg);

  notification_type_e type;
  if (params.status_type == "PROGRESS" || params.status_type == "ONGOING")
    type = NOTIFICATION_TYPE_ONGOING;
  else
    type = NOTIFICATION_TYPE_NOTI;

  notification_h notification = manager_->CreateNotification(type);
  if (!FillNotificationHandle(notification, params)) {
    SendSyncReply(kSerializedNull);
    return;
  }

  int id = manager_->PostNotification(notification, this);
  if (!id) {
    SendSyncReply(kSerializedNull);
    return;
  }

  SendSyncReply(JSONValueFromInt(id).serialize().c_str());
}

void NotificationInstanceMobile::HandleRemove(const picojson::value& msg) {
  int id;
  if (GetIntFromJSONValue(msg.get("id"), &id))
    return;
  if (!manager_->RemoveNotification(id)) {
    std::cerr << "tizen.notification error: "
              << "couldn't remove notification with id '" << id << "'\n";
  }
}

void NotificationInstanceMobile::HandleUpdate(const picojson::value& msg) {
  int id;
  if (!GetIntFromJSONValue(msg.get("id"), &id)) {
    SendSyncReply(kSerializedNull);
    return;
  }

  notification_h notification = manager_->GetNotification(id);
  if (!notification) {
    SendSyncReply(kSerializedNull);
    return;
  }

  FillNotificationHandle(notification, ReadNotificationParameters(msg));

  picojson::value result;
  if (manager_->UpdateNotification(notification))
    result = picojson::value(true);
  SendSyncReply(result.serialize().c_str());
}

void NotificationInstanceMobile::OnNotificationRemoved(int id) {
  picojson::value::object o;
  o["cmd"] = picojson::value("NotificationRemoved");
  o["id"] = JSONValueFromInt(id);
  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

