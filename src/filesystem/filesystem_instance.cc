// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "filesystem/filesystem_instance.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tzplatform_config.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <utility>

namespace {

const char kPlatformEncoding[] = "UTF-8";
const size_t kBufferSize = 1024 * 4;

unsigned int lastStreamId = 0;

bool IsWritable(const struct stat& st) {
  if (st.st_mode & S_IWOTH)
    return true;
  if ((st.st_mode & S_IWUSR) && geteuid() == st.st_uid)
    return true;
  if ((st.st_mode & S_IWGRP) && getegid() == st.st_gid)
    return true;
  return false;
}

picojson::object StorageToJSON(Storage storage,
    const std::string& label) {
  picojson::object storage_object;
  storage_object["label"] = picojson::value(label);
  storage_object["type"] = picojson::value(storage.GetType());
  storage_object["state"] = picojson::value(storage.GetState());
  return storage_object;
}

}  // namespace

FilesystemInstance::FilesystemInstance() {
}

void FilesystemInstance::Initialize() {
  vfs_.SetOnStorageChangedCb(OnStorageStateChanged, this);
}

FilesystemInstance::~FilesystemInstance() {
  FStreamMap::iterator it;

  for (it = fstream_map_.begin(); it != fstream_map_.end(); it++) {
    std::fstream* fs = std::get<1>(it->second);
    fs->close();
    delete(fs);
  }
}

void FilesystemInstance::HandleMessage(const char* message) {
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

void FilesystemInstance::PostAsyncErrorReply(const picojson::value& msg,
      WebApiAPIErrors error_code) {
  picojson::value::object o;
  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_code));
  o["reply_id"] = picojson::value(msg.get("reply_id").get<double>());

  picojson::value v(o);
  PostMessage(v.serialize().c_str());
}

void FilesystemInstance::PostAsyncSuccessReply(const picojson::value& msg,
      picojson::value::object& reply) {
  reply["isError"] = picojson::value(false);
  reply["reply_id"] = picojson::value(msg.get("reply_id").get<double>());

  picojson::value v(reply);
  PostMessage(v.serialize().c_str());
}

void FilesystemInstance::PostAsyncSuccessReply(const picojson::value& msg) {
  picojson::value::object reply;
  PostAsyncSuccessReply(msg, reply);
}

void FilesystemInstance::PostAsyncSuccessReply(const picojson::value& msg,
      picojson::value& value) {
  picojson::value::object reply;
  reply["value"] = value;
  PostAsyncSuccessReply(msg, reply);
}

void FilesystemInstance::HandleFileSystemManagerResolve(
      const picojson::value& msg) {
  if (!msg.contains("location")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string location = msg.get("location").to_str();
  bool check_if_inside_default = true;
  std::string mode;

  mode = msg.contains("mode") ? msg.get("mode").to_str() : "rw";

  size_t pos_wgt_pkg = location.find(vfs_const::kLocationWgtPackage);
  size_t pos_ringtones = location.find(vfs_const::kLocationRingtones);

  if (pos_wgt_pkg != std::string::npos || pos_ringtones != std::string::npos) {
    if (mode == "w" || mode == "rw" || mode == "a") {
      PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
      return;
    }
    mode = "r";
  }

  if (pos_wgt_pkg != std::string::npos ||
      pos_ringtones != std::string::npos ||
      location.find(vfs_const::kLocationWgtPrivate) != std::string::npos)
    check_if_inside_default = false;

  std::string real_path;
  if (location.find("file://") == 0) {
    real_path = location.substr(sizeof("file://") - 1);
    check_if_inside_default = false;
  } else {
    real_path = vfs_.GetRealPath(location);
  }

  if (real_path.empty()) {
    PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    return;
  }

  char* real_path_cstr = realpath(real_path.c_str(), NULL);
  if (!real_path_cstr) {
    if (errno == ENOENT)
      PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    else
      PostAsyncErrorReply(msg, IO_ERR);
    return;
  }
  std::string real_path_ack = std::string(real_path_cstr);
  free(real_path_cstr);

  if (check_if_inside_default &&
      real_path_ack.find(
          tzplatform_getenv(TZ_USER_CONTENT)) == std::string::npos) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  struct stat st;
  if (stat(real_path_ack.c_str(), &st) < 0) {
    if (errno == ENOENT || errno == ENOTDIR) {
      PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    } else {
      PostAsyncErrorReply(msg, IO_ERR);
    }
    return;
  }

  if (!IsWritable(st) && (mode == "w" || mode == "rw")) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }
  picojson::value::object o;
  o["fullPath"] = picojson::value(location);
  PostAsyncSuccessReply(msg, o);
}

void FilesystemInstance::HandleFileSystemManagerGetStorage(
      const picojson::value& msg) {
  Storage storage;
  std::string label = msg.get("label").to_str();
  if (!vfs_.GetStorageByLabel(label, storage)) {
    PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    return;
  }

  picojson::object storage_object = StorageToJSON(storage, label);
  PostAsyncSuccessReply(msg, storage_object);
}

void FilesystemInstance::HandleFileSystemManagerListStorages(
      const picojson::value& msg) {
  picojson::array storage_objects;
  Storages::const_iterator it = vfs_.begin();
  while (it != vfs_.end()) {
    picojson::object storage_object = StorageToJSON(it->second, it->first);
    storage_objects.push_back(picojson::value(storage_object));
    ++it;
  }

  picojson::value value(storage_objects);
  PostAsyncSuccessReply(msg, value);
}

void FilesystemInstance::HandleFileOpenStream(const picojson::value& msg) {
  if (!msg.contains("mode")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string mode = msg.get("mode").to_str();
  std::ios_base::openmode open_mode = std::ios_base::binary;
  if (mode == "a") {
    open_mode |= (std::ios_base::app | std::ios_base::out);
  } else if (mode == "w") {
    open_mode |= std::ios_base::out;
  } else if (mode == "rw") {
    open_mode |= (std::ios_base::in | std::ios_base::out);
  } else if (mode == "r") {
    open_mode |= std::ios_base::in;
  } else {
    PostAsyncErrorReply(msg, TYPE_MISMATCH_ERR);
    return;
  }

  std::string encoding = "";
  if (msg.contains("encoding"))
    encoding = msg.get("encoding").to_str();
  if (encoding.empty()) {
    encoding = kPlatformEncoding;
  }
  // is the encoding supported by iconv?
  iconv_t cd = iconv_open("UTF-8", encoding.c_str());
  if (cd == reinterpret_cast<iconv_t>(-1)) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    iconv_close(cd);
    return;
  }
  iconv_close(cd);
  if (!msg.contains("fullPath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(msg.get("fullPath").to_str());
  char* real_path_cstr = realpath(real_path.c_str(), NULL);
  if (!real_path_cstr) {
    free(real_path_cstr);
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  struct stat st;
  if (stat(real_path_cstr, &st) < 0) {
    free(real_path_cstr);
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  if (S_ISDIR(st.st_mode)) {
    free(real_path_cstr);
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }
  std::fstream* fs = new std::fstream(real_path_cstr, open_mode);
  if (!(*fs) || !fs->is_open()) {
    free(real_path_cstr);
    delete fs;
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  free(real_path_cstr);

  fstream_map_[lastStreamId] = FStream(open_mode, fs, encoding);

  picojson::value::object o;
  o["streamID"] = picojson::value(static_cast<double>(lastStreamId));
  lastStreamId++;
  PostAsyncSuccessReply(msg, o);
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

void FilesystemInstance::HandleFileDeleteDirectory(const picojson::value& msg) {
  if (!msg.contains("directoryPath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  bool recursive = msg.get("recursive").evaluate_as_boolean();
  std::string real_path = vfs_.GetRealPath(msg.get("directoryPath").to_str());
  if (real_path.empty()) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  if (recursive) {
    if (!RecursiveDeleteDirectory(real_path)) {
      PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
      return;
    }
  } else if (rmdir(real_path.c_str()) < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void FilesystemInstance::HandleFileDeleteFile(const picojson::value& msg) {
  if (!msg.contains("filePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(msg.get("filePath").to_str());
  if (real_path.empty()) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  if (unlink(real_path.c_str()) < 0) {
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
    case EISDIR:
      PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
      break;
    default:
      PostAsyncErrorReply(msg, UNKNOWN_ERR);
    }
  } else {
    PostAsyncSuccessReply(msg);
  }
}

void FilesystemInstance::HandleFileListFiles(const picojson::value& msg) {
  if (!msg.contains("fullPath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(msg.get("fullPath").to_str());
  if (real_path.empty()) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  DIR* directory = opendir(real_path.c_str());
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

    a.push_back(picojson::value(VirtualFS::JoinPath(
        msg.get("fullPath").to_str(), entry.d_name)));
  }

  closedir(directory);

  picojson::value v(a);
  PostAsyncSuccessReply(msg, v);
}

std::string FilesystemInstance::ResolveImplicitDestination(
    const std::string& from, const std::string& to) {
  // Resolve implicit destination paths
  // Sanity checks are done later, in CopyAndRenameSanityChecks
  if (to.empty())
    return "";

  std::string explicit_to = to;
  if (*to.rbegin() == '/' || *to.rbegin() == '\\') {
    // 1. hinted paths
    std::string::size_type found = from.find_last_of("/\\");
    explicit_to.append(from.substr(found + 1));
  } else {
    // 2. no hint, apply heuristics on path types
    // i.e. if we copy a file to a directory, we copy it into that directory
    // as a file with the same name
    struct stat to_st;
    struct stat from_st;
    if (stat(from.c_str(), &from_st) == 0 &&
        stat(to.c_str(), &to_st) == 0 &&
        S_ISREG(from_st.st_mode) &&
        S_ISDIR(to_st.st_mode)) {
      std::string::size_type found = from.find_last_of("/\\");
      explicit_to.append(from.substr(found));  // including '/' to join
    }
  }
  return explicit_to;
}

bool FilesystemInstance::CopyAndRenameSanityChecks(const picojson::value& msg,
    const std::string& from, const std::string& to, bool overwrite) {
  if (from.empty() || to.empty()) {
    PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    return false;
  }
  bool destination_file_exists = true;
  if (access(to.c_str(), F_OK) < 0) {
    if (errno == ENOENT) {
      destination_file_exists = false;
    } else {
      PostAsyncErrorReply(msg, IO_ERR);
      std::cerr << "destination unreachable\n";
      return false;
    }
  }

  std::string::size_type found = to.find_last_of("/\\");
  struct stat destination_parent_st;
  if (stat(to.substr(0, found).c_str(), &destination_parent_st) < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    std::cerr << "parent of destination does not exist\n";
    return false;
  }

  if (overwrite && !IsWritable(destination_parent_st)) {
    PostAsyncErrorReply(msg, IO_ERR);
    std::cerr << "parent of destination is not writable (overwrite is true)\n";
    return false;
  }
  if (!overwrite && destination_file_exists) {
    PostAsyncErrorReply(msg, IO_ERR);
    std::cerr << "destination exists and overwrite is false\n";
    return false;
  }

  if (access(from.c_str(), F_OK)) {
    PostAsyncErrorReply(msg, NOT_FOUND_ERR);
    std::cerr << "origin does not exist\n";
    return false;
  }

  // don't copy or move into itself
  if (to.length() >= from.length() && to.compare(0, from.length(), from) == 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    std::cerr << "won't copy/move into itself\n";
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
      : fd_(open(path.c_str(), mode, vfs_const::kDefaultFileMode))
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

bool CopyElement(const std::string &from, const std::string &to) {
  struct stat from_st;
  // element is a file
  if (stat(from.c_str(), &from_st) == 0 && S_ISREG(from_st.st_mode)) {
    PosixFile origin(from, O_RDONLY);
    if (!origin.is_valid()) {
      std::cerr << "from: " << from << " is invalid\n";
      return false;
    }

    PosixFile destination(to, O_WRONLY | O_CREAT | O_TRUNC);
    if (!destination.is_valid()) {
      std::cerr << "to: " << to << " is invalid\n";
      return false;
    }

    while (true) {
      char buffer[kBufferSize];
      ssize_t read_bytes = origin.Read(buffer, kBufferSize);
      if (!read_bytes)
        break;
      if (read_bytes < 0) {
        std::cerr << "read error\n";
        return false;
      }

      if (destination.Write(buffer, read_bytes) < 0) {
        std::cerr << "write error\n";
        return false;
      }
    }

    destination.UnlinkWhenDone(false);
    return true;
  }  // end file case

  // element is a directory, create if not exists
  int status = mkdir(to.c_str(), vfs_const::kDefaultFileMode);
  if (status != 0 && errno != EEXIST) {
    std::cerr << "failed to create destination dir: " << to << std::endl;
    return false;
  }
  // recursively copy content
  DIR* dir;
  dir = opendir(from.c_str());
  dirent* elt;
  while ((elt = readdir(dir)) != NULL) {
    if (!strcmp(elt->d_name, ".") || !strcmp(elt->d_name, ".."))
      continue;
    const std::string filename = elt->d_name;
    const std::string full_origin = from + "/" + filename;
    const std::string full_destination = to + "/" + filename;
    if (!CopyElement(full_origin, full_destination)) {
      closedir(dir);
      return false;
    }
  }
  closedir(dir);
  return true;
}

}  // namespace

void FilesystemInstance::HandleFileCopyTo(const picojson::value& msg) {
  if (!msg.contains("originFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("destinationFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  bool overwrite = msg.get("overwrite").evaluate_as_boolean();
  std::string real_origin_path =
      vfs_.GetRealPath(msg.get("originFilePath").to_str());
  std::string real_destination_path =
      ResolveImplicitDestination(real_origin_path,
      vfs_.GetRealPath(msg.get("destinationFilePath").to_str()));

  if (!CopyAndRenameSanityChecks(msg, real_origin_path, real_destination_path,
                                 overwrite))
    return;
  if (CopyElement(real_origin_path, real_destination_path))
    PostAsyncSuccessReply(msg);
  else
    PostAsyncErrorReply(msg, IO_ERR);
}

void FilesystemInstance::HandleFileMoveTo(const picojson::value& msg) {
  if (!msg.contains("originFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("destinationFilePath")) {
    PostAsyncErrorReply(msg, INVALID_VALUES_ERR);
    return;
  }

  bool overwrite = msg.get("overwrite").evaluate_as_boolean();
  std::string real_origin_path =
      vfs_.GetRealPath(msg.get("originFilePath").to_str());
  std::string real_destination_path =
      ResolveImplicitDestination(real_origin_path,
      vfs_.GetRealPath(msg.get("destinationFilePath").to_str()));

  if (!CopyAndRenameSanityChecks(msg, real_origin_path, real_destination_path,
                                 overwrite))
    return;

  if (rename(real_origin_path.c_str(), real_destination_path.c_str()) < 0) {
    PostAsyncErrorReply(msg, IO_ERR);
    return;
  }

  PostAsyncSuccessReply(msg);
}

void FilesystemInstance::HandleSyncMessage(const char* message) {
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
  else if (cmd == "FileStreamWrite")
    HandleFileStreamWrite(v, reply);
  else if (cmd == "FileCreateDirectory")
    HandleFileCreateDirectory(v, reply);
  else if (cmd == "FileCreateFile")
    HandleFileCreateFile(v, reply);
  else if (cmd == "FileGetURI")
    HandleFileGetURI(v, reply);
  else if (cmd == "FileResolve")
    HandleFileResolve(v, reply);
  else if (cmd == "FileStat")
    HandleFileStat(v, reply);
  else if (cmd == "FileStreamStat")
    HandleFileStreamStat(v, reply);
  else if (cmd == "FileStreamSetPosition")
    HandleFileStreamSetPosition(v, reply);
  else
    std::cout << "Ignoring unknown command: " << cmd << std::endl;
  if (!reply.empty())
    SendSyncReply(reply.c_str());
}

void FilesystemInstance::HandleFileSystemManagerGetMaxPathLength(
      const picojson::value& msg, std::string& reply) {
  int max_path = pathconf("/", _PC_PATH_MAX);
  if (max_path < 0)
    max_path = PATH_MAX;

  picojson::value value(static_cast<double>(max_path));
  SetSyncSuccess(reply, value);
}

bool FilesystemInstance::IsKnownFileStream(const picojson::value& msg) {
  if (!msg.contains("streamID"))
    return false;
  unsigned int key = msg.get("streamID").get<double>();

  return fstream_map_.find(key) != fstream_map_.end();
}

std::fstream* FilesystemInstance::GetFileStream(unsigned int key) {
  FStreamMap::iterator it = fstream_map_.find(key);
  if (it == fstream_map_.end())
    return NULL;
  std::fstream* fs = std::get<1>(it->second);

  if (fs->is_open())
    return fs;
  return NULL;
}

std::string FilesystemInstance::GetFileEncoding(unsigned int key) const {
  FStreamMap::const_iterator it = fstream_map_.find(key);
  if (it == fstream_map_.end())
    return kPlatformEncoding;
  return std::get<2>(it->second);
}

std::fstream* FilesystemInstance::GetFileStream(unsigned int key,
    std::ios_base::openmode mode) {
  FStreamMap::iterator it = fstream_map_.find(key);
  if (it == fstream_map_.end())
    return NULL;

  if ((std::get<0>(it->second) & mode) != mode)
    return NULL;

  std::fstream* fs = std::get<1>(it->second);

  if (fs->is_open())
    return fs;
  return NULL;
}

void FilesystemInstance::SetSyncError(std::string& output,
      WebApiAPIErrors error_type) {
  picojson::value::object o;

  o["isError"] = picojson::value(true);
  o["errorCode"] = picojson::value(static_cast<double>(error_type));
  picojson::value v(o);
  output = v.serialize();
}

void FilesystemInstance::SetSyncSuccess(std::string& reply,
      std::string& output) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);
  o["value"] = picojson::value(output);

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemInstance::SetSyncSuccess(std::string& reply) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemInstance::SetSyncSuccess(std::string& reply,
      picojson::value& output) {
  picojson::value::object o;

  o["isError"] = picojson::value(false);
  o["value"] = output;

  picojson::value v(o);
  reply = v.serialize();
}

void FilesystemInstance::HandleFileStreamClose(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("streamID")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  unsigned int key = msg.get("streamID").get<double>();

  FStreamMap::iterator it = fstream_map_.find(key);
  if (it != fstream_map_.end()) {
    std::fstream* fs = std::get<1>(it->second);
    if (fs->is_open())
      fs->close();
    delete fs;
    fstream_map_.erase(it);
  }

  SetSyncSuccess(reply);
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
  int input_len = input.length();

  if (input_len % 4)
    return input;

  int i;
  int c = 0;
  int decoded_bits = 0;
  for (i = 0; i < input_len;) {
    c = input[i++];
    if (c == '=')
      break;
    if (c > 255)
      continue;
    int decoded_byte = DecodeOne(c);
    if (decoded_byte < 0)
      continue;

    decoded_bits |= decoded_byte;

    if (i % 4 == 0) {
      decoded.push_back(static_cast<char>(decoded_bits >> 16));
      decoded.push_back(static_cast<char>(decoded_bits >> 8));
      decoded.push_back(static_cast<char>(decoded_bits));
      decoded_bits = 0;
    } else {
      decoded_bits <<= 6;
    }
  }

  if (c == '=') {
    switch (input_len - i) {
    case 0:
      return input;
    case 1:
      decoded.push_back(static_cast<char>(decoded_bits >> 10));
      break;
    case 2:
      decoded.push_back(static_cast<char>(decoded_bits >> 16));
      decoded.push_back(static_cast<char>(decoded_bits >> 8));
      break;
    }
  }

  return decoded;
}

}  // namespace base64

}  // namespace

void FilesystemInstance::HandleFileStreamRead(const picojson::value& msg,
      std::string& reply) {
  if (!IsKnownFileStream(msg)) {
    SetSyncError(reply, IO_ERR);
    return;
  }
  unsigned int key = msg.get("streamID").get<double>();

  std::streamsize count;
  if (msg.contains("count")) {
    count = msg.get("count").get<double>();
  } else {
    // count is not optional
    SetSyncError(reply, IO_ERR);
    return;
  }
  std::fstream* fs = GetFileStream(key, std::ios_base::in);
  if (!fs) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  if (msg.get("type").to_str() == "Default") {
    // we want decoded text data
    // depending on encoding, a character (a.k.a. a glyph) may take
    // one or several bytes in input and in output as well.
    std::string encoding = GetFileEncoding(key);
    ReadText(fs, count, encoding.c_str(), reply);
    return;
  }
  // we want binary data
  std::string buffer;
  buffer.resize(count);
  fs->read(&buffer[0], count);
  std::streamsize bytes_read = fs->gcount();
  fs->clear();
  if (fs->bad()) {
    fs->clear();
    SetSyncError(reply, IO_ERR);
    return;
  }
  buffer.resize(bytes_read);

  if (msg.get("type").to_str() == "Bytes") {
    // return binary data as numeric array
    picojson::value::array a;

    for (int i = 0; i < bytes_read; i++) {
        a.push_back(picojson::value(static_cast<double>(buffer[i])));
    }

    picojson::value v(a);
    SetSyncSuccess(reply, v);
    return;
  }

  if (msg.get("type").to_str() == "Base64") {
    // return binary data as Base64 encoded string
    std::string base64_buffer = base64::ConvertTo(buffer);
    SetSyncSuccess(reply, base64_buffer);
    return;
  }

  SetSyncSuccess(reply, buffer);
}

void FilesystemInstance::HandleFileStreamWrite(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("data")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  if (!IsKnownFileStream(msg)) {
    SetSyncError(reply, IO_ERR);
    return;
  }
  unsigned int key = msg.get("streamID").get<double>();

  std::fstream* fs = GetFileStream(key, std::ios_base::out);
  if (!fs) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  std::string buffer;
  if (msg.get("type").to_str() == "Bytes") {
    picojson::array a = msg.get("data").get<picojson::array>();
    for (picojson::array::iterator iter = a.begin(); iter != a.end(); ++iter)
      buffer.append<int>(1, (*iter).get<double>());
  } else if (msg.get("type").to_str() == "Base64") {
    buffer = base64::ConvertFrom(msg.get("data").to_str());
  } else {
    // text mode
    std::string text = msg.get("data").to_str();
    std::string encoding = GetFileEncoding(key);
    if (encoding != "UTF-8" && encoding != "utf-8") {
      // transcode
      iconv_t cd = iconv_open(encoding.c_str(), "UTF-8");
      char encode_buf[kBufferSize];
      // ugly cast for inconsistent iconv prototype
      char* in_p = const_cast<char*>(text.data());
      size_t in_bytes_left = text.length();
      while (in_bytes_left > 0) {
        char* out_p = encode_buf;
        size_t out_bytes_free = kBufferSize;
        size_t icnv = iconv(cd, &in_p, &in_bytes_left, &out_p, &out_bytes_free);
        if (icnv == static_cast<size_t>(-1)) {
          switch (errno) {
            case E2BIG:
              // expected case if encode_buf is full
              break;
            case EINVAL:
            case EILSEQ:
            default:
              iconv_close(cd);
              SetSyncError(reply, IO_ERR);
              return;
          }
        }
        buffer.append(encode_buf, kBufferSize-out_bytes_free);
      }
      iconv_close(cd);
    } else {
      buffer = text;
    }
  }

  if (!((*fs) << buffer)) {
    fs->clear();
    SetSyncError(reply, IO_ERR);
    return;
  }
  fs->flush();
  SetSyncSuccess(reply);
}

void FilesystemInstance::HandleFileCreateDirectory(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fullPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relativeDirPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string full_path = VirtualFS::JoinPath(msg.get("fullPath").to_str(),
                                   msg.get("relativeDirPath").to_str());
  if (full_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(full_path);
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  if (!VirtualFS::MakePath(real_path, vfs_const::kDefaultFileMode)) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  SetSyncSuccess(reply, full_path);
}

void FilesystemInstance::HandleFileCreateFile(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fullPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relativeFilePath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string full_path = VirtualFS::JoinPath(
      msg.get("fullPath").to_str(),
      msg.get("relativeFilePath").to_str());
  if (full_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(full_path);
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  int result = open(real_path.c_str(), O_CREAT | O_WRONLY | O_EXCL,
      vfs_const::kDefaultFileMode);
  if (result < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  close(result);
  SetSyncSuccess(reply, full_path);
}

void FilesystemInstance::HandleFileGetURI(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fullPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  std::string full_path = msg.get("fullPath").to_str();

  std::string real_path = vfs_.GetRealPath(full_path);
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  char* real_path_c = realpath(real_path.c_str(), NULL);
  if (!real_path_c) {
    SetSyncError(reply, NOT_FOUND_ERR);
    return;
  }
  free(real_path_c);

  std::string uri_path = VirtualFS::JoinPath("file:/", full_path);

  SetSyncSuccess(reply, uri_path);
}

void FilesystemInstance::HandleFileResolve(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fullPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!msg.contains("relativeFilePath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string full_path = VirtualFS::JoinPath(msg.get("fullPath").to_str(),
      msg.get("relativeFilePath").to_str());
  if (full_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(full_path);
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  char* real_path_c = realpath(real_path.c_str(), NULL);
  if (!real_path_c) {
    SetSyncError(reply, NOT_FOUND_ERR);
    return;
  }
  free(real_path_c);

  SetSyncSuccess(reply, full_path);
}

void FilesystemInstance::HandleFileStat(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("fullPath")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  std::string real_path = vfs_.GetRealPath(msg.get("fullPath").to_str());
  if (real_path.empty()) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }

  struct stat st;
  if (stat(real_path.c_str(), &st) < 0) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  bool is_directory = !!S_ISDIR(st.st_mode);

  picojson::value::object o;
  o["size"] = picojson::value(static_cast<double>(st.st_size));
  o["modified"] = picojson::value(static_cast<double>(st.st_mtime));
  o["created"] = picojson::value(static_cast<double>(st.st_ctime));  // ?
  o["readOnly"] = picojson::value(!IsWritable(st));
  o["isFile"] = picojson::value(!!S_ISREG(st.st_mode));
  o["isDirectory"] = picojson::value(is_directory);
  if (is_directory)
    o["length"] = picojson::value(
        static_cast<double>(VirtualFS::GetDirEntryCount(real_path.c_str())));


  picojson::value v(o);
  SetSyncSuccess(reply, v);
}

void FilesystemInstance::HandleFileStreamStat(const picojson::value& msg,
      std::string& reply) {
  if (!IsKnownFileStream(msg)) {
    SetSyncError(reply, IO_ERR);
    return;
  }
  unsigned int key = msg.get("streamID").get<double>();

  std::fstream* fs = GetFileStream(key);
  if (!fs) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  std::streampos fsize = 0;
  if (!fs->eof()) {
    std::streampos initial_pos = fs->tellg();
    fs->seekg(0, std::ios::end);
    fsize = fs->tellg() - initial_pos;
    if (fs->bad()) {
      fs->clear();
      SetSyncError(reply, IO_ERR);
      return;
    }
    // Recover the position.
    fs->clear();
    fs->seekg(initial_pos);
    if (fs->bad()) {
      fs->clear();
      SetSyncError(reply, IO_ERR);
      return;
    }
  }
  picojson::value::object o;
  o["position"] = picojson::value(static_cast<double>(fs->tellg()));
  o["eof"] = picojson::value(fs->eof());
  o["bytesAvailable"] = picojson::value(static_cast<double>(fsize));

  picojson::value v(o);
  SetSyncSuccess(reply, v);
}

void FilesystemInstance::HandleFileStreamSetPosition(const picojson::value& msg,
      std::string& reply) {
  if (!msg.contains("position")) {
    SetSyncError(reply, INVALID_VALUES_ERR);
    return;
  }
  if (!IsKnownFileStream(msg)) {
    SetSyncError(reply, IO_ERR);
    return;
  }
  unsigned int key = msg.get("streamID").get<double>();

  std::fstream* fs = GetFileStream(key);
  if (!fs) {
    SetSyncError(reply, IO_ERR);
    return;
  }

  int position = msg.get("position").get<double>();
  fs->seekg(position);
  if (fs->bad()) {
    fs->clear();
    SetSyncError(reply, IO_ERR);
    return;
  }
  SetSyncSuccess(reply);
}

namespace {

/**
 * Request req_char_num characters (glyphs) from buffer s of length slen.
 * Set actual_char_num to the number of actually available characters
 * (actual_char_num <= req_char_num)
 * Set bytes_num to the number of corresponding bytes in the buffer.
 * Beware! the buffer must contain complete and valid UTF-8 character sequences.
 */
bool GetCharsFromBytes(char* s, int slen, int req_char_num,
    size_t* actual_char_num, size_t* bytes_num) {
  int i = 0;
  int j = 0;
  while (i < slen && j < req_char_num) {
    if (s[i] > 0) {
      i++;
    } else if ((s[i] & 0xE0) == 0xC0) {
      i += 2;
    } else if ((s[i] & 0xF0) == 0xE0) {
      i += 3;
    } else if ((s[i] & 0xF8) == 0xF0) {
      i += 4;
    // these should never happen (restriction of unicode under 0x10FFFF)
    // but belong to the UTF-8 standard yet.
    } else if ((s[i] & 0xFC) == 0xF8) {
      i += 5;
    } else if ((s[i] & 0xFE) == 0xFC) {
      i += 6;
    } else {
      std::cerr << "Invalid UTF-8!" << std::endl;
      return false;
    }
    j++;
  }
  (*actual_char_num) = j;
  (*bytes_num) = i;
  return true;
}

}  // namespace

void FilesystemInstance::ReadText(std::fstream* file, size_t num_chars,
    const char* encoding, std::string& reply) {
  iconv_t cd = iconv_open("UTF-8", encoding);

  char inbuffer[kBufferSize];
  char utf8buffer[kBufferSize];
  std::string out;
  int strlength = 0;

  bool out_of_space = false;
  bool partial_remains = false;
  // number of bytes already at start of buffer
  size_t offset = 0;
  // As we can't predict how much data to read, we may convert more
  // data than needed. Keep track of excess (converted) bytes in utf8buffer.
  size_t excess_offset = 0;
  size_t excess_len = 0;
  std::streampos original_pos = file->tellg();

  while (strlength < num_chars && !file->eof()) {
    file->read(inbuffer + offset, kBufferSize - offset);
    size_t src_bytes_left = file->gcount()+offset;

    char* in_p = inbuffer;
    do {
      char* utf8_p = utf8buffer;
      size_t utf8_bytes_free = kBufferSize;
      out_of_space = false;
      partial_remains = false;

      size_t icnv = iconv(cd, &in_p, &src_bytes_left, &utf8_p,
          &utf8_bytes_free);

      if (icnv == static_cast<size_t>(-1)) {
        switch (errno) {
          case E2BIG:
            out_of_space = true;
            break;
          case EINVAL:
            partial_remains = true;
            break;
          case EILSEQ:
          default:
            iconv_close(cd);
            // restore filepos
            file->clear();
            file->seekg(original_pos);
            SetSyncError(reply, IO_ERR);
            return;
        }
      }
      int missing = num_chars - strlength;
      size_t available = 0;
      size_t datalen;
      size_t utf8load = kBufferSize - utf8_bytes_free;
      if (!GetCharsFromBytes(utf8buffer, utf8load,
          missing, &available, &datalen)) {
        iconv_close(cd);
        // restore filepos
        file->clear();
        file->seekg(original_pos);
        SetSyncError(reply, IO_ERR);
        return;
      }
      out.append(utf8buffer, datalen);
      strlength += available;
      if (datalen < utf8load) {
        excess_offset = datalen;
        excess_len = utf8load - datalen;
        out_of_space = false;
        partial_remains = (src_bytes_left > 0);
      }
    } while (out_of_space);

    if (partial_remains) {
      // some bytes remains at the end of the buffer that were not converted
      // move them to the beginning of the inbuffer before completing with data
      // from disk
      if (strlength < num_chars)
        memmove(inbuffer, inbuffer + kBufferSize - src_bytes_left,
            src_bytes_left);
      offset = src_bytes_left;
    } else {
      offset = 0;
    }
  }

  iconv_close(cd);
  std::streampos back_jump = 0;
  if (offset > 0) {
    back_jump = offset;
  }
  if (excess_len > 0) {
    // we've read too much, so reposition the file
    // first, convert back(!) to compute input data size
    cd = iconv_open(encoding, "UTF-8");
    char* in_p = utf8buffer + excess_offset;
    char* out_p = inbuffer;
    size_t free_bytes = kBufferSize;
    iconv(cd, &in_p, &excess_len, &out_p, &free_bytes);
    if (!strcasecmp(encoding, "UTF16") || !strcasecmp(encoding, "UTF-16")) {
      // don't count the BOM added by iconv
      free_bytes += 2;
    }
    back_jump += (kBufferSize-free_bytes);
    iconv_close(cd);
  }
  if (back_jump > 0) {
    file->clear();
    file->seekg(file->tellg()-back_jump);
  }
  SetSyncSuccess(reply, out);
  return;
}

void FilesystemInstance::NotifyStorageStateChanged(const std::string& label,
    Storage storage) {
  picojson::object reply;
  reply["storage"] = picojson::value(StorageToJSON(storage, label));
  reply["cmd"] = picojson::value("storageChanged");
  picojson::value value(reply);
  PostMessage(value.serialize().c_str());
}

void FilesystemInstance::OnStorageStateChanged(const std::string& label,
    Storage storage, void* user_data) {
  reinterpret_cast<FilesystemInstance*>(user_data)->NotifyStorageStateChanged(
      label, storage);
}
