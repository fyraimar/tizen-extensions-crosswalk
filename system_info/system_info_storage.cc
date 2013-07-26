// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_storage.h"

#include <stdlib.h>
#include <sys/statvfs.h>
#include <mntent.h>

#include "common/picojson.h"

#define MOUNT_TABLE "/proc/mounts"

SysInfoStorage::SysInfoStorage(picojson::value& error) {
  picojson::object& error_map = error.get<picojson::object>();

  _udev = udev_new();
  if (!_udev) {
    error_map["message"] = picojson::value("Can't create udev");
  }
}

SysInfoStorage::~SysInfoStorage() {
  if(_udev)
    udev_unref(_udev);
}

void SysInfoStorage::Update(picojson::value& error,
                            picojson::value& data) {
  struct mntent *ent;
  FILE *aFile;

  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  picojson::value units = picojson::value(picojson::array(0));
  picojson::array& units_arr = units.get<picojson::array>();

  aFile = setmntent(MOUNT_TABLE, "r");
  if (aFile == NULL) {
    std::string msg = std::string("Read ");
    msg += MOUNT_TABLE;
    msg += " faild.";
    error_map["message"] = picojson::value(msg);
    return;
  }

  while (NULL != (ent = getmntent(aFile))) {
    if(ent->mnt_fsname[0] == '/') {
      picojson::value unit = picojson::value(picojson::object());
      GetDetails(ent->mnt_fsname, ent->mnt_dir, error, unit);
      if (error_map["message"] != picojson::value("")) {
        endmntent(aFile);
        return;
      }
      units_arr.push_back(unit);
    }
  }

  endmntent(aFile);
  data_map["units"] = units;
  error_map["message"] = picojson::value("");
}

std::string SysinfoStorage::GetDevProperty(struct udev_device* dev,
                                           const std::string& attr) {
  struct udev_list_entry *attr_list_entry, *attr_entry;

  attr_list_entry = udev_device_get_properties_list_entry(dev);
  attr_entry = udev_list_entry_get_by_name(attr_list_entry, attr.c_str());
  if (0 == attr_entry)
   return NULL;

  return std::string(udev_list_entry_get_value(attr_entry));
}
                              
std::string
SysInfoStorage::GetDevPathFromMountPath(const std::string& mnt_path) {
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  if(mnt_path.empty() || mnt_path[0] != '/' || mnt_path.size() <=1) {
    return NULL;
  }

  enumerate = udev_enumerate_new(_udev);
  udev_enumerate_add_match_subsystem(enumerate, "block");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(dev_list_entry, devices) {
    struct udev_device *dev;
    const char *path;
    std::string dev_path, str;

    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(_udev, path);

    dev_path = GetDevProperty(dev, "DEVPATH");
    if (dev_path.empty()) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return NULL;
    }

    // Append /sys
    dev_path = "/sys" + dev_path;

    str = GetDevProperty(dev, "DEVNAME");
    if (!str.empty() && (str == mnt_path)) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }

    str = GetDevProperty(dev, "DEVLINKS");
    if (!str.empty() && (std::string::npos != str.find(mnt_path))) {
      udev_device_unref(dev);
      udev_enumerate_unref(enumerate);
      return dev_path;
    }
    
  }

  udev_enumerate_unref(enumerate);
  return NULL;
}

void SysInfoStorage::GetDetails(const std::string& mnt_fsname,
                                const std::string& mnt_dir,
                                picojson::value& error,
                                picojson::value& unit) {
  struct udev_device* dev;
  struct udev_list_entry *attr_list_entry, *attr_entry;
  const char* str;
  bool is_removable;

  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& unit_map = unit.get<picojson::object>();

  std::string dev_path = GetDevPathFromMountPath(mnt_fsname);
  if (dev_path.empty()) {
    error_map["message"] = picojson::value("Get storage DEVPATH failed.");
    return;
  }

  dev = udev_device_new_from_syspath(_udev, (char *)dev_path.c_str());
  if (!dev) {
    error_map["message"] = picojson::value("Get storage udev from device_id failed.");
  }

  attr_list_entry = udev_device_get_properties_list_entry(dev);

  struct udev_device* parent_dev;
  parent_dev = udev_device_get_parent_with_subsystem_devtype(dev, "block", "disk");
  str = udev_device_get_sysattr_value(parent_dev, "removable");
  if (NULL == str) {
    error_map["message"] = picojson::value("Get storage attribute removable failed.");
    udev_device_unref(parent_dev);
    udev_device_unref(dev);
    return;
  }

  is_removable = (1 == atoi(str));
  unit_map["isRemovable"] = picojson::value(is_removable);
  // deperacated, same as isRemovable
  unit_map["isRemoveable"] = picojson::value(is_removable);

  unit_map["type"] = picojson::value("UNKNOWN");
  if (is_removable) {
    unit_map["type"] = picojson::value("INTERNAL");
  } else {
    attr_entry = udev_list_entry_get_by_name(attr_list_entry, "ID_BUS");
    if (attr_entry) {
      str = udev_list_entry_get_value(attr_entry);
      if (str && (strcmp(str, "usb") == 0)) {
        unit_map["type"] = picojson::value("USB_HOST");
      }
    }
    // FIXME(halton): Add MMC type support, we do not find a correct
    // attribute to identify.
  }

  str = udev_device_get_sysattr_value(dev, "size");
  if (NULL == str) {
    error_map["message"] =
        picojson::value("Get storage attribute size failed.");
    udev_device_unref(dev);
    return;
  }
  unit_map["capacity"] = picojson::value((double)(atoll(str)*512));

  struct statvfs buf;
  int ret = statvfs(mnt_dir.c_str(), &buf);
  if (-1 == ret) {
    error_map["message"] =
        picojson::value("Get storage availableCapacity failed.");
    udev_device_unref(dev);
    return;
  }

  udev_device_unref(dev);
  unit_map["availableCapacity"] =
      picojson::value((double)(buf.f_bavail * buf.f_bsize));
}
