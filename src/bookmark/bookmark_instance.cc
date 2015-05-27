// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "bookmark/bookmark_instance.h"
#include "common/picojson.h"

struct Context {
  int id;
  bool shouldGetItems;
  std::vector<favorites_bookmark_entry_s*> folders;
};

BookmarkInstance::BookmarkInstance() {}

BookmarkInstance::~BookmarkInstance() {}

void BookmarkInstance::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty())
    return;

  picojson::value::object o;
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "AddBookmark")
    o = HandleAddBookmark(v);
  else if (cmd == "GetFolder")
    o = HandleGetFolder(v, false);
  else if (cmd == "GetFolderItems")
    o = HandleGetFolder(v, true);
  else if (cmd == "RemoveBookmark")
    o = HandleRemoveBookmark(v);
  else if (cmd == "RemoveAll")
    o = HandleRemoveAll(v);
  else if (cmd == "GetRootID")
    o = HandleGetRootID(v);

  if (o.empty())
    o["error"] = picojson::value(true);

  SetSyncReply(picojson::value(o));
}

void BookmarkInstance::HandleMessage(const char* message) {}

void BookmarkInstance::SetSyncReply(picojson::value v) {
  SendSyncReply(v.serialize().c_str());
}

static bool bookmark_foreach_get_cb(favorites_bookmark_entry_s* item,
                                    void* user_data) {
  if (!user_data)
    return true;

  Context* ctx = reinterpret_cast<Context*>(user_data);

  if ((ctx->shouldGetItems && item->folder_id != ctx->id) ||
     (!ctx->shouldGetItems && item->id != ctx->id))
    return true;

  favorites_bookmark_entry_s* item_cp = new favorites_bookmark_entry_s();
  memcpy(item_cp, item, sizeof(favorites_bookmark_entry_s));

  item_cp->title = reinterpret_cast<char*>(::operator new(PATH_MAX));
  snprintf(item_cp->title, PATH_MAX, "%s", item->title);

  if (!item->is_folder) {
    item_cp->address = reinterpret_cast<char*>(::operator new(PATH_MAX));
    snprintf(item_cp->address, PATH_MAX, "%s", item->address);
  }

  ctx->folders.push_back(item_cp);

  return true;
}

const picojson::value::object BookmarkInstance::HandleAddBookmark(
  const picojson::value& msg) {
  picojson::value::object o;
  int saved_id = 0;

  std::string title = msg.get("title").to_str();
  std::string url = msg.get("url").to_str();
  int parent_id = msg.get("parent_id").get<double>();
  int type = msg.get("type").get<double>();

  if (favorites_bookmark_add(title.c_str(), url.c_str(),
                             parent_id, type, &saved_id) == 0)
    o["id"] = picojson::value(std::to_string(saved_id));

  return o;
}

const picojson::value::object BookmarkInstance::HandleGetFolder(
  const picojson::value& msg, bool shouldGetItems) {
  picojson::value::object o;

  Context ctx = {0};

  ctx.shouldGetItems = shouldGetItems;
  ctx.id = msg.get("id").get<double>();

  if (favorites_bookmark_foreach(bookmark_foreach_get_cb, &ctx) < 0)
    return o;

  picojson::value::array a;
  std::vector<favorites_bookmark_entry_s*>::iterator it;
  for (it = ctx.folders.begin(); it != ctx.folders.end(); ++it) {
    picojson::value::object obj;
    const favorites_bookmark_entry_s* entry = *it;

    obj["title"] = picojson::value(entry->title);
    obj["id"] = picojson::value(std::to_string(entry->id));
    obj["is_folder"] = picojson::value(std::to_string(entry->is_folder));
    obj["folder_id"] = picojson::value(std::to_string(entry->folder_id));

    if (!entry->is_folder) {
      obj["address"] = picojson::value(entry->address);
      delete(entry->address);
    }

    delete(entry->title);
    delete(entry);

    a.push_back(picojson::value(obj));
  }

  o["value"] = picojson::value(a);
  return o;
}

const picojson::value::object BookmarkInstance::HandleRemoveBookmark(
  const picojson::value& msg) {
  picojson::value::object o;

  int id = msg.get("id").get<double>();

  if (favorites_bookmark_delete_bookmark(id) == 0)
    o["value"] = picojson::value(true);

  return o;
}

const picojson::value::object BookmarkInstance::HandleRemoveAll(
  const picojson::value& msg) {
  picojson::value::object o;

  if (favorites_bookmark_delete_all_bookmarks() == 0)
    o["value"] = picojson::value(true);

  return o;
}

const picojson::value::object BookmarkInstance::HandleGetRootID(
  const picojson::value& msg) {
  picojson::value::object o;

  int rootID;
  if (favorites_bookmark_get_root_folder_id(&rootID) == 0)
    o["value"] = picojson::value(std::to_string(rootID));

  return o;
}
