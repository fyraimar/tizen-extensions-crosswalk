// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_
#define SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_

#include "common/extension_adapter.h"

namespace picojson {
class value;
}  // namespace picojson

class SystemInfoContext {
 public:
  SystemInfoContext(ContextAPI* api);
  ~SystemInfoContext();

  // ExtensionAdapter implementation.
  static const char name[];
  static const char* GetJavaScript();
  void HandleMessage(const char*);

 private:
  void HandleGetPropertyValue(const picojson::value& input,
                              picojson::value& output);
  void GetBattery(picojson::value& error,
                  picojson::value& data);
  void GetCPU(picojson::value& error,
              picojson::value& data);
  void GetStorage(picojson::value& error,
                  picojson::value& data);
  void GetDisplay(picojson::value& error,
                  picojson::value& data);
  void GetDeviceOrientation(picojson::value& error,
                            picojson::value& data);
  void GetBuild(picojson::value& error,
                picojson::value& data);
  void GetLocale(picojson::value& error,
                 picojson::value& data);
  void GetNetwork(picojson::value& error,
                  picojson::value& data);
  void GetWifiNetwork(picojson::value& error,
                      picojson::value& data);
  void GetCellularNetwork(picojson::value& error,
                          picojson::value& data);
  void GetSIM(picojson::value& error,
              picojson::value& data);

  ContextAPI* api_;
};

#endif  // SYSTEM_INFO_SYSTEM_INFO_CONTEXT_H_
