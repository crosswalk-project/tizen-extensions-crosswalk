// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/content_instance.h"
#include "content/content_filter.h"

#include <media_content.h>
#include <media_filter.h>

#include <assert.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <string>
#include "common/picojson.h"

namespace {

const std::string STR_ID("id");
const std::string STR_NAME("name");
const std::string STR_DESCRIPTION("description");
const std::string STR_RATING("rating");
const std::string STR_FILTER("filter");
const std::string STR_CONTENT_URI("contentURI");
const std::string STR_EVENT_TYPE("eventType");

std::string createUriFromLocalPath(const std::string path) {
  static std::string fileScheme("file://");

  return fileScheme + path;
}

std::string getUriPath(const std::string uri) {
  static std::string fileScheme("file://");
  std::string _fileScheme = uri.substr(0, fileScheme.size());

  if (_fileScheme == fileScheme)
    return uri.substr(fileScheme.size());
  else
    return "";
}

// Tizen video meta APIs return date format "%Y:%m:%d %H:%M:%S",
// while standard JavaScript Date accept format as "%Y-%m-%d %H:%M:%S".
// So we need this conversion function.
std::string ConvertToJSDateString(const char* date) {
  struct tm result;
  if (strptime(date, "%Y:%m:%d %H:%M:%S", &result) == NULL) {
    return "";
  }

  char output_date[20];
  memset(output_date, 0, 20);
  if (!strftime(output_date, 20, "%Y-%m-%d %H:%M:%S", &result)) {
    return "";
  }

  return std::string(output_date);
}

}  // namespace

unsigned ContentInstance::m_instanceCount = 0;

ContentInstance::ContentInstance() {
  ++m_instanceCount;
  if (media_content_connect() != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_content_connect: DB connection error" << std::endl;
    return;
  }
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
#ifdef DEBUG_JSON_CMD
  std::cout << "HandleMessage: " << message << std::endl;
#endif
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "ContentManager.getDirectories") {
    HandleGetDirectoriesRequest(v);
  } else if (cmd == "ContentManager.find") {
    HandleFindRequest(v);
  } else if (cmd == "ContentManager.scanFile") {
    HandleScanFileRequest(v);
  } else if (cmd == "ContentManager.updateBatch") {
    HandleUpdateBatchRequest(v);
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
  if (msg.contains(STR_EVENT_TYPE))
    reply[STR_EVENT_TYPE] = picojson::value(msg.get(STR_EVENT_TYPE));
#ifdef DEBUG_JSON_CMD
  std::cout << "reply: " << msg.serialize().c_str() << std::endl;
#endif
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
  picojson::value v;
  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cerr << "Ignoring message.\n";
    return;
  }
#ifdef DEBUG_JSON_CMD
  std::cout << "HandleSyncMessage: " << message << std::endl;
#endif
  std::string cmd = v.get("cmd").to_str();
  int rc = MEDIA_CONTENT_ERROR_INVALID_OPERATION;

  if (cmd == "ContentManager.setChangeListener") {
    rc = media_content_set_db_updated_cb(MediaContentChangeCallback, this);
  } else if (cmd == "ContentManager.unsetChangeListener") {
    rc = media_content_unset_db_updated_cb();
  } else if (cmd == "ContentManager.update") {
    if (HandleUpdateRequest(v.get("content")))
      rc = MEDIA_CONTENT_ERROR_NONE;
  } else {
    std::cerr << "Message " + cmd + " is not supported.\n";
  }

  if (rc != MEDIA_CONTENT_ERROR_NONE)
    std::cerr << "error " << cmd << std::endl;

#ifdef DEBUG_JSON_CMD
  std::cout << "Reply: " << v.serialize().c_str() << std::endl;
#endif
  SendSyncReply(v.serialize().c_str());
}

bool ContentInstance::HandleUpdateRequest(const picojson::value& msg) {
  if (!msg.contains(STR_ID)) {
    std::cerr << "HandleUpdateRequest: No ID in the message" << std::endl;
    return false;
  }

  media_info_h handle;
  if (media_info_get_media_from_db(msg.get(STR_ID).to_str().c_str(), &handle)
      != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_get_media_from_db: error" << std::endl;
    return false;
  }

  bool no_error = true;
  if (msg.contains(STR_NAME) &&
      media_info_set_display_name(handle,
          msg.get(STR_NAME).to_str().c_str()) != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_set_display_name: error" << std::endl;
    no_error = false;
  }

  if (msg.contains(STR_DESCRIPTION) &&
      media_info_set_description(handle,
          msg.get(STR_DESCRIPTION).to_str().c_str())
              != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_set_description: error" << std::endl;
    no_error = false;
  }

  if (msg.contains(STR_RATING)) {
    int i = std::stoi(msg.get(STR_RATING).to_str());
    if (i >= 0 && i <= 10) {
      if (media_info_set_rating(handle, i) != MEDIA_CONTENT_ERROR_NONE) {
        std::cerr << "media_info_set_rating: error" << std::endl;
        no_error = false;
      }
    }
  }

  // Commit the changes to DB
  if (media_info_update_to_db(handle) != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_update_to_db: error" << std::endl;
    no_error = false;
  }

  return no_error;
}

void ContentInstance::HandleUpdateBatchRequest(const picojson::value& msg) {
  picojson::array list = msg.get("content").get<picojson::array>();

  for (picojson::array::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (!HandleUpdateRequest((*i))) {
      PostAsyncErrorReply(msg, WebApiAPIErrors::INVALID_MODIFICATION_ERR);
      return;
    }
  }
  PostAsyncSuccessReply(msg);
}

void ContentInstance::HandleGetDirectoriesRequest(const picojson::value& msg) {
  ContentFolderList folderList;
  if (media_folder_foreach_folder_from_db(NULL,
          MediaFolderCallback,
          reinterpret_cast<void*>(&folderList)) != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_folder_foreach_folder_from_db: error" << std::endl;
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

    o[STR_ID] = picojson::value(folder->id());
    o["directoryURI"] = picojson::value(folder->directoryURI());
    o["title"] = picojson::value(folder->title());
    o["storageType"] = picojson::value(folder->storageType());
    o["modifiedDate"] = picojson::value(folder->modifiedDate());

    folders.push_back(picojson::value(o));
  }
  picojson::value value(folders);
  PostAsyncSuccessReply(msg, value);
}

bool ContentInstance::MediaFolderCallback(media_folder_h handle,
    void* user_data) {
  if (!user_data)
    return false;

  ContentFolderList* folderList =
    reinterpret_cast<ContentFolderList*>(user_data);

  ContentFolder* folder = new ContentFolder;
  folder->init(handle);
  folderList->addFolder(folder);
#ifdef DEBUG_ITEM
  folder->print();
#endif
  return true;
}

void ContentInstance::HandleFindRequest(const picojson::value& msg) {
  ContentItemList itemList;
  filter_h filterHandle = NULL;

  ContentFilter& filter = ContentFilter::instance();
  if (msg.contains(STR_FILTER)) {
    picojson::value filterValue = msg.get(STR_FILTER);
    if (!filterValue.is<picojson::null>() &&
        filterValue.is<picojson::object>()) {
      std::string condition = filter.convert(msg.get(STR_FILTER));
      if (media_filter_create(&filterHandle) == MEDIA_CONTENT_ERROR_NONE)
        media_filter_set_condition(filterHandle,
            condition.c_str(), MEDIA_CONTENT_COLLATE_DEFAULT);
    }
  }

  if (media_info_foreach_media_from_db(filterHandle,
      MediaInfoCallback,
      reinterpret_cast<ContentFolderList*>(&itemList))
      != MEDIA_CONTENT_ERROR_NONE) {
    std::cerr << "media_info_foreach_media_from_db: error" << std::endl;
  } else {
    HandleFindReply(msg, &itemList);
  }

  if (filterHandle != NULL && media_filter_destroy(filterHandle)
      != MEDIA_CONTENT_ERROR_NONE)
    std::cerr << "media_filter_destroy failed" << std::endl;
}

void ContentInstance::HandleFindReply(
    const picojson::value& msg,
    ContentItemList* itemList) {
  const std::vector<ContentItem*> &results = itemList->getAllItems();

  picojson::value::array items;

  for (unsigned i = 0; i < results.size(); i++) {
    ContentItem* item = results[i];

    picojson::value::object o;

    picojson::value::array editableAttributesJson;
    std::vector<std::string> editableAttributes = item->editable_attributes();
    for (unsigned i = 0; i < editableAttributes.size(); i++)
      editableAttributesJson.push_back(picojson::value(editableAttributes[i]));
    o["editableAttributes"] = picojson::value(editableAttributesJson);
    o[STR_ID] = picojson::value(item->id());
    o[STR_NAME] = picojson::value(item->name());
    o["type"] = picojson::value(item->type());
    o["mimeType"] = picojson::value(item->mime_type());
    o["title"] = picojson::value(item->title());
    o["contentURI"] = picojson::value(item->content_uri());
    picojson::value::array uris;
    uris.push_back(picojson::value(item->thumbnail_uris()));
    o["thumbnailURIs"] = picojson::value(uris);
    o["releaseDate"] = picojson::value(item->release_date());
    o["modifiedDate"] =
      picojson::value(item->modified_date());
    o["size"] = picojson::value(static_cast<double>(item->size()));
    o[STR_DESCRIPTION] = picojson::value(item->description());
    o[STR_RATING] = picojson::value(static_cast<double>(item->rating()));

    if (item->type() == "AUDIO") {
      o["album"] = picojson::value(item->album());
      picojson::value::array genres;
      genres.push_back(picojson::value(item->genres()));
      o["genres"] = picojson::value(genres);
      picojson::value::array artists;
      artists.push_back(picojson::value(item->artists()));
      o["artists"] = picojson::value(artists);
      picojson::value::array composers;
      composers.push_back(picojson::value(item->composer()));
      o["composers"] = picojson::value(composers);
      o["copyright"] = picojson::value(item->copyright());
      o["bitrate"] = picojson::value(static_cast<double>(item->bitrate()));
      o["trackNumber"] = picojson::value(
          static_cast<double>(item->track_number()));
      o["duration"] = picojson::value(static_cast<double>(item->duration()));
    } else if (item->type() == "IMAGE") {
      o["width"] = picojson::value(static_cast<double>(item->width()));
      o["height"] = picojson::value(static_cast<double>(item->height()));
      o["orientation"] = picojson::value(item->orientation());
      o["latitude"] =
          picojson::value(static_cast<double>(item->latitude()));
      o["longitude"] =
          picojson::value(static_cast<double>(item->longitude()));
    } else if (item->type() == "VIDEO") {
      o["album"] = picojson::value(item->album());
      picojson::value::array artists;
      artists.push_back(picojson::value(item->artists()));
      o["artists"] = picojson::value(artists);
      o["duration"] = picojson::value(static_cast<double>(item->duration()));
      o["width"] = picojson::value(static_cast<double>(item->width()));
      o["height"] = picojson::value(static_cast<double>(item->height()));
      o["latitude"] =
          picojson::value(static_cast<double>(item->latitude()));
      o["longitude"] =
          picojson::value(static_cast<double>(item->longitude()));
    }

    picojson::value v(o);

    items.push_back(picojson::value(o));
  }
  picojson::value value(items);
#ifdef DEBUG_JSON_REPLY
  std::cout << "JSON reply: " << std::endl <<
     value.serialize().c_str() << std::endl;
#endif
  PostAsyncSuccessReply(msg, value);
}

bool ContentInstance::MediaInfoCallback(media_info_h handle, void* user_data) {
  if (!user_data)
    return false;

  ContentItemList* itemList = reinterpret_cast<ContentItemList*>(user_data);

  ContentItem* item = new ContentItem;
  item->init(handle);
  itemList->addItem(item);
#ifdef DEBUG_ITEM
  item->print();
#endif
  return true;
}

void ContentInstance::MediaContentChangeCallback(
    media_content_error_e error,
    int pid,
    media_content_db_update_item_type_e update_item,
    media_content_db_update_type_e update_type,
    media_content_type_e media_type,
    char* uuid,
    char* path,
    char* mime_type,
    void* user_data) {
#ifdef DEBUG_ITEM
  std::cout << "MediaContentChangeCallback: error=" << error <<
      ", item=" << update_item << ", type=" << update_type << ", " <<
      uuid << ", " << path << std::endl;
#endif
  if (!user_data)
    return;

  ContentInstance* self =
      reinterpret_cast<ContentInstance*>(user_data);

  picojson::value::object om;
  om["replyId"] = picojson::value(static_cast<double>(0));
  const std::string type = (update_type == MEDIA_CONTENT_INSERT ?
      "INSERT" : (update_type == MEDIA_CONTENT_DELETE ? "DELETE" : "UPDATE"));
  om[STR_EVENT_TYPE] = picojson::value(type);
  picojson::value::object ov;
  ov["type"] = picojson::value(static_cast<double>(media_type));

  if (uuid)
    ov["id"] = picojson::value(uuid);

  if (path)
    ov["contentURI"] = picojson::value(path);

  if (mime_type)
    ov["mimeType"] = picojson::value(mime_type);

  picojson::value msg(om);
  picojson::value value(ov);
#ifdef DEBUG_JSON_REPLY
  std::cout << "JSON event msg: " << msg.serialize().c_str() << std::endl;
  std::cout << "JSON event val: " << value.serialize().c_str() << std::endl;
#endif

  self->PostAsyncSuccessReply(msg, value);
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
    setDirectoryURI(createUriFromLocalPath(str));
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
    char tmp[26];
    ctime_r(&date, tmp);
    setModifiedDate(tmp);
  }
}

#ifdef DEBUG_ITEM
void ContentFolder::print(void) {
  std::cout << "ID: " << id() << std::endl;
  std::cout << "URI: " << directoryURI() << std::endl;
  std::cout << "Title: " << title() << std::endl;
  std::cout << "Type: " << storageType() << std::endl;
  std::cout << "Date: " << modifiedDate() << std::endl;
}
#endif

void ContentItem::init(media_info_h handle) {
  char* pc = NULL;

  // NOTE: the Tizen CAPI media_info_* functions assumes
  // the caller frees the char**. The CAPI can also return NULL char**
  // even the return code is MEDIA_CONTENT_ERROR_NONE.

  if (media_info_get_media_id(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_id(pc);
    free(pc);
  }

  if (media_info_get_mime_type(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_mime_type(pc);
    free(pc);
  }

  if (media_info_get_title(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_title(pc);
    free(pc);
  }

  if (media_info_get_display_name(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_name(pc);
    free(pc);
  }

  if (media_info_get_file_path(handle, &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_content_uri(createUriFromLocalPath(pc));
    free(pc);
  }

  if (media_info_get_thumbnail_path(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_thumbnail_uris(createUriFromLocalPath(pc));
    free(pc);
  }

  if (media_info_get_description(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    set_description(pc);
    free(pc);
  }

  time_t date;
  if (media_info_get_modified_time(handle, &date) == MEDIA_CONTENT_ERROR_NONE) {
    char tmp[26];
    ctime_r(&date, tmp);
    set_modified_date(tmp);
  }

  int i = 0;
  if (media_info_get_rating(handle, &i) == MEDIA_CONTENT_ERROR_NONE)
    set_rating(i);

  unsigned long long ll; // NOLINT
  if (media_info_get_size(handle, &ll) == MEDIA_CONTENT_ERROR_NONE)
    set_size(ll);

  media_content_type_e type;
  if (media_info_get_media_type(handle, &type) == MEDIA_CONTENT_ERROR_NONE) {
    if (type == MEDIA_CONTENT_TYPE_IMAGE) {
      set_type("IMAGE");

      image_meta_h image;
      if (media_info_get_image(handle, &image) == MEDIA_CONTENT_ERROR_NONE) {
        if (image_meta_get_width(image, &i) == MEDIA_CONTENT_ERROR_NONE)
          set_width(i);

        if (image_meta_get_height(image, &i) == MEDIA_CONTENT_ERROR_NONE)
          set_height(i);

        double d;
        if (media_info_get_latitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          set_latitude(d);

       if (media_info_get_longitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          set_longitude(d);

        media_content_orientation_e orientation;
        if (image_meta_get_orientation(image, &orientation)
            == MEDIA_CONTENT_ERROR_NONE) {
          std::string result("");

          switch (orientation) {
            case 0:
            case 1: result = "NORMAL"; break;
            case 2: result = "FLIP_HORIZONTAL"; break;
            case 3: result = "ROTATE_180"; break;
            case 4: result = "FLIP_VERTICAL"; break;
            case 5: result = "TRANSPOSE"; break;
            case 6: result = "ROTATE_90"; break;
            case 7: result = "TRANSVERSE"; break;
            case 8: result = "ROTATE_270"; break;
            default: result = "Unknown"; break;
          }
          set_orientation(result);
        }
        image_meta_destroy(image);
      }
    } else if (type == MEDIA_CONTENT_TYPE_VIDEO) {
      set_type("VIDEO");

      video_meta_h video;
      if (media_info_get_video(handle, &video) == MEDIA_CONTENT_ERROR_NONE) {
        if (video_meta_get_recorded_date(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_release_date(ConvertToJSDateString(pc));
          free(pc);
        }

        if (video_meta_get_album(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_album(pc);
          free(pc);
        }

        if (video_meta_get_artist(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_artists(pc);
          free(pc);
        }

        if (video_meta_get_width(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          set_width(i);
        }

        if (video_meta_get_height(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          set_height(i);
        }

        if (video_meta_get_duration(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          set_duration(i);
        }

        double d;
        if (media_info_get_latitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          set_latitude(d);

        if (media_info_get_longitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          set_longitude(d);

        video_meta_destroy(video);
      }
    } else if (type == MEDIA_CONTENT_TYPE_MUSIC ||
        type == MEDIA_CONTENT_TYPE_SOUND) {
      set_type("AUDIO");

      audio_meta_h audio;
      if (media_info_get_audio(handle, &audio) == MEDIA_CONTENT_ERROR_NONE) {
        if (audio_meta_get_recorded_date(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_release_date(ConvertToJSDateString(pc));
          free(pc);
        }

        if (audio_meta_get_album(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_album(pc);
          free(pc);
        }

        if (audio_meta_get_artist(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_artists(pc);
          free(pc);
        }

        if (audio_meta_get_composer(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_composer(pc);
          free(pc);
        }

        if (audio_meta_get_duration(audio, &i) == MEDIA_CONTENT_ERROR_NONE)
          set_duration(i);

        if (audio_meta_get_copyright(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          set_copyright(pc);
          free(pc);
        }

        if (audio_meta_get_track_num(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          i = atoi(pc);
          set_track_number(i);
          free(pc);
        }

        if (audio_meta_get_bit_rate(audio, &i) == MEDIA_CONTENT_ERROR_NONE)
          set_bitrate(i);
      }
      audio_meta_destroy(audio);
    } else if (type == MEDIA_CONTENT_TYPE_OTHERS) {
      set_type("OTHER");
    }
  }
}

#ifdef DEBUG_ITEM
void ContentItem::print(void) {
  std::cout << "----" << std::endl;
  std::cout << "ID: " << id() << std::endl;
  std::cout << "Name: " << name() << std::endl;
  std::cout << "Type: " << type() << std::endl;
  std::cout << "MIME: " << mime_type() << std::endl;
  std::cout << "Title: " << title() << std::endl;
  std::cout << "URI: " << content_uri() << std::endl;
  std::cout << "ThumbnailURIs: " << thumbnail_uris() << std::endl;
  std::cout << "Modified: " << modified_date();
  std::cout << "Size: " << size() << std::endl;
  std::cout << "Description: " << description() << std::endl;
  std::cout << "Rating: " << rating() << std::endl;
  if (type() == "AUDIO") {
    std::cout << "Release Date: " << release_date() << std::endl;
    std::cout << "Album: " << album() << std::endl;
    std::cout << "Genres: " << genres() << std::endl;
    std::cout << "Artists: " << artists() << std::endl;
    std::cout << "Composer: " << composer() << std::endl;
    std::cout << "Copyright: " << copyright() << std::endl;
    std::cout << "Bitrate: " << bitrate() << std::endl;
    std::cout << "Track num: " << track_number() << std::endl;
    std::cout << "Duration: " << duration() << std::endl;
  } else if (type() == "IMAGE") {
    std::cout << "Width/Height: " << width() << "/" << height() << std::endl;
    std::cout << "Latitude: " << latitude() << std::endl;
    std::cout << "Longitude: " << longitude() << std::endl;
    std::cout << "Orientation: " << orientation() << std::endl;
  } else if (type() == "VIDEO") {
    std::cout << "Album: " << album() << std::endl;
    std::cout << "Artists: " << artists() << std::endl;
    std::cout << "Duration: " << duration() << std::endl;
    std::cout << "Width/Height: " << width() << "/" << height() << std::endl;
  }
}
#endif

void ContentInstance::HandleScanFileRequest(const picojson::value& msg) {
  if (msg.contains(STR_CONTENT_URI)) {
    picojson::value uriValue = msg.get(STR_CONTENT_URI);
    if (!uriValue.is<picojson::null>()) {
      std::string uri = uriValue.to_str();
      std::string path = getUriPath(uri);
      if (path.empty())
        path = uri;
      int result = media_content_scan_file(path.c_str());
      if (result == MEDIA_CONTENT_ERROR_NONE) {
        HandleScanFileReply(msg);
      } else {
        std::cerr << "media_content_scan_file error:" << result << std::endl;
        PostAsyncErrorReply(msg, WebApiAPIErrors::DATABASE_ERR);
      }
    }
  }
}

void ContentInstance::HandleScanFileReply(const picojson::value& msg) {
  PostAsyncSuccessReply(msg);
}
