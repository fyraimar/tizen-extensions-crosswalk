// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_display.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "common/picojson.h"
#include "system_info/system_info_utils.h"

#if defined(GENERIC_DESKTOP)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/acpi_video0"
#elif defined(TIZEN_MOBILE)
  #define ACPI_BACKLIGHT_DIR "/sys/class/backlight/psb-bl"
#else
  #error "Unsupported platform"
#endif

void SysInfoDisplay::Update(picojson::value& error) {
  picojson::object& error_map = error.get<picojson::object>();
  Display *dpy = XOpenDisplay(NULL);

  if (NULL == dpy) {
    error_map["message"] = picojson::value("XOpenDisplay Failed");
    return;
  }

  _resolution_width = DisplayWidth(dpy, DefaultScreen(dpy));
  if (!_resolution_width) {
    error_map["message"] = picojson::value("SCREEN WIDTH is Zero");
    XCloseDisplay(dpy);
    return;
  }

  _resolution_height = DisplayHeight(dpy, DefaultScreen(dpy));
  if (!_resolution_height) {
    error_map["message"] = picojson::value("SCREEN HEIGHT is Zero");
    XCloseDisplay(dpy);
    return;
  }

  _physical_width = DisplayWidthMM(dpy, DefaultScreen(dpy));
  if (!_physical_width) {
    error_map["message"] = picojson::value("SCREEN WIDTH is Zero");
    XCloseDisplay(dpy);
    return;
  }

  _physical_height = DisplayHeightMM(dpy, DefaultScreen(dpy));
  if (!_physical_width) {
    error_map["message"] = picojson::value("SCREEN HEIGHT is Zero");
    XCloseDisplay(dpy);
    return;
  }

  XCloseDisplay(dpy);
}

SysInfoDisplay::SysInfoDisplay() :
    _resolution_width(0), _resolution_height(0),
    _physical_width(0.0), _physical_height(0.0) {
}

unsigned long SysInfoDisplay::GetResolutionWidth(picojson::value& error) {
  Update(error);
  return _resolution_width;
}

unsigned long SysInfoDisplay::GetResolutionHeight(picojson::value& error) {
  Update(error);
  return _resolution_height;
}

unsigned long SysInfoDisplay::GetDotsPerInchWidth(picojson::value& error) {
  Update(error);
  // there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
  //     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
  //        = N pixels / (M inch / 25.4)
  //        = N * 25.4 pixels / M inch
  return (_resolution_width * 25.4) / _physical_width;
}

unsigned long SysInfoDisplay::GetDotsPerInchHeight(picojson::value& error) {
  Update(error);
  return (_resolution_height * 25.4) / _physical_height;
}

double SysInfoDisplay::GetPhysicalWidth(picojson::value& error) {
  Update(error);
  return _physical_width;
}

double SysInfoDisplay::GetPhysicalHeight(picojson::value& error) {
  Update(error);
  return _physical_height;
}

double SysInfoDisplay::GetBrightness(picojson::value& error) {
  FILE* fp = NULL;
  char* str_val = NULL;
  char max_path[] = ACPI_BACKLIGHT_DIR"/max_brightness";
  char brightness_path[] = ACPI_BACKLIGHT_DIR"/brightness";
  int max_val, val;

  str_val = system_info::read_one_line(max_path);
  if(NULL == str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum
    return 1.0;
  }
  max_val = atoi(str_val);
  free(str_val);

  str_val = system_info::read_one_line(brightness_path);
  if(NULL == str_val) {
    // FIXME(halton): ACPI is not enabled, fallback to maximum
    return 1.0;
  }
  val = atoi(str_val);
  free(str_val);

  return val/max_val;
}
