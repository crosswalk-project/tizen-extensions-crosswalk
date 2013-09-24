// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "filesystem/filesystem_context.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

DEFINE_XWALK_EXTENSION(FilesystemContext)

namespace {
const unsigned kDefaultFileMode = 0644;
const std::string kDefaultPath = "/opt/usr/media";

bool IsWritable(const struct stat& st) {
  if (st.st_mode & S_IWOTH)
    return true;
  if ((st.st_mode & S_IWUSR) && geteuid() == st.st_uid)
    return true;
  if ((st.st_mode & S_IWGRP) && getegid() == st.st_gid)
    return true;
  return false;
}

bool IsValidPathComponent(const std::string& path) {
  return path.find('/') == std::string::npos;
}

std::string JoinPath(const std::string& one, const std::string& another) {
  if (!IsValidPathComponent(another))
    return std::string();
  return one + "/" + another;
}

};  // namespace

FilesystemContext::FilesystemContext(ContextAPI* api)
  : api_(api) {}

FilesystemContext::~FilesystemContext() {
  std::set<int>::iterator it;

  for (it = known_file_descriptors_.begin();
        it != known_file_descriptors_.end(); it++)
    close(*it);
}

const char FilesystemContext::name[] = "tizen.filesystem";

extern const char kSource_filesystem_api[];

const char* FilesystemContext::GetJavaScript() {
  return kSource_filesystem_api;
}

void FilesystemContext::HandleMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "FileSystemManagerResolve")
    HandleFileSystemManagerResolve(v);
  else if (cmd == "FileSystemManagerGetStorage")
    HandleFileSystemManagerGetStorage(v);
  else if (cmd == "FileSystemManagerListStorages")
    HandleFileSystemManagerListStorages(v);
  else if (cmd == "FileOpenStream")
    HandleFileOpenStream(v);
  else if (cmd == "FileDeleteDirectory")
    HandleFileDeleteDirectory(v);
  else if (cmd == "FileDeleteFile")
    HandleFileDeleteFile(v);
  else if (cmd == "FileListFiles")
    HandleFileListFiles(v);
  else if (cmd == "FileCopyTo")
    HandleFileCopyTo(v);
  else if (cmd == "FileMoveTo")
    HandleFileMoveTo(v);
  else
    std::cout << "Ignoring unknown command: " << cmd;
}

void FilesystemContext::PostAsyncErrorReply(const picojson::value& msg,
      WebApiAPIErrors error_code) {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_code));
  o["reply_id"] = picojson::value(msg.get("reply_id").get<double>());

  picojson::value v(o);
  api_->PostMessage(v.serialize().c_str());
}

void FilesystemContext::PostAsyncSuccessReply(const picojson::value& msg,
      picojson::value::object& reply) {
  reply["isError"] = picojson::value(false);
  reply["reply_id"] = picojson::value(msg.get("reply_id").get<double>());

  picojson::value v(reply);
  api_->PostMessage(v.serialize().c_str());
}

void FilesystemContext::PostAsyncSuccessReply(const picojson::value& msg) {
  picojson::value::object reply;
  PostAsyncSuccessReply(msg, reply);
}

void FilesystemContext::PostAsyncSuccessReply(const picojson::value& msg,
      picojson::value& value) {
  picojson::value::object reply;
  reply["value"] = value;
  PostAsyncSuccessReply(msg, reply);
}

void FilesystemContext::HandleFileSystemManagerResolve(
      const picojson::value& msg) {
  if (!msg.contains("location")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  std::string mode;
  if (!msg.contains("mode"))
    mode = "rw";
  else
    mode = msg.get("mode").to_str();
  std::string location = msg.get("location").to_str();
  std::string path;
  bool check_if_inside_default = true;
  if (location.find("documents") == 0) {
    path = JoinPath(kDefaultPath, "Documents");
  } else if (location.find("images") == 0) {
    path = JoinPath(kDefaultPath, "Images");
  } else if (location.find("music") == 0) {
    path = JoinPath(kDefaultPath, "Sounds");
  } else if (location.find("videos") == 0) {
    path = JoinPath(kDefaultPath, "Videos");
  } else if (location.find("downloads") == 0) {
    path = JoinPath(kDefaultPath, "Downloads");
  } else if (location.find("ringtones") == 0) {
    path = JoinPath(kDefaultPath, "Sounds");
  } else if (location.find("wgt-package") == 0) {
    if (mode == "w" || mode == "rw") {
      PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
      return;
    }
    path = "/tmp";  // FIXME
  } else if (location.find("wgt-private") == 0) {
    path = "/tmp";  // FIXME
  } else if (location.find("wgt-private-tmp") == 0) {
    path = "/tmp";  // FIXME
  } else if (location.find("file://") == 0) {
    path = location.substr(sizeof("file://") - 1);
    check_if_inside_default = false;
  } else {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  char* real_path_cstr = realpath(path.c_str(), NULL);
  if (!real_path_cstr) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }
  std::string real_path = std::string(real_path_cstr);
  free(real_path_cstr);

  if (check_if_inside_default && real_path.find(kDefaultPath) != 0) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  struct stat st;
  if (stat(path.c_str(), &st) < 0) {
    if (errno == ENOENT || errno == ENOTDIR)
      PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    else
      PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  if (!IsWritable(st) && (mode == "w" || mode == "rw")) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  picojson::value::object o;
  o["realPath"] = picojson::value(real_path);
  PostAsyncSuccessReply(msg, o);
}

void FilesystemContext::HandleFileSystemManagerGetStorage(
      const picojson::value& msg) {
  // FIXME(leandro): This requires specific Tizen support.
  PostAsyncErrorReply(msg, NOT_SUPPORTED_ERR);
}

void FilesystemContext::HandleFileSystemManagerListStorages(
      const picojson::value& msg) {
  // FIXME(leandro): This requires specific Tizen support.
  PostAsyncErrorReply(msg, NOT_SUPPORTED_ERR);
}

void FilesystemContext::HandleFileOpenStream(const picojson::value& msg) {
  if (!msg.contains("mode")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string mode = msg.get("mode").to_str();
  int mode_for_open = 0;
  if (mode == "a") {
    mode_for_open = O_APPEND;
  } else if (mode == "w") {
    mode_for_open = O_TRUNC | O_WRONLY | O_CREAT;
  } else if (mode == "rw") {
    mode_for_open = O_RDWR | O_CREAT;
  } else if (mode == "r") {
    mode_for_open = O_RDONLY;
  } else {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string encoding;
  if (msg.contains("encoding"))
    encoding = msg.get("encoding").to_str();
  if (!encoding.empty() && encoding != "UTF-8") {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  if (!msg.contains("filePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string path = msg.get("filePath").to_str();
  int fd = open(path.c_str(), mode_for_open, kDefaultFileMode);
  if (fd < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
  } else {
    known_file_descriptors_.insert(fd);

    picojson::value::object o;
    o["fileDescriptor"] = picojson::value(static_cast<double>(fd));
    PostAsyncSuccessReply(msg, o);
  }
}

static bool RecursiveDeleteDirectory(const std::string& path) {
  DIR* dir = opendir(path.c_str());
  if (!dir)
    return false;
  struct dirent entry, *buffer;
  int fd = dirfd(dir);
  if (fd < 0)
    goto error;

  while (!readdir_r(dir, &entry, &buffer)) {
    struct stat st;

    if (!buffer)
      break;
    if (!strcmp(entry.d_name, ".") || !strcmp(entry.d_name, ".."))
      continue;
    if (fstatat(fd, entry.d_name, &st, 0) < 0)
      continue;

    if (S_ISDIR(st.st_mode)) {
      const std::string next_path = path + "/" + entry.d_name;
      if (!RecursiveDeleteDirectory(next_path))
        goto error;
    } else if (unlinkat(fd, entry.d_name, 0) < 0) {
      goto error;
    }
  }

  closedir(dir);
  return rmdir(path.c_str()) >= 0;

 error:
  closedir(dir);
  return false;
}

void FilesystemContext::HandleFileDeleteDirectory(const picojson::value& msg) {
  bool recursive = msg.get("recursive").evaluate_as_boolean();

  if (!msg.contains("path")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  std::string path = msg.get("path").to_str();
  if (!IsValidPathComponent(path))  {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  if (recursive) {
    if (!RecursiveDeleteDirectory(path)) {
      PostAsyncErrorReply(msg, IO_ERR);
      return;
    }
  } else if (rmdir(path.c_str()) < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void FilesystemContext::HandleFileDeleteFile(const picojson::value& msg) {
  if (!msg.contains("path") || !msg.contains("filePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string root_path = msg.get("path").to_str();
  std::string relative_path = msg.get("filePath").to_str();
  std::string full_path = JoinPath(root_path, relative_path);
  if (full_path.empty()) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  if (unlink(full_path.c_str()) < 0) {
    switch (errno) {
    case EACCES:
    case EBUSY:
    case EIO:
    case EPERM:
      PostAsyncErrorReply(msg, IO_ERR);
      break;
    case ENOENT:
      PostAsyncErrorReply(msg, NOT_FOUND_ERR);
      break;
    default:
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }
  } else {
    PostAsyncSuccessReply(msg);
  }
}

void FilesystemContext::HandleFileListFiles(const picojson::value& msg) {
  if (!msg.contains("path")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  std::string path = msg.get("path").to_str();
  if (path.empty()) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  DIR* directory = opendir(path.c_str());
  if (!directory) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  picojson::value::array a;

  struct dirent entry, *buffer;
  while (!readdir_r(directory, &entry, &buffer)) {
    if (!buffer)
      break;
    if (!strcmp(entry.d_name, ".") || !strcmp(entry.d_name, ".."))
      continue;

    a.push_back(picojson::value(entry.d_name));
  }

  closedir(directory);

  picojson::value v(a);
  PostAsyncSuccessReply(msg, v);
}


bool FilesystemContext::CopyAndRenameSanityChecks(const picojson::value& msg,
      const std::string& from, const std::string& to, bool overwrite) {
  struct stat destination_st;
  bool destination_exists = true;
  if (stat(to.c_str(), &destination_st) < 0) {
    if (errno == ENOENT) {
      destination_exists = false;
    } else {
      PostAsyncErrorReply(msg, IO_ERR);
      return false;
    }
  }

  if (overwrite && !IsWritable(destination_st)) {
    PostAsyncErrorReply(msg, IO_ERR);
    return false;
  }
  if (!overwrite && destination_exists) {
    PostAsyncErrorReply(msg, IO_ERR);
    return false;
  }

  if (access(from.c_str(), F_OK)) {
    PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    return false;
  }

  return true;
}

namespace {

class PosixFile {
 private:
  int fd_;
  int mode_;
  std::string path_;
  bool unlink_when_done_;
 public:
  PosixFile(const std::string& path, int mode)
      : fd_(open(path.c_str(), mode, kDefaultFileMode))
      , mode_(mode)
      , path_(path)
      , unlink_when_done_(mode & O_CREAT) {}
  ~PosixFile();

  bool is_valid() { return fd_ >= 0; }

  void UnlinkWhenDone(bool setting) { unlink_when_done_ = setting; }

  ssize_t Read(char* buffer, size_t count);
  ssize_t Write(char* buffer, size_t count);
};

PosixFile::~PosixFile() {
  if (fd_ < 0)
    return;

  close(fd_);
  if (unlink_when_done_)
    unlink(path_.c_str());
}

ssize_t PosixFile::Read(char* buffer, size_t count) {
  if (fd_ < 0)
    return -1;

  while (true) {
    ssize_t read_bytes = read(fd_, buffer, count);
    if (read_bytes < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    return read_bytes;
  }
}

ssize_t PosixFile::Write(char* buffer, size_t count) {
  if (fd_ < 0)
    return -1;

  while (true) {
    ssize_t written_bytes = write(fd_, buffer, count);
    if (written_bytes < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    return written_bytes;
  }
}

}  // namespace

void FilesystemContext::HandleFileCopyTo(const picojson::value& msg) {
  if (!msg.contains("originFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("destinationFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string origin_path = msg.get("originFilePath").to_str();
  std::string destination_path = msg.get("destinationFilePath").to_str();
  bool overwrite = msg.get("overwrite").evaluate_as_boolean();

  if (!CopyAndRenameSanityChecks(msg, origin_path, destination_path, overwrite))
    return;

  PosixFile origin(origin_path, O_RDONLY);
  if (!origin.is_valid()) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  PosixFile destination(destination_path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!destination.is_valid()) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  while (true) {
    char buffer[512];
    ssize_t read_bytes = origin.Read(buffer, 512);
    if (!read_bytes)
      break;
    if (read_bytes < 0) {
      PostAsyncErrorReply(msg, IO_ERR);
      return;
    }

    if (destination.Write(buffer, read_bytes) < 0) {
      PostAsyncErrorReply(msg, IO_ERR);
      return;
    }
  }

  destination.UnlinkWhenDone(false);
  PostAsyncSuccessReply(msg);
}

void FilesystemContext::HandleFileMoveTo(const picojson::value& msg) {
  if (!msg.contains("originFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("destinationFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  std::string origin_path = msg.get("originFilePath").to_str();
  std::string destination_path = msg.get("destinationFilePath").to_str();
  bool overwrite = msg.get("overwrite").evaluate_as_boolean();

  if (!CopyAndRenameSanityChecks(msg, origin_path, destination_path, overwrite))
    return;

  if (rename(origin_path.c_str(), destination_path.c_str()) < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void FilesystemContext::HandleSyncMessage(const char* message) {
  picojson::value v;

  std::string err;
  picojson::parse(v, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring sync message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  std::string reply;
  if (cmd == "FileSystemManagerGetMaxPathLength")
    HandleFileSystemManagerGetMaxPathLength(v, reply);
  else if (cmd == "FileStreamClose")
    HandleFileStreamClose(v, reply);
  else if (cmd == "FileStreamRead")
    HandleFileStreamRead(v, reply);
  else if (cmd == "FileStreamReadBytes")
    HandleFileStreamReadBytes(v, reply);
  else if (cmd == "FileStreamReadBase64")
    HandleFileStreamReadBase64(v, reply);
  else if (cmd == "FileStreamWrite")
    HandleFileStreamWrite(v, reply);
  else if (cmd == "FileStreamWriteBytes")
    HandleFileStreamWriteBytes(v, reply);
  else if (cmd == "FileStreamWriteBase64")
    HandleFileStreamWriteBase64(v, reply);
  else if (cmd == "FileCreateDirectory")
    HandleFileCreateDirectory(v, reply);
  else if (cmd == "FileCreateFile")
    HandleFileCreateFile(v, reply);
  else if (cmd == "FileResolve")
    HandleFileResolve(v, reply);
  else if (cmd == "FileStat")
    HandleFileStat(v, reply);
  else if (cmd == "FileGetFullPath")
    HandleFileGetFullPath(v, reply);
  else
    std::cout << "Ignoring unknown command: " << cmd;

  if (!reply.empty())
    api_->SetSyncReply(reply.c_str());
}

static std::string to_string(int value) {
  char buffer[sizeof(value) * 3];
  if (snprintf(buffer, sizeof(buffer), "%d", value) < 0)
    return std::string();
  return std::string(buffer);
}

void FilesystemContext::HandleFileSystemManagerGetMaxPathLength(
      const picojson::value& msg, std::string& reply) {
  int max_path = pathconf("/", _PC_PATH_MAX);
  if (max_path < 0)
    max_path = PATH_MAX;

  std::string max_path_len_str = to_string(max_path);
  SetSyncSuccess(reply, max_path_len_str);
}

bool FilesystemContext::IsKnownFileDescriptor(int fd) {
  return known_file_descriptors_.find(fd) != known_file_descriptors_.end();
}

void FilesystemContext::SetSyncError(std::string& output,
      WebApiAPIErrors error_type) {
  picojson::value::object o;

  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_type));
  picojson::value v(o);
  output = v.serialize();
}

void FilesystemContext::SetSyncSuccess(std::string& reply,
      std::string& output) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);
  o["value"] = picojson::value(output);

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemContext::SetSyncSuccess(std::string& reply) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemContext::SetSyncSuccess(std::string& reply,
      picojson::value& output) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);
  o["value"] = output;

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemContext::HandleFileStreamClose(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fileDescriptor")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  int fd = msg.get("fileDescriptor").get<double>();

  if (IsKnownFileDescriptor(fd)) {
    close(fd);
    known_file_descriptors_.erase(fd);
  }

  SetSyncSuccess(reply);
}

void FilesystemContext::HandleFileStreamRead(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fileDescriptor")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  int fd = msg.get("fileDescriptor").get<double>();

  if (!IsKnownFileDescriptor(fd)) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  unsigned char_count;
  const unsigned kMaxSize = 64 * 1024;
  if (!msg.contains("charCount")) {
    char_count = kMaxSize;
  } else {
    char_count = msg.get("charCount").get<double>();
    if (char_count > kMaxSize) {
      SetSyncError(reply, IO_ERR);
      return;
    }
  }

  char buffer[kMaxSize];
  size_t read_bytes = read(fd, buffer, char_count);
  if (read_bytes < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  std::string buffer_as_string = std::string(buffer, read_bytes);
  SetSyncSuccess(reply, buffer_as_string);
}

void FilesystemContext::HandleFileStreamReadBytes(const picojson::value& msg,
      std::string& reply) {
  HandleFileStreamRead(msg, reply);
}

namespace {
namespace base64 {

static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmn" \
      "opqrstuvwxyz0123456789+/";

std::string ConvertTo(std::string input) {
  std::string encoded;
  size_t input_len = input.length();

  for (size_t i = 0; i < input_len;) {
    unsigned triple = input[i];
    i++;

    triple <<= 8;
    if (i < input_len)
      triple |= input[i];
    i++;

    triple <<= 8;
    if (i < input_len)
      triple |= input[i];
    i++;

    encoded.push_back(chars[(triple & 0xfc0000) >> 18]);
    encoded.push_back(chars[(triple & 0x3f000) >> 12]);
    encoded.push_back((i > input_len + 1) ? '=' : chars[(triple & 0xfc0) >> 6]);
    encoded.push_back((i > input_len) ? '=' : chars[triple & 0x3f]);
  }

  return encoded;
}

int DecodeOne(char c) {
  if (c < '0') {
    if (c == '+')
      return 62;
    if (c == '/')
      return 63;
    return -1;
  }
  if (c <= '9')
    return c - '0' + 52;
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  return -1;
}

std::string ConvertFrom(std::string input) {
  std::string decoded;
  size_t input_len = input.length();

  if (input_len % 4)
    return input;

  for (size_t i = 0; i < input_len;) {
    int c0 = DecodeOne(input[i++]);
    int c1 = DecodeOne(input[i++]);

    if (c0 < 0 || c1 < 0)
      return input;

    int c2 = DecodeOne(input[i++]);
    int c3 = DecodeOne(input[i++]);

    if (c2 < 0 && c3 < 0)
      return input;

    decoded.push_back(c0 << 2 | c1 >> 4);
    if (c2 < 0)
      decoded.push_back(((c1 & 15) << 4) | (c2 >> 2));
    if (c3 < 0)
      decoded.push_back(((c2 & 3) << 6) | c3);
    i += 3;
  }

  return decoded;
}

}  // namespace base64
}  // namespace

void FilesystemContext::HandleFileStreamReadBase64(const picojson::value& msg,
      std::string& reply) {
  HandleFileStreamRead(msg, reply);
  std::string base64_contents = base64::ConvertTo(reply);
  SetSyncSuccess(reply, base64_contents);
}

void FilesystemContext::HandleFileStreamWrite(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fileDescriptor")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  int fd = msg.get("fileDescriptor").get<double>();

  if (!IsKnownFileDescriptor(fd)) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  if (!msg.contains("stringData")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string buffer = msg.get("stringData").to_str();
  if (write(fd, buffer.c_str(), buffer.length()) < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  SetSyncSuccess(reply);
}

void FilesystemContext::HandleFileStreamWriteBytes(const picojson::value& msg,
      std::string& reply) {
  HandleFileStreamWrite(msg, reply);
}

void FilesystemContext::HandleFileStreamWriteBase64(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("base64Data")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string base64_data = msg.get("base64Data").to_str();
  std::string raw_data = base64::ConvertFrom(base64_data);
  HandleFileStreamWrite(msg, raw_data);
}

void FilesystemContext::HandleFileCreateDirectory(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("path")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relative")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string relative_path = msg.get("relative").to_str();
  std::string root_path = msg.get("path").to_str();
  std::string real_path = JoinPath(root_path, relative_path);
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  if (mkdir(real_path.c_str(), kDefaultFileMode) < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  SetSyncSuccess(reply, relative_path);
}

void FilesystemContext::HandleFileCreateFile(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("path")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relative")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string parent = msg.get("path").to_str();
  std::string relative = msg.get("relative").to_str();
  std::string file_path = JoinPath(parent, relative);
  if (file_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  int result = open(file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC,
        kDefaultFileMode);
  if (result < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  close(result);
  SetSyncSuccess(reply, file_path);
}

void FilesystemContext::HandleFileResolve(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("path")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relative")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string path = msg.get("path").to_str();
  std::string relative = msg.get("relative").to_str();
  std::string joined_path = JoinPath(path, relative);
  if (joined_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  char* full_path = realpath(joined_path.c_str(), NULL);
  if (!full_path) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  std::string path_as_str = std::string(full_path);
  free(full_path);

  SetSyncSuccess(reply, path_as_str);
}

void FilesystemContext::HandleFileStat(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("path")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("parent")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string path = msg.get("path").to_str();
  std::string parent = msg.get("parent").to_str();
  std::string full_path = JoinPath(parent, path);
  if (full_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  struct stat st;
  if (stat(full_path.c_str(), &st) < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  picojson::value::object o;
  o["size"] = picojson::value(static_cast<double>(st.st_size));
  o["modified"] = picojson::value(static_cast<double>(st.st_mtime));
  o["created"] = picojson::value(static_cast<double>(st.st_ctime));  // ?
  o["readOnly"] = picojson::value(!IsWritable(st));
  o["isFile"] = picojson::value(!!S_ISREG(st.st_mode));
  o["isDirectory"] = picojson::value(!!S_ISDIR(st.st_mode));

  picojson::value v(o);
  SetSyncSuccess(reply, v);
}

void FilesystemContext::HandleFileGetFullPath(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("path")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string path = msg.get("path").to_str();
  char* full_path = realpath(path.c_str(), NULL);
  if (!full_path) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  std::string full_path_as_str = std::string(full_path);
  free(full_path);

  SetSyncSuccess(reply, full_path_as_str);
}
