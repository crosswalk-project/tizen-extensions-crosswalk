// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CONTENT_INSTANCE_H_
#define CONTENT_CONTENT_INSTANCE_H_

#include <media_content.h>

#include <string>
#include <algorithm>
#include <vector>

#include "common/extension.h"
#include "common/picojson.h"
#include "tizen/tizen.h"

namespace picojson {
class value;
}

class ContentFolderList;
class ContentItemList;

class ContentInstance : public common::Instance {
 public:
  ContentInstance();
  virtual ~ContentInstance();

 private:
  // common::Instance implementation.
  virtual void HandleMessage(const char* msg);
  virtual void HandleSyncMessage(const char* msg);

  void HandleGetDirectoriesRequest(const picojson::value& json);
  void HandleGetDirectoriesReply(const picojson::value& json,
    ContentFolderList *);
  void HandleFindRequest(const picojson::value& json);
  void HandleFindReply(const picojson::value& json, ContentItemList *);

  // Asynchronous message helpers
  void PostAsyncErrorReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value::object&);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value&);
  void PostAsyncSuccessReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&);

  // Tizen CAPI helpers
  static bool mediaFolderCallback(media_folder_h handle, void *user_data);
  static bool mediaInfoCallback(media_info_h handle, void *user_data);

  static unsigned m_instanceCount;
};

class ContentFolder {
 public:
  ContentFolder() {
    modifiedDate_ = { 0 };
  }
  ~ContentFolder() {}

  void init(media_folder_h handle);

  // Getters & Getters
  std::string id() const { return id_; }
  void setID(const std::string& id) { id_ = id; }
  std::string directoryURI() const { return directoryURI_; }
  void setDirectoryURI(const std::string& uri) { directoryURI_ = uri; }
  std::string title() const { return title_; }
  void setTitle(const std::string& title) { title_ = title; }
  std::string storageType() const { return storageType_; }
  void setStorageType(const std::string& type) { storageType_ = type; }
  time_t modifiedDate() const { return modifiedDate_; }
  void setModifiedDate(time_t modifiedDate) { modifiedDate_ = modifiedDate; }

#ifdef DEBUG
  void print(void);
#endif

 protected:
  std::string id_;
  std::string directoryURI_;
  std::string title_;
  std::string storageType_;
  time_t modifiedDate_;
};

class ContentItem {
 public:
  ContentItem() {
    releaseDate_ = { 0 };
    modifiedDate_ = { 0 };
    size_ = 0;
    rating_ = 0;
  }
  ~ContentItem() {}

  void init(media_info_h handle);

  // Getters & Setters
  std::string id() const { return id_; }
  void setID(const std::string& id) { id_ = id; }
  const std::string name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  std::string type() const { return type_; }
  void setType(const std::string& type) { type_ = type;}
  std::string mimeType() const { return mimeType_; }
  void setMimeType(const std::string& mimeType) { mimeType_ = mimeType; }
  std::string title() const { return title_; }
  void setTitle(const std::string& title) { title_ = title;}
  std::string contentURI() const { return contentURI_; }
  void setContentURI(const std::string& uri) { contentURI_ = uri; }
  std::string thumbnailURIs() const { return thumbnailURIs_; }
  void setThumbnailURIs(const std::string& uris) { thumbnailURIs_ = uris; }
  time_t releaseDate() const { return releaseDate_; }
  void setReleaseDate(time_t releaseDate) { releaseDate_ = releaseDate; }
  time_t modifiedDate() const { return modifiedDate_; }
  void setModifiedDate(time_t modifiedDate) { modifiedDate_ = modifiedDate; }
  uint64_t size() const { return size_; }
  void setSize(const uint64_t size) { size_ = size; }
  std::string description() const { return description_; }
  void setDescription(const std::string& desc) { description_ = desc; }
  uint64_t rating() const { return rating_; }
  void setRating(uint64_t rating) { rating_ = rating; }

#ifdef DEBUG
  void print(void);
#endif

 protected:
  std::string id_;
  std::string name_;
  std::string type_;
  std::string mimeType_;
  std::string title_;
  std::string contentURI_;
  std::string thumbnailURIs_;
  time_t releaseDate_;
  time_t modifiedDate_;
  uint64_t size_;
  std::string description_;
  uint64_t rating_;
};

class ContentFolderList {
 public:
  ~ContentFolderList() {
    for (unsigned i = 0; i < m_folders.size(); i++)
      delete m_folders[i];
  }
  void addFolder(ContentFolder* folder) {
    m_folders.push_back(folder);
  }
  const std::vector<ContentFolder*>& getAllItems() {
    return m_folders;
  }

 private:
  std::vector<ContentFolder*> m_folders;
};

class ContentItemList {
 public:
  ~ContentItemList() {
    for (unsigned i = 0; i < m_items.size(); i++)
      delete m_items[i];
  }
  void addItem(ContentItem* item) {
    m_items.push_back(item);
  }
  const std::vector<ContentItem*>& getAllItems() {
    return m_items;
  }

 private:
  std::vector<ContentItem*> m_items;
};

#endif  // CONTENT_CONTENT_INSTANCE_H_
