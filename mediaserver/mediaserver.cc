// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mediaserver/mediaserver.h"

#include <map>
#include "common/extension.h"

namespace {

const gchar kdLeynaInterfaceName[] = "com.intel.dleyna-server";

const gchar kMediaObjectPath[] = "Path";
const gchar kMediaObjectType[] = "Type";
const gchar kMediaObjectDisplayName[] = "DisplayName";
const gchar kMediaObjectURLs[] = "URLs";
const gchar kMediaObjectMIMEType[] = "MIMEType";
const gchar kMediaObjectDate[] = "Date";
const gchar kMediaObjectSize[] = "Size";
const gchar kMediaObjectWidth[] = "Width";
const gchar kMediaObjectHeight[] = "Height";
const gchar kMediaObjectDuration[] = "Duration";
const gchar kMediaObjectBitrate[] = "Bitrate";
const gchar kMediaObjectAlbum[] = "Album";
const gchar kMediaObjectArtist[] = "Artist";
const gchar kMediaObjectGenre[] = "Genre";
const gchar kMediaObjectTrackNumber[] = "TrackNumber";

// map between GUPnP and w3c spec names
const std::map<std::string, std::string> g_gupnp_w3c_map = {
  {kMediaObjectPath, "id"},
  {kMediaObjectType, "type"},
  {kMediaObjectDisplayName, "title"},
  {kMediaObjectURLs, "sourceUri"},
  {kMediaObjectMIMEType, "mimeType"},
  {kMediaObjectDate, "createDate"},
  {kMediaObjectSize, "fileSize"},
  {kMediaObjectWidth, "width"},
  {kMediaObjectHeight, "height"},
  {kMediaObjectDuration, "duration"},
  {kMediaObjectBitrate, "audioSampleRate"},
  {kMediaObjectAlbum, "collection"},
  {kMediaObjectArtist, "author"},
  {kMediaObjectGenre, "category"},
  {kMediaObjectTrackNumber, "trackNumber"}
};

const gchar kMediaObjectTypeContainer[] = "container";
const gchar kMediaObjectTypeVideo[] = "video";
const gchar kMediaObjectTypeAudio[] = "audio";
const gchar kMediaObjectTypeImage[] = "image";

const gchar *const kArgFilter[] = {
    kMediaObjectPath,
    kMediaObjectType,
    kMediaObjectDisplayName,
    kMediaObjectURLs,
    kMediaObjectMIMEType,
    kMediaObjectDate,
    kMediaObjectSize,
    kMediaObjectWidth,
    kMediaObjectHeight,
    kMediaObjectDuration,
    kMediaObjectBitrate,
    kMediaObjectAlbum,
    kMediaObjectArtist,
    kMediaObjectGenre,
    kMediaObjectTrackNumber,
    NULL
};

const gchar *const kContainerObjectTypes[] = { "*", NULL };

}  // namespace

static void setChildCount(picojson::value::object& container,
                          const std::string& object_path) {
  GError* gerror = NULL;
  upnpMediaContainer2* root_container_proxy =
      upnp_media_container2_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      kdLeynaInterfaceName,
      object_path.c_str(),
      NULL,
      &gerror);

  if (!gerror) {
    container["childCount"] = picojson::value(
        static_cast<double>(
            upnp_media_container2_get_child_count(root_container_proxy)));
    g_object_unref(root_container_proxy);
  } else {
    // child count may be null.
    g_error_free(gerror);
  }
}

static picojson::value variantToJSON(GVariant* variant) {
  picojson::value value;
  switch (g_variant_classify(variant)) {
    case G_VARIANT_CLASS_STRING:
    case G_VARIANT_CLASS_OBJECT_PATH:
    {
      gsize length = 0;
      const gchar* str = g_variant_get_string(variant, &length);
      if (length)
        value = picojson::value(str);
    }
      break;

    case G_VARIANT_CLASS_INT32:
      value =
          picojson::value(static_cast<double>(g_variant_get_int32(variant)));
      break;

    case G_VARIANT_CLASS_INT64:
      value =
          picojson::value(static_cast<double>(g_variant_get_int64(variant)));
      break;

    // Only used for URLs, spec supports only 1 URL
    case G_VARIANT_CLASS_ARRAY:
    {
      GVariantIter* it;
      gchar* str = NULL;
      g_variant_get(variant, "as", &it);
      if (g_variant_iter_loop(it, "s", &str)) {
        g_variant_iter_free(it);
        value = picojson::value(str);
      }
    }
      break;

    default:
      std::cerr << "Cannot process GVariant with type: "
                << g_variant_get_type_string(variant) << "\n";
      break;
  }

  return value;
}

static picojson::value toJSONValue(const gchar* value) {
  return value ? picojson::value(value) : picojson::value();
}

static picojson::value toJSONValueArray(const gchar* const* values) {
  picojson::array array;
  while (*values)
    array.push_back(picojson::value(*values++));
  return picojson::value(array);
}

MediaServer::MediaServer(common::Instance* instance,
                         const std::string& object_path)
    : instance_(instance),
      mediadevice_proxy_(0),
      object_path_(object_path),
      cancellable_(g_cancellable_new()) {
  GError* gerror = NULL;
  mediadevice_proxy_ = dleyna_media_device_proxy_new_for_bus_sync(
      G_BUS_TYPE_SESSION,
      G_DBUS_PROXY_FLAGS_NONE,
      kdLeynaInterfaceName,
      object_path.c_str(),
      NULL,
      &gerror);

  if (gerror) {
    g_error_free(gerror);
    return;
  }
}

MediaServer::~MediaServer() {
  g_object_unref(mediadevice_proxy_);
}

picojson::value MediaServer::toJSON() {
  if (!object_.size()) {
    object_["id"] = toJSONValue(
        dleyna_media_device_get_location(mediadevice_proxy_));
    object_["friendlyName"] = toJSONValue(
        dleyna_media_device_get_friendly_name(mediadevice_proxy_));
    object_["manufacturer"] = toJSONValue(
        dleyna_media_device_get_manufacturer(mediadevice_proxy_));
    object_["manufacturerURL"] = toJSONValue(
        dleyna_media_device_get_manufacturer_url(mediadevice_proxy_));
    object_["modelDescription"] = toJSONValue(
        dleyna_media_device_get_model_description(mediadevice_proxy_));
    object_["modelName"] = toJSONValue(
        dleyna_media_device_get_model_name(mediadevice_proxy_));
    object_["modelNumber"] = toJSONValue(
        dleyna_media_device_get_model_number(mediadevice_proxy_));
    object_["serialNumber"] = toJSONValue(
        dleyna_media_device_get_serial_number(mediadevice_proxy_));
    object_["UDN"] = toJSONValue(
        dleyna_media_device_get_udn(mediadevice_proxy_));
    object_["presentationURL"] = toJSONValue(
        dleyna_media_device_get_presentation_url(mediadevice_proxy_));
    object_["iconURL"] = toJSONValue(
        dleyna_media_device_get_icon_url(mediadevice_proxy_));
    object_["deviceType"] = toJSONValue(
        dleyna_media_device_get_device_type(mediadevice_proxy_));

    picojson::value::object rootContainer;
    rootContainer["id"] = picojson::value(object_path_);
    rootContainer["title"] = picojson::value("root");
    rootContainer["type"] = picojson::value(kMediaObjectTypeContainer);
    setChildCount(rootContainer, object_path_);

    object_["root"] = picojson::value(rootContainer);
    object_["canCreateContainer"] = picojson::value("true");
    object_["canUpload"] = picojson::value("true");
    object_["searchAttrs"] = toJSONValueArray(
        dleyna_media_device_get_search_caps(mediadevice_proxy_));
    object_["sortAttrs"] = toJSONValueArray(
        dleyna_media_device_get_sort_caps(mediadevice_proxy_));
  }

  return picojson::value(object_);
}

picojson::value MediaServer::mediaObjectToJSON(GVariant* variant) {
  GVariantIter iter;
  GVariant *value;
  gchar *key;
  bool is_container = false;
  picojson::value::object object;
  std::map<std::string, std::string>::const_iterator it;
  g_variant_iter_init(&iter, variant);
  while (g_variant_iter_next(&iter, "{sv}", &key, &value)) {
    if ((it = g_gupnp_w3c_map.find(key)) != g_gupnp_w3c_map.end()) {
      object[(*it).second] = variantToJSON(value);
      if (!g_strcmp0(kMediaObjectType, key)) {
        is_container = true;
        object["rootContainerId"] = picojson::value(
            g_dbus_proxy_get_object_path(
                reinterpret_cast<GDBusProxy*>(mediadevice_proxy_)));
      }
    }
    g_variant_unref(value);
    g_free(key);
  }

  if (is_container)
    setChildCount(object, object["id"].get<std::string>());

  return picojson::value(object);
}

void MediaServer::browse(const picojson::value& value) {
  GError* gerror = NULL;
  double async_call_id = value.get("asyncCallId").get<double>();

  upnpMediaContainer2* container_proxy =
      upnp_media_container2_proxy_new_for_bus_sync(
          G_BUS_TYPE_SESSION,
          G_DBUS_PROXY_FLAGS_NONE,
          kdLeynaInterfaceName,
          value.get("containerId").to_str().c_str(),
          NULL,
          &gerror);

  if (gerror) {
    postError(async_call_id);
    g_error_free(gerror);
    return;
  }

  upnp_media_container2_call_list_children_ex(
      container_proxy,
      value.get("offset").get<double>(),
      value.get("count").get<double>(),
      kArgFilter,
      value.get("sortMode").to_str().c_str(),
      cancellable_,
      OnBrowseCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::find(const picojson::value& value) {
  GError* gerror = NULL;
  double async_call_id = value.get("asyncCallId").get<double>();

  upnpMediaContainer2* container_proxy =
      upnp_media_container2_proxy_new_for_bus_sync(
          G_BUS_TYPE_SESSION,
          G_DBUS_PROXY_FLAGS_NONE,
          kdLeynaInterfaceName,
          value.get("containerId").to_str().c_str(),
          NULL,
          &gerror);

  if (gerror) {
    postError(async_call_id);
    g_error_free(gerror);
    return;
  }

  if (!upnp_media_container2_get_searchable(container_proxy)) {
    postError(async_call_id);
    return;
  }

  upnp_media_container2_call_search_objects_ex(
      container_proxy,
      value.get("searchFilter").to_str().c_str(),
      value.get("offset").get<double>(),
      value.get("count").get<double>(),
      kArgFilter,
      value.get("sortMode").to_str().c_str(),
      cancellable_,
      OnFindCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::createFolder(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mediadevice_proxy_) {
    postError(async_call_id);
    return;
  }

  dleyna_media_device_call_create_container_in_any_container(
      mediadevice_proxy_,
      value.get("folderName").to_str().c_str(),
      kMediaObjectTypeContainer,
      kContainerObjectTypes,
      cancellable_,
      OnCreateFolderCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::upload(const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();

  if (!mediadevice_proxy_) {
    postError(async_call_id);
    return;
  }

  dleyna_media_device_call_upload_to_any_container(
      mediadevice_proxy_,
      value.get("title").to_str().c_str(),
      value.get("path").to_str().c_str(),
      cancellable_,
      OnUploadCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::cancel(const picojson::value& value) {
  g_cancellable_cancel(cancellable_);
  postResult("cancelCompleted", value.get("asyncCallId").get<double>());
}

bool MediaServer::isCancelled() const {
  return g_cancellable_is_cancelled(cancellable_);
}

void MediaServer::uploadToContainer(const picojson::value& value) {
  GError* gerror = NULL;
  double async_call_id = value.get("asyncCallId").get<double>();

  upnpMediaContainer2* container_proxy =
      upnp_media_container2_proxy_new_for_bus_sync(
          G_BUS_TYPE_SESSION,
          G_DBUS_PROXY_FLAGS_NONE,
          kdLeynaInterfaceName,
          value.get("containerId").to_str().c_str(),
          NULL,
          &gerror);

  if (gerror) {
    postError(async_call_id);
    g_error_free(gerror);
    return;
  }

  upnp_media_container2_call_upload(
      container_proxy,
      value.get("title").to_str().c_str(),
      value.get("path").to_str().c_str(),
      cancellable_,
      OnUploadToContainerCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::createFolderInContainer(const picojson::value& value) {
  GError* gerror = NULL;
  double async_call_id = value.get("asyncCallId").get<double>();

  upnpMediaContainer2* container_proxy =
      upnp_media_container2_proxy_new_for_bus_sync(
          G_BUS_TYPE_SESSION,
          G_DBUS_PROXY_FLAGS_NONE,
          kdLeynaInterfaceName,
          value.get("containerId").to_str().c_str(),
          NULL,
          &gerror);

  if (gerror) {
    postError(async_call_id);
    g_error_free(gerror);
    return;
  }

  upnp_media_container2_call_create_container(
      container_proxy,
      value.get("title").to_str().c_str(),
      kMediaObjectTypeContainer,
      kContainerObjectTypes,
      cancellable_,
      OnCreateFolderInContainerCallBack,
      new CallbackData(this, async_call_id));
}

void MediaServer::postResult(
    GVariant* objects,
    const char* completed_operation,
    double async_operation_id) {
  GVariantIter it;
  GVariant *variant;
  picojson::array json_objects;

  g_variant_iter_init(&it, objects);
  while ((variant = g_variant_iter_next_value(&it))) {
    json_objects.push_back(mediaObjectToJSON(variant));
    g_variant_unref(variant);
  }
  g_variant_unref(objects);

  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);
  object["mediaObjects"] = picojson::value(json_objects);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaServer::postResult(
    const char* completed_operation,
    double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value(completed_operation);
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaServer::postError(double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value("asyncCallError");
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  instance_->PostMessage(value.serialize().c_str());
}

void MediaServer::OnBrowse(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  GVariant* objects;
  if (upnp_media_container2_call_list_children_ex_finish(
      reinterpret_cast<upnpMediaContainer2*>(source_object),
      &objects,
      res,
      &gerror))
    postResult(objects, "browseCompleted", async_id);
  else
    postError(async_id);

  if (gerror)
    g_error_free(gerror);

  g_object_unref(source_object);
}

void MediaServer::OnFind(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  GVariant* objects;
  guint totalItems;

  if (upnp_media_container2_call_search_objects_ex_finish(
      reinterpret_cast<upnpMediaContainer2*>(source_object),
      &objects,
      &totalItems,
      res,
      &gerror))
    postResult(objects, "findCompleted", async_id);
  else
    postError(async_id);

  if (gerror)
    g_error_free(gerror);

  g_object_unref(source_object);
}

void MediaServer::OnCreateFolder(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  gchar **out_Paths = NULL;

  if (dleyna_media_device_call_create_container_in_any_container_finish(
      mediadevice_proxy_,
      out_Paths,
      res,
      &gerror))
    postResult("createFolderCompleted", async_id);
  else
    postError(async_id);

  if (gerror)
    g_error_free(gerror);
}

void MediaServer::OnUpload(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  guint upload_id;
  gchar **out_Path = NULL;

  if (dleyna_media_device_call_upload_to_any_container_finish(
      mediadevice_proxy_,
      &upload_id,
      out_Path,
      res,
      &gerror))
    postResult("uploadCompleted", async_id);
  else
    postError(async_id);

  if (gerror) {
    g_error_free(gerror);
  }
}

void MediaServer::OnUploadToContainer(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  guint out_UploadId;
  gchar **out_Path = NULL;

  if (upnp_media_container2_call_upload_finish(
      reinterpret_cast<upnpMediaContainer2*>(source_object),
      &out_UploadId,
      out_Path,
      res,
      &gerror))
    postResult("uploadToContainerCompleted", async_id);
  else
    postError(async_id);

  if (gerror) {
    g_error_free(gerror);
  }

  g_object_unref(source_object);
}

void MediaServer::OnCreateFolderInContainer(
    GObject *source_object,
    GAsyncResult *res,
    double async_id) {
  GError* gerror = NULL;
  gchar **out_Path = NULL;

  if (upnp_media_container2_call_create_container_finish(
      reinterpret_cast<upnpMediaContainer2*>(source_object),
      out_Path,
      res,
      &gerror))
    postResult("createFolderInContainerCompleted", async_id);
  else
    postError(async_id);

  if (gerror) {
    g_error_free(gerror);
  }

  g_object_unref(source_object);
}
