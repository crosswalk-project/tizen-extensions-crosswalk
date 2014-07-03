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

  bool HandleUpdateRequest(const picojson::value& json);
  void HandleGetDirectoriesRequest(const picojson::value& json);
  void HandleGetDirectoriesReply(const picojson::value& json,
    ContentFolderList *);
  void HandleFindRequest(const picojson::value& json);
  void HandleFindReply(const picojson::value& json, ContentItemList *);
  void HandleScanFileRequest(const picojson::value& json);
  void HandleScanFileReply(const picojson::value& json);

  // Asynchronous message helpers
  void PostAsyncErrorReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value::object&);
  void PostAsyncSuccessReply(const picojson::value&, picojson::value&);
  void PostAsyncSuccessReply(const picojson::value&, WebApiAPIErrors);
  void PostAsyncSuccessReply(const picojson::value&);

  // Tizen CAPI helpers
  static bool MediaFolderCallback(media_folder_h handle, void *user_data);
  static bool MediaInfoCallback(media_info_h handle, void *user_data);
  static void MediaContentChangeCallback(
      media_content_error_e error,
      int pid,
      media_content_db_update_item_type_e update_item,
      media_content_db_update_type_e update_type,
      media_content_type_e media_type,
      char* uuid,
      char* path,
      char* mime_type,
      void* user_data);

  static unsigned m_instanceCount;
};

class ContentFolder {
 public:
  void init(media_folder_h handle);

  // Getters & Getters
  const std::string& id() const { return id_; }
  void setID(const std::string& id) { id_ = id; }
  const std::string& directoryURI() const { return directoryURI_; }
  void setDirectoryURI(const std::string& uri) { directoryURI_ = uri; }
  const std::string& title() const { return title_; }
  void setTitle(const std::string& title) { title_ = title; }
  const std::string& storageType() const { return storageType_; }
  void setStorageType(const std::string& type) { storageType_ = type; }
  const std::string& modifiedDate() const { return modifiedDate_; }
  void setModifiedDate(const std::string& modifiedDate) {
    modifiedDate_ = modifiedDate; }

#ifdef DEBUG_ITEM
  void print(void);
#endif

 protected:
  std::string id_;
  std::string directoryURI_;
  std::string title_;
  std::string storageType_;
  std::string modifiedDate_;
};

class ContentItem {
 public:
  ContentItem() : size_(0), rating_(0), bitrate_(0), trackNumber_(0),
      duration_(0), width_(0), height_(0), latitude_(DEFAULT_GEOLOCATION),
      longitude_(DEFAULT_GEOLOCATION) {
    editable_attributes_.push_back("name");
    editable_attributes_.push_back("description");
    editable_attributes_.push_back("rating");
    editable_attributes_.push_back("geolocation");
    editable_attributes_.push_back("orientation");
  }

  void init(media_info_h handle);

  // Getters & Setters
  const std::vector<std::string>& editable_attributes() const {
    return editable_attributes_;
  }
  const std::string& id() const { return id_; }
  void setID(const std::string& id) { id_ = id; }
  const std::string& name() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  const std::string& type() const { return type_; }
  void setType(const std::string& type) { type_ = type;}
  const std::string& mimeType() const { return mimeType_; }
  void setMimeType(const std::string& mimeType) { mimeType_ = mimeType; }
  const std::string& title() const { return title_; }
  void setTitle(const std::string& title) { title_ = title;}
  const std::string& contentURI() const { return contentURI_; }
  void setContentURI(const std::string& uri) { contentURI_ = uri; }
  const std::string& thumbnailURIs() const { return thumbnailURIs_; }
  void setThumbnailURIs(const std::string& uris) { thumbnailURIs_ = uris; }
  const std::string& releaseDate() const { return releaseDate_; }
  void setReleaseDate(const std::string releaseDate) {
    releaseDate_ = releaseDate; }
  const std::string& modifiedDate() const { return modifiedDate_; }
  void setModifiedDate(const std::string& modifiedDate) {
    modifiedDate_ = modifiedDate; }
  uint64_t size() const { return size_; }
  void setSize(const uint64_t size) { size_ = size; }
  const std::string& description() const { return description_; }
  void setDescription(const std::string& desc) { description_ = desc; }
  uint64_t rating() const { return rating_; }
  void setRating(uint64_t rating) { rating_ = rating; }
  // type = AUDIO and VIDEO
  const std::string& album() const { return album_; }
  void setAlbum(const std::string& album) { album_ = album;}
  const std::string& genres() const { return genres_; }
  void setGenres(const std::string& genres) { genres_ = genres;}
  const std::string& artists() const { return artists_; }
  void setArtists(const std::string& artists) { artists_ = artists;}
  const std::string& composer() const { return composer_; }
  void setComposer(const std::string& composer) { composer_ = composer;}
  const std::string& copyright() const { return copyright_; }
  void setCopyright(const std::string& copyright) { copyright_ = copyright;}
  uint64_t bitrate() const { return rating_; }
  void setBitrate(uint64_t bitrate) { bitrate_ = bitrate; }
  uint64_t trackNumber() const { return bitrate_; }
  void setTrackNumber(uint64_t num) { trackNumber_ = num; }
  int duration() const { return duration_; }
  void setDuration(int duration) { duration_ = duration; }
  // type = IMAGE
  uint64_t width() const { return width_; }
  void setWidth(uint64_t width) { width_ = width; }
  uint64_t height() const { return height_; }
  void setHeight(uint64_t height) { height_ = height; }
  const std::string& orientation() const { return orientation_; }
  void setOrientation(const std::string& orintatin) {orientation_ = orintatin;}
  double latitude() const { return latitude_; }
  void setLatitude(double latitude) { latitude_ = latitude; }
  double longitude() const { return longitude_; }
  void setLongitude(double longitude) { longitude_ = longitude; }

#ifdef DEBUG_ITEM
  void print(void);
#endif

 protected:
  std::vector<std::string> editable_attributes_;
  std::string id_;
  std::string name_;
  std::string type_;
  std::string mimeType_;
  std::string title_;
  std::string contentURI_;
  std::string thumbnailURIs_;
  std::string releaseDate_;
  std::string modifiedDate_;
  uint64_t size_;
  std::string description_;
  uint64_t rating_;

  // type = AUDIO and VIDEO
  std::string album_;
  std::string genres_;
  std::string artists_;
  std::string composer_;
  std::string copyright_;
  uint64_t bitrate_;
  uint16_t trackNumber_;
  int duration_;

  // type = IMAGE
  uint64_t width_;
  uint64_t height_;
  double latitude_;
  double longitude_;
  std::string orientation_;
  const double DEFAULT_GEOLOCATION = -200;
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
