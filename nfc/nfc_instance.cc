// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "nfc/nfc_instance.h"

#include <string>

#include "common/picojson.h"

namespace {

const char kTypeText[] = "T";
const char kTypeSmartPoster[] = "Sp";
const char kTypeUri[] = "U";
const char kTypeMime[] = "mime";
const char kTypeAlternativeCarrier[] = "ac";
const char kTypeHandoverCarrier[] = "Hc";
const char kTypeHandoverRequest[] = "Hr";
const char kTypeHandoverSelect[] = "Hs";

const char kJSTypeText[] = "text";
const char kJSTypeUri[] = "uri";
const char kJSTypeMedia[] = "media";
const char kJSTypeSmartPoster[] = "smartPoster";
const char kJSTypeUnknown[] = "unknown";

std::string NfcRecordTypeToJsType(const std::string& type) {
  if (type == kTypeText)
    return kJSTypeText;
  else if (type == kTypeSmartPoster)
    return kJSTypeSmartPoster;
  else if (type == kTypeUri)
    return kJSTypeUri;
  else if (type == kTypeMime)
    return kJSTypeMedia;
  else if (type.empty())
    return kJSTypeUnknown;
  else
    return type;
}

void SetTextProperties(nfc_ndef_record_h record,
    picojson::object& out_value) {
  char* text = NULL;
  if (nfc_ndef_record_get_text(record, &text) == NFC_ERROR_NONE)
    out_value["text"] = picojson::value(text);

  char* lang_code = NULL;
  if (nfc_ndef_record_get_langcode(record, &lang_code) == NFC_ERROR_NONE)
    out_value["languageCode"] = picojson::value(lang_code);

  nfc_encode_type_e encode;
  if (nfc_ndef_record_get_encode_type(record, &encode) == NFC_ERROR_NONE)
    out_value["encoding"] =
        picojson::value(encode == NFC_ENCODE_UTF_8 ? "UTF-8" : "UTF-16");
}

void SetUriProperties(nfc_ndef_record_h record,
    picojson::object& out_value) {
  char* uri = NULL;
  if (nfc_ndef_record_get_uri(record, &uri) == NFC_ERROR_NONE) {
    out_value["uri"] = picojson::value(uri);
    free(uri);
  }
}

void SetMIMEProperties(nfc_ndef_record_h record,
    picojson::object& out_value) {
  char* mime_type = NULL;
  if (nfc_ndef_record_get_mime_type(record, &mime_type) == NFC_ERROR_NONE) {
    out_value["mimeType"] = picojson::value(mime_type);
    free(mime_type);
  }
}

void SetType(const std::string& type, picojson::object& out_value) {
  out_value["recordType"] = picojson::value(NfcRecordTypeToJsType(type));
}

void SetTnf(nfc_record_tnf_e tnf, picojson::object& out_value) {
  out_value["tnf"] = picojson::value(static_cast<double>(tnf));
}

void SetId(nfc_ndef_record_h record, picojson::object& out_value) {
  unsigned char* id = NULL;
  int id_size = 0;
  int err = nfc_ndef_record_get_id(record, &id, &id_size);
  if (err == NFC_ERROR_NONE && id_size > 0)
    out_value["id"] =
        picojson::value(std::string(reinterpret_cast<char*>(id), id_size));
}

void SetPayload(nfc_ndef_record_h record, picojson::object& out_value) {
  unsigned char* payload = NULL;
  unsigned int payload_size = 0;
  int err = nfc_ndef_record_get_payload(record, &payload, &payload_size);
  if (err == NFC_ERROR_NONE) {
    picojson::array array;
    for (int i = 0; i < payload_size; i++)
      array.push_back(picojson::value(static_cast<double>(payload[i])));
    out_value["payload"] = picojson::value(array);
  }
}

int NdefRecordToJson(nfc_ndef_record_h record,
    picojson::value& out_value) {
  picojson::object json_record;
  nfc_record_tnf_e tnf;
  int ret = nfc_ndef_record_get_tnf(record, &tnf);
  if (ret != NFC_ERROR_NONE)
    return ret;

  unsigned char* type = 0;
  int type_size = 0;
  nfc_ndef_record_get_type(record, &type, &type_size);

  std::string type_str(reinterpret_cast<char*>(type), type_size);
  SetType(type_str, json_record);
  SetTnf(tnf, json_record);
  SetId(record, json_record);
  SetPayload(record, json_record);

  switch (tnf) {
    case NFC_RECORD_TNF_WELL_KNOWN:
      if (type_str == kTypeText)
        SetTextProperties(record, json_record);
      else if (type_str == kTypeUri)
        SetUriProperties(record, json_record);
      break;
    case NFC_RECORD_TNF_MIME_MEDIA:
      SetType(kJSTypeMedia, json_record);
      SetMIMEProperties(record, json_record);
      break;
    case NFC_RECORD_TNF_URI:
      SetUriProperties(record, json_record);
      break;
  }

  if (ret == NFC_ERROR_NONE)
    out_value = picojson::value(json_record);

  return ret;
}

int NdefMessageToJson(nfc_ndef_message_h message,
    picojson::value& out_value) {
  picojson::value::array records;
  int record_count = 0;
  int ret = nfc_ndef_message_get_record_count(message, &record_count);
  if (ret == NFC_ERROR_NONE) {
    for (int i = 0; i < record_count; i++) {
      nfc_ndef_record_h record = NULL;
      ret = nfc_ndef_message_get_record(message, i, &record);
      picojson::value json_record;
      if (ret == NFC_ERROR_NONE
          && NdefRecordToJson(record, json_record) == NFC_ERROR_NONE)
        records.push_back(json_record);
      else
        break;
    }
  }

  if (ret == NFC_ERROR_NONE)
    out_value = picojson::value(records);
  return ret;
}

int CreateTextRecord(nfc_ndef_record_h* out_ndef_record,
    const picojson::value& record) {
  return nfc_ndef_record_create_text(out_ndef_record,
      record.get("text").to_str().c_str(),
      record.get("languageCode").to_str().c_str(),
      record.get("encoding").to_str() == "UTF-8" ? NFC_ENCODE_UTF_8
                                                 : NFC_ENCODE_UTF_16);
}

int CreateUriRecord(nfc_ndef_record_h* out_ndef_record,
    const picojson::value& record) {
  return nfc_ndef_record_create_uri(out_ndef_record,
      record.get("uri").to_str().c_str());
}

int CreateMediaRecord(nfc_ndef_record_h* out_ndef_record,
    const picojson::value& record) {
  picojson::array payload = record.get("payload").get<picojson::array>();
  size_t payload_size = payload.size();
  unsigned char* data = new unsigned char[payload_size];

  for (size_t i = 0; i < payload_size; i++)
    data[i] = static_cast<unsigned char>(payload[i].get<double>());

  int ret = nfc_ndef_record_create_mime(out_ndef_record,
      record.get("mimeType").to_str().c_str(), data, payload_size);

  delete[] data;
  return ret;
}

int JsonRecordToNdefRecord(const picojson::value& record,
    nfc_ndef_record_h* out_ndef_record) {
  if (!record.is<picojson::object>())
    return NFC_ERROR_INVALID_PARAMETER;

  int ret = NFC_ERROR_NONE;
  std::string recordType = record.get("recordType").to_str();
  if (recordType == kJSTypeText)
    ret = CreateTextRecord(out_ndef_record, record);
  else if (recordType == kJSTypeUri)
    ret = CreateUriRecord(out_ndef_record, record);
  else if (recordType == kJSTypeMedia)
    ret = CreateMediaRecord(out_ndef_record, record);
  else if (recordType == kJSTypeSmartPoster)
    // TODO(shalamov): Not implemented.
    ret = NFC_ERROR_INVALID_PARAMETER;
  else if (recordType == kJSTypeUnknown)
    // TODO(shalamov): Not implemented.
    ret = NFC_ERROR_INVALID_PARAMETER;

  return ret;
}

int JsonMessageToNdefMessage(const picojson::value& message,
    nfc_ndef_message_h* out_ndef_message) {
  if (!message.is<picojson::array>())
    return NFC_ERROR_INVALID_PARAMETER;

  int ret = nfc_ndef_message_create(out_ndef_message);
  if (ret != NFC_ERROR_NONE)
    return ret;

  picojson::array records = message.get<picojson::array>();
  picojson::array::const_iterator it = records.begin();
  picojson::array::const_iterator end = records.end();
  while (it != end) {
    nfc_ndef_record_h ndef_record = NULL;
    ret = JsonRecordToNdefRecord(*it, &ndef_record);
    if (ret == NFC_ERROR_NONE) {
      ret = nfc_ndef_message_append_record(*out_ndef_message, ndef_record);
      it++;
    } else {
      nfc_ndef_message_destroy(*out_ndef_message);
      break;
    }
  }

  return ret;
}

}  // namespace

NfcInstance::NfcInstance()
    : nfc_manager_activated_(nfc_manager_is_activated()) {
  nfc_manager_init_code_ = nfc_manager_initialize_sync();
  nfc_manager_set_activation_changed_cb(OnActivationChangedCallBack, this);
}

NfcInstance::~NfcInstance() {
}

void NfcInstance::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty())
    return;

  if (nfc_manager_init_code_) {
    PostError(v.get("asyncCallId").get<double>());
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "poweron")
    SwitchPower(true, v);
  else if (cmd == "poweroff")
    SwitchPower(false, v);
  else if (cmd == "pollstart")
    PollStart(v);
  else if (cmd == "pollstop")
    PollStop(v);
  else if (cmd == "readNdef")
    ReadNdef(v);
  else if (cmd == "writeNdef")
    WriteNdef(v);
  else if (cmd == "sendNdef")
    SendNdef(v);
  else if (cmd == "startHandover")
    StartHandover(v);
  else if (cmd == "getBytes")
    GetBytes(v);
  else if (cmd == "getPayload")
    GetPayload(v);
  else
    std::cerr << "Received unknown message: " << cmd << "\n";
}

void NfcInstance::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "is_powered")
    SendSyncReply(picojson::value(nfc_manager_activated_).to_str().c_str());
  else
    std::cerr << "Ignoring unknown command: " << cmd;
}

void NfcInstance::SwitchPower(bool state, const picojson::value& value) {
  double async_call_id = value.get("asyncCallId").get<double>();
  CallbackData* cd =
      new CallbackData(this, async_call_id, value.get("cmd").to_str());
  int ret = nfc_manager_set_activation(state, OnPoweredCallBack, cd);
  if (ret != NFC_ERROR_NONE) {
    PostError(async_call_id);
    delete cd;
  }
}

void NfcInstance::PollStart(const picojson::value& value) {
  nfc_manager_set_tag_discovered_cb(OnTagDiscoveredCallBack, this);
  nfc_manager_set_p2p_target_discovered_cb(OnP2PDiscoveredCallBack, this);
  PostSuccess("pollstart", value.get("asyncCallId").get<double>());
}

void NfcInstance::PollStop(const picojson::value& value) {
  nfc_manager_unset_tag_discovered_cb();
  nfc_manager_unset_p2p_target_discovered_cb();
  PostSuccess("pollstop", value.get("asyncCallId").get<double>());
}

void NfcInstance::ReadNdef(const picojson::value& value) {
  double async_id = value.get("asyncCallId").get<double>();
  int ret = NFC_ERROR_NONE;
  if (attached_tag_) {
    bool ndef_supported = false;
    ret = nfc_tag_is_support_ndef(attached_tag_, &ndef_supported);
    if (ret == NFC_ERROR_NONE && ndef_supported) {
      CallbackData* cd = new CallbackData(this, async_id, "readNdef");
      ret = nfc_tag_read_ndef(attached_tag_, OnReadNdefCallBack, cd);
      if (ret != NFC_ERROR_NONE)
        delete cd;
    }
  }

  if (ret != NFC_ERROR_NONE)
    PostError(async_id);
}

void NfcInstance::WriteNdef(const picojson::value& value) {
  double async_id = value.get("asyncCallId").get<double>();
  nfc_ndef_message_h ndef_message = NULL;
  picojson::value message = value.get("message");
  int ret = JsonMessageToNdefMessage(message.get("records"), &ndef_message);
  if (ret == NFC_ERROR_NONE) {
    CallbackData* cd =
        new CallbackData(this, async_id, "writeNdef", ndef_message);
    ret = nfc_tag_write_ndef(attached_tag_, ndef_message,
        OnWriteNdefCallBack, cd);
    if (ret != NFC_ERROR_NONE) {
      nfc_ndef_message_destroy(ndef_message);
      delete cd;
    }
  }

  if (ret != NFC_ERROR_NONE)
    PostError(async_id);
}

void NfcInstance::SendNdef(const picojson::value& value) {
  double async_id = value.get("asyncCallId").get<double>();
  nfc_ndef_message_h ndef_message = NULL;
  picojson::value message = value.get("message");
  int ret = JsonMessageToNdefMessage(message.get("records"), &ndef_message);
  if (ret == NFC_ERROR_NONE) {
    CallbackData* cd =
        new CallbackData(this, async_id, "sendNdef", ndef_message);
    ret = nfc_p2p_send(connected_peer_, ndef_message,
        OnP2PMessageSentCallBack, cd);
    if (ret != NFC_ERROR_NONE) {
      nfc_ndef_message_destroy(ndef_message);
      delete cd;
    }
  }

  if (ret != NFC_ERROR_NONE)
    PostError(async_id);
}

void NfcInstance::StartHandover(const picojson::value& value) {
  // TODO(shalamov): Not implemented.
}

void NfcInstance::GetBytes(const picojson::value& value) {
  // TODO(shalamov): Not implemented.
}

void NfcInstance::GetPayload(const picojson::value& value) {
  double async_id = value.get("asyncCallId").get<double>();
  if (value.contains("record")
     && value.get("record").contains("payload")) {
    PostResult("getPayload",
        value.get("record").get("payload"), async_id);
  } else {
    nfc_ndef_record_h out_ndef_record;
    int ret = JsonRecordToNdefRecord(value.get("record"), &out_ndef_record);
    if (ret == NFC_ERROR_NONE) {
      picojson::object payload;
      SetPayload(out_ndef_record, payload);
      nfc_ndef_record_destroy(out_ndef_record);
      PostResult("getPayload", picojson::value(payload["payload"]), async_id);
    } else {
      PostError(async_id);
    }
  }
}

void NfcInstance::PostError(double async_operation_id) {
  PostResult("asyncCallError", async_operation_id);
}

void NfcInstance::PostSuccess(const std::string& event,
    double async_operation_id) {
  PostResult(event, async_operation_id);
}

void NfcInstance::PostResult(
    const std::string& command,
    double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value(command);
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void NfcInstance::PostResult(
    const std::string& command,
    const picojson::value& result,
    double async_operation_id) {
  picojson::value::object object;
  object["cmd"] = picojson::value(command);
  object["result"] = picojson::value(result);
  object["asyncCallId"] = picojson::value(async_operation_id);
  picojson::value value(object);
  PostMessage(value.serialize().c_str());
}

void NfcInstance::OnActivationChanged(bool activated) {
  nfc_manager_activated_ = activated;
  nfc_manager_activated_ ? PostSuccess("poweron") :
                           PostSuccess("poweroff");
}

void NfcInstance::OnPowered(nfc_error_e error, CallbackData* cd) {
  error == NFC_ERROR_NONE ? PostSuccess(cd->event_name_, cd->async_id_)
                          : PostError(cd->async_id_);
}

void NfcInstance::OnTagDiscovered(nfc_discovered_type_e type, nfc_tag_h tag) {
  if (type == NFC_DISCOVERED_TYPE_ATTACHED) {
    attached_tag_ = tag;
    PostSuccess("tagfound");
  } else {
    attached_tag_ = NULL;
    PostSuccess("taglost");
  }
}

void NfcInstance::OnP2PDiscovered(nfc_discovered_type_e type,
    nfc_p2p_target_h target) {
  if (type == NFC_DISCOVERED_TYPE_ATTACHED) {
    connected_peer_ = target;
    CallbackData* cd = new CallbackData(this, 0, "messageread");
    if (nfc_p2p_set_data_received_cb(connected_peer_,
        OnP2PDataRecievedCallBack, cd) == NFC_ERROR_NONE)
      PostSuccess("peerfound");
    else
      delete cd;
  } else {
    nfc_p2p_unset_data_received_cb(connected_peer_);
    connected_peer_ = NULL;
    PostSuccess("peerlost");
  }
}

void NfcInstance::OnReadNdef(nfc_error_e result,
    nfc_ndef_message_h message, CallbackData* cd) {
  picojson::value retval;
  if (result == NFC_ERROR_NONE
      && NdefMessageToJson(message, retval) == NFC_ERROR_NONE)
    PostResult(cd->event_name_, retval, cd->async_id_);
  else
    PostError(cd->async_id_);
}

void NfcInstance::OnWriteNdef(nfc_error_e result, CallbackData* cd) {
  if (result == NFC_ERROR_NONE) {
    PostSuccess(cd->event_name_, cd->async_id_);
    nfc_ndef_message_destroy(cd->message_);
  } else {
    PostError(cd->async_id_);
  }
}

void NfcInstance::OnP2PDataRecieved(nfc_p2p_target_h target,
    nfc_ndef_message_h message, CallbackData* cd) {
  picojson::value retval;
  if (target == connected_peer_
      && NdefMessageToJson(message, retval) == NFC_ERROR_NONE)
    PostResult(cd->event_name_, retval, cd->async_id_);
}

void NfcInstance::OnP2PMessageSent(nfc_error_e result, CallbackData* cd) {
  if (result == NFC_ERROR_NONE) {
    PostSuccess(cd->event_name_, cd->async_id_);
    nfc_ndef_message_destroy(cd->message_);
  } else {
    PostError(cd->async_id_);
  }
}
