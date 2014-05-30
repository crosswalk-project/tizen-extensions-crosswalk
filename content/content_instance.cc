// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/content_instance.h"
#include "content/content_filter.h"

#include <media_content.h>
#include <media_filter.h>

#include <assert.h>

#include <iostream>
#include <fstream>
#include <string>
#include "common/picojson.h"

namespace {
const std::string STR_FILTER("filter");
const std::string STR_CONTENT_URI("contentURI");

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
#ifdef DEBUG_JSON
  std::cout << "HandleMessage: " << message << std::endl;
#endif
  std::string cmd = v.get("cmd").to_str();
  if (cmd == "ContentManager.getDirectories") {
    HandleGetDirectoriesRequest(v);
  } else if (cmd == "ContentManager.find") {
    HandleFindRequest(v);
  } else if (cmd == "ContentManager.scanFile") {
    HandleScanFileRequest(v);
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

    o["id"] = picojson::value(folder->id());
    o["directoryURI"] = picojson::value(folder->directoryURI());
    o["title"] = picojson::value(folder->title());
    o["storageType"] = picojson::value(folder->storageType());
    o["modifiedDate"] = picojson::value(folder->modifiedDate());

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
      mediaInfoCallback,
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

    o["id"] = picojson::value(item->id());
    o["name"] = picojson::value(item->name());
    o["type"] = picojson::value(item->type());
    o["mimeType"] = picojson::value(item->mimeType());
    o["title"] = picojson::value(item->title());
    o["contentURI"] = picojson::value(item->contentURI());
    picojson::value::array uris;
    uris.push_back(picojson::value(item->thumbnailURIs()));
    o["thumbnailURIs"] = picojson::value(uris);
    o["releaseDate"] = picojson::value(item->releaseDate());
    o["modifiedDate"] =
      picojson::value(item->modifiedDate());
    o["size"] = picojson::value(static_cast<double>(item->size()));
    o["description"] = picojson::value(item->description());
    o["rating"] = picojson::value(static_cast<double>(item->rating()));

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
          static_cast<double>(item->trackNumber()));
      o["duration"] = picojson::value(static_cast<double>(item->duration()));
    } else if (item->type() == "IMAGE") {
      o["width"] = picojson::value(static_cast<double>(item->width()));
      o["height"] = picojson::value(static_cast<double>(item->height()));
      o["orientation"] = picojson::value(item->orientation());
    } else if (item->type() == "VIDEO") {
      o["album"] = picojson::value(item->album());
      picojson::value::array artists;
      artists.push_back(picojson::value(item->artists()));
      o["artists"] = picojson::value(artists);
      o["duration"] = picojson::value(static_cast<double>(item->duration()));
      o["width"] = picojson::value(static_cast<double>(item->width()));
      o["height"] = picojson::value(static_cast<double>(item->height()));
    }

    picojson::value v(o);

    items.push_back(picojson::value(o));
  }
  picojson::value value(items);
#ifdef DEBUG_JSON
  std::cout << "JSON reply: " << std::endl <<
     value.serialize().c_str() << std::endl;
#endif
  PostAsyncSuccessReply(msg, value);
}

bool ContentInstance::mediaInfoCallback(media_info_h handle, void* user_data) {
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
    setContentURI(createUriFromLocalPath(pc));
    free(pc);
  }

  if (media_info_get_thumbnail_path(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setThumbnailURIs(createUriFromLocalPath(pc));
    free(pc);
  }

  if (media_info_get_description(handle,
      &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
    setDescription(pc);
    free(pc);
  }

  time_t date;
  if (media_info_get_modified_time(handle, &date) == MEDIA_CONTENT_ERROR_NONE) {
    char tmp[26];
    ctime_r(&date, tmp);
    setModifiedDate(tmp);
  }

  int i = 0;
  if (media_info_get_rating(handle, &i) == MEDIA_CONTENT_ERROR_NONE)
    setRating(i);

  unsigned long long ll; // NOLINT
  if (media_info_get_size(handle, &ll) == MEDIA_CONTENT_ERROR_NONE)
    setSize(ll);

  media_content_type_e type;
  if (media_info_get_media_type(handle, &type) == MEDIA_CONTENT_ERROR_NONE) {
    if (type == MEDIA_CONTENT_TYPE_IMAGE) {
      setType("IMAGE");

      image_meta_h image;
      if (media_info_get_image(handle, &image) == MEDIA_CONTENT_ERROR_NONE) {
        if (image_meta_get_width(image, &i) == MEDIA_CONTENT_ERROR_NONE)
          setWidth(i);

        if (image_meta_get_height(image, &i) == MEDIA_CONTENT_ERROR_NONE)
          setHeight(i);

        // TODO(spoussa): coordinates do not return sensible values...
        double d;
        if (media_info_get_latitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          setLatitude(d);

       if (media_info_get_longitude(handle, &d) == MEDIA_CONTENT_ERROR_NONE)
          setLongitude(d);

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
          setOrientation(result);
        }
        image_meta_destroy(image);
      }
    } else if (type == MEDIA_CONTENT_TYPE_VIDEO) {
      setType("VIDEO");

      video_meta_h video;
      if (media_info_get_video(handle, &video) == MEDIA_CONTENT_ERROR_NONE) {
        if (video_meta_get_recorded_date(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setReleaseDate(pc);
          free(pc);
        }

        if (video_meta_get_album(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setAlbum(pc);
          free(pc);
        }

        if (video_meta_get_artist(video,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setArtists(pc);
          free(pc);
        }

        if (video_meta_get_width(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          setWidth(i);
        }

        if (video_meta_get_height(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          setHeight(i);
        }

        if (video_meta_get_duration(video, &i) == MEDIA_CONTENT_ERROR_NONE) {
          setDuration(i);
        }

        video_meta_destroy(video);
      }
    } else if (type == MEDIA_CONTENT_TYPE_MUSIC) {
      setType("AUDIO");

      audio_meta_h audio;
      if (media_info_get_audio(handle, &audio) == MEDIA_CONTENT_ERROR_NONE) {
        if (audio_meta_get_recorded_date(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setReleaseDate(pc);
          free(pc);
        }

        if (audio_meta_get_album(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setAlbum(pc);
          free(pc);
        }

        if (audio_meta_get_artist(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setArtists(pc);
          free(pc);
        }

        if (audio_meta_get_composer(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setComposer(pc);
          free(pc);
        }

        if (audio_meta_get_duration(audio, &i) == MEDIA_CONTENT_ERROR_NONE)
          setDuration(i);

        if (audio_meta_get_copyright(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          setCopyright(pc);
          free(pc);
        }

        if (audio_meta_get_track_num(audio,
            &pc) == MEDIA_CONTENT_ERROR_NONE && pc) {
          i = atoi(pc);
          setTrackNumber(i);
          free(pc);
        }

        if (audio_meta_get_bit_rate(audio, &i) == MEDIA_CONTENT_ERROR_NONE)
          setBitrate(i);
      }
      audio_meta_destroy(audio);
    } else if (type == MEDIA_CONTENT_TYPE_OTHERS) {
      setType("OTHER");
    }
  }
}

#ifdef DEBUG_ITEM
void ContentItem::print(void) {
  std::cout << "----" << std::endl;
  std::cout << "ID: " << id() << std::endl;
  std::cout << "Name: " << name() << std::endl;
  std::cout << "Type: " << type() << std::endl;
  std::cout << "MIME: " << mimeType() << std::endl;
  std::cout << "Title: " << title() << std::endl;
  std::cout << "URI: " << contentURI() << std::endl;
  std::cout << "ThumbnailURIs: " << thumbnailURIs() << std::endl;
  std::cout << "Modified: " << modifiedDate();
  std::cout << "Size: " << size() << std::endl;
  std::cout << "Description: " << description() << std::endl;
  std::cout << "Rating: " << rating() << std::endl;
  if (type() == "AUDIO") {
    std::cout << "Release Date: " << releaseDate() << std::endl;
    std::cout << "Album: " << album() << std::endl;
    std::cout << "Genres: " << genres() << std::endl;
    std::cout << "Artists: " << artists() << std::endl;
    std::cout << "Composer: " << composer() << std::endl;
    std::cout << "Copyright: " << copyright() << std::endl;
    std::cout << "Bitrate: " << bitrate() << std::endl;
    std::cout << "Track num: " << trackNumber() << std::endl;
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
