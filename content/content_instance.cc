// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/content_instance.h"

#include <media_content.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <string>
#include "common/picojson.h"

namespace {
std::string pathToURI(const std::string path) {
  static std::string scheme("file://");

  return scheme + path;
}
}  // namespace

unsigned ContentInstance::m_instanceCount = 0;

ContentInstance::ContentInstance() {
  if (media_content_connect() != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_content_connect: error\n";
    return;
  }
  ++m_instanceCount;
}

ContentInstance::~ContentInstance() {
  assert(m_instanceCount > 0);
  if (--m_instanceCount > 0)
    return;
  if (media_content_disconnect() != MEDIA_CONTENT_ERROR_NONE)
    std::cerr << "media_discontent_connect: error\n";
}

void ContentInstance::HandleMessage(const char* message) {
  picojson::value v;
  picojson::value::object o;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring message.\n";
    return;
  }
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "ContentManager.getDirectories") {
    HandleGetDirectoriesRequest(v);
  } else if (cmd == "ContentManager.find") {
    HandleFindRequest(v);
  } else {
    std::cerr << "Message " + cmd + " is not supported.\n";
  }
}

void ContentInstance::PostAsyncErrorReply(const picojson::value& msg,
    WebApiAPIErrors error_code) {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_code));
  o["replyId"] = picojson::value(msg.get("replyId").get<double>());

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

void ContentInstance::PostAsyncSuccessReply(const picojson::value& msg,
    picojson::value::object& reply) {
  reply["isError"] = picojson::value(false);
  reply["replyId"] = picojson::value(msg.get("replyId").get<double>());

  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void ContentInstance::PostAsyncSuccessReply(const picojson::value& msg) {
  picojson::value::object reply;
  PostAsyncSuccessReply(msg, reply);
}

void ContentInstance::PostAsyncSuccessReply(const picojson::value& msg,
    picojson::value& value) {
  picojson::value::object reply;
  reply["value"] = value;
  PostAsyncSuccessReply(msg, reply);
}

void ContentInstance::HandleSyncMessage(const char* message) {
}

void ContentInstance::HandleGetDirectoriesRequest(const picojson::value& msg) {
  ContentFolderList folderList;
  if (media_folder_foreach_folder_from_db(NULL,
          mediaFolderCallback,
          reinterpret_cast<void*>(&folderList)) != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_folder_get_folder_count_from_db: error\n";
  } else {
    HandleGetDirectoriesReply(msg, &folderList);
  }
}

void ContentInstance::HandleGetDirectoriesReply(const picojson::value& msg,
    ContentFolderList* folderList) {
  const std::vector<ContentFolder*>& results = folderList->getAllItems();
  picojson::value::array folders;

  for (unsigned i = 0; i < results.size(); i++) {
    ContentFolder* folder = results[i];

    picojson::value::object o;

    o["id"] = picojson::value(folder->id());
    o["directoryURI"] = picojson::value(folder->directoryURI());
    o["title"] = picojson::value(folder->title());
    o["storageType"] = picojson::value(folder->storageType());
    o["modifiedDate"] =
      picojson::value(static_cast<double>(folder->modifiedDate()));

    folders.push_back(picojson::value(o));
  }
  picojson::value value(folders);
  PostAsyncSuccessReply(msg, value);
}

bool ContentInstance::mediaFolderCallback(media_folder_h handle,
    void* user_data) {
  if (!user_data)
    return false;

  ContentFolderList* folderList =
    reinterpret_cast<ContentFolderList*>(user_data);

  ContentFolder* folder = new ContentFolder;
  folder->init(handle);
  folderList->addFolder(folder);
#ifdef DEBUG
  folder->print();
#endif
  return true;
}

void ContentInstance::HandleFindRequest(const picojson::value& msg) {
  ContentItemList itemList;
  if (media_info_foreach_media_from_db(NULL,
      mediaInfoCallback,
      reinterpret_cast<ContentFolderList*>(&itemList))
      != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_foreach_media_from_db: error\n";
  } else {
    HandleFindReply(msg, &itemList);
  }
}

void ContentInstance::HandleFindReply(
    const picojson::value& msg,
    ContentItemList* itemList) {
  const std::vector<ContentItem*> &results = itemList->getAllItems();

  picojson::value::array items;

  for (unsigned i = 0; i < results.size(); i++) {
    ContentItem* item = results[i];

    picojson::value::object o;

    o["id"] = picojson::value(item->id());
    o["name"] = picojson::value(item->name());
    o["type"] = picojson::value(item->type());
    o["mimeType"] = picojson::value(item->mimeType());
    o["title"] = picojson::value(item->title());
    o["contentURI"] = picojson::value(item->contentURI());
    o["thumbnailURIs"] = picojson::value(item->thumbnailURIs());
    o["releaseDate"] =
      picojson::value(static_cast<double>(item->releaseDate()));
    o["modifiedDate"] =
      picojson::value(static_cast<double>(item->modifiedDate()));
    o["size"] = picojson::value(static_cast<double>(item->size()));
    o["description"] = picojson::value(item->description());
    o["rating"] = picojson::value(static_cast<double>(item->rating()));

    items.push_back(picojson::value(o));
  }
  picojson::value value(items);
  PostAsyncSuccessReply(msg, value);
}

bool ContentInstance::mediaInfoCallback(media_info_h handle, void* user_data) {
  if (!user_data)
    return false;

  ContentItemList* itemList = reinterpret_cast<ContentItemList*>(user_data);

  ContentItem* item = new ContentItem;
  item->init(handle);
  itemList->addItem(item);
#ifdef DEBUG
  item->print();
#endif
  return true;
}

void ContentFolder::init(media_folder_h handle) {
  char* str = NULL;
  time_t date;
  media_content_storage_e storageType;

  if (media_folder_get_folder_id(handle, &str) == MEDIA_CONTENT_ERROR_NONE) {
    setID(str);
    free(str);
  }

  if (media_folder_get_path(handle, &str) == MEDIA_CONTENT_ERROR_NONE) {
    setDirectoryURI(pathToURI(str));
    free(str);
  }

  if (media_folder_get_name(handle, &str) == MEDIA_CONTENT_ERROR_NONE) {
    setTitle(str);
    free(str);
  }

  if (media_folder_get_storage_type(
      handle, &storageType) == MEDIA_CONTENT_ERROR_NONE) {
    std::string type;
    if (storageType == MEDIA_CONTENT_STORAGE_INTERNAL) {
      type = "INTERNAL";
    } else if (storageType == MEDIA_CONTENT_STORAGE_EXTERNAL) {
      type = "EXTERNAL";
    } else {
      type = "UNKNOWN";
    }
    setStorageType(type);
  }

  if (media_folder_get_modified_time(
      handle, &date) == MEDIA_CONTENT_ERROR_NONE) {
    setModifiedDate(date);
  }
}

#ifdef DEBUG
void ContentFolder::print(void) {
  std::cout << "ID: " << getID() << std::endl;
  std::cout << "URI: " << getDirectoryURI() << std::endl;
  std::cout << "Title: " << getTitle() << std::endl;
  std::cout << "Type: " << getStorageType() << std::endl;
  char pc[26];
  time_t time = getModifiedDate();
  std::cout << "Date: " << ctime_r(&time, pc) << std::endl;
}
#endif

void ContentItem::init(media_info_h handle) {
  char* pc = NULL;

  // NOTE: the Tizen CAPI media_info_* functions assumes
  // the caller frees the char**. The CAPI can also return NULL char**
  // even the return code is MEDIA_CONTENT_ERROR_NONE.

  if (media_info_get_media_id(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setID(pc);
    free(pc);
  }

  if (media_info_get_mime_type(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setMimeType(pc);
    free(pc);
  }

  if (media_info_get_title(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setTitle(pc);
    free(pc);
  }

  if (media_info_get_display_name(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setName(pc);
    free(pc);
  }

  if (media_info_get_file_path(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setContentURI(pathToURI(pc));
    free(pc);
  }

  if (media_info_get_thumbnail_path(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setThumbnailURIs(pc);
    free(pc);
  }

  if (media_info_get_description(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setDescription(pc);
    free(pc);
  }

  time_t date;
  if (media_info_get_modified_time(handle, &date) == MEDIA_CONTENT_ERROR_NONE) {
    setModifiedDate(date);
  }

  int i = 0;
  if (media_info_get_rating(handle, &i) == MEDIA_CONTENT_ERROR_NONE)
    setRating(i);

  unsigned long long ll; // NOLINT
  if (media_info_get_size(handle, &ll) == MEDIA_CONTENT_ERROR_NONE)
    setSize(ll);

  media_content_type_e type;
  if (media_info_get_media_type(handle, &type) == MEDIA_CONTENT_ERROR_NONE) {
    if (type == MEDIA_CONTENT_TYPE_IMAGE)
      setType("IMAGE");
    else if (type == MEDIA_CONTENT_TYPE_VIDEO)
      setType("VIDEO");
    else if (type == MEDIA_CONTENT_TYPE_MUSIC)
      setType("AUDIO");
    else if (type == MEDIA_CONTENT_TYPE_OTHERS)
      setType("OTHER");
  }
}

#ifdef DEBUG
void ContentItem::print(void) {
  std::cout << "----" << std::endl;
  std::cout << "ID: " << getID() << std::endl;
  std::cout << "Name: " << getName() << std::endl;
  std::cout << "Type: " << getType() << std::endl;
  std::cout << "MIME: " << getMimeType() << std::endl;
  std::cout << "Title: " << getTitle() << std::endl;
  std::cout << "URI: " << getContentURI() << std::endl;
  std::cout << "ThumbnailURIs: " << getThumbnailURIs() << std::endl;
  char pc[26];
  time_t time = getModifiedDate();
  std::cout << "Modified: " << ctime_r(&time, pc);
  std::cout << "Size: " << getSize() << std::endl;
  std::cout << "Description: " << getDescription() << std::endl;
  std::cout << "Rating: " << getRating() << std::endl;
}
#endif
