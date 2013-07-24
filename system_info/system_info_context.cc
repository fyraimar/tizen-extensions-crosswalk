// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_info/system_info_context.h"

#include "common/picojson.h"

CXWalkExtension* xwalk_extension_init(int32_t api_version) {
  return ExtensionAdapter<SystemInfoContext>::Initialize();
}

SystemInfoContext::SystemInfoContext(ContextAPI* api)
    : api_(api) {}

SystemInfoContext::~SystemInfoContext() {
  delete api_;
}

const char SystemInfoContext::name[] = "tizen.systeminfo";

// This will be generated from system_info_api.js.
extern const char kSource_system_info_api[];

const char* SystemInfoContext::GetJavaScript() {
  return kSource_system_info_api;
}

void SystemInfoContext::GetBattery(picojson::value& error,
                                   picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["level"] = picojson::value(0.5);
  data_map["isCharging"] = picojson::value(false);
  error_map["message"] = picojson::value("");

  // delete this so that the value can be get from JS
  error_map["message"] = picojson::value("Get battery failed.");
}

void SystemInfoContext::GetCPU(picojson::value& error,
                               picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  data_map["load"] = picojson::value(0.3);
  error_map["message"] = picojson::value("");

  // delete this so that the value can be get from JS
  error_map["message"] = picojson::value("Get CPU failed.");
}

void SystemInfoContext::GetStorage(picojson::value& error,
                                   picojson::value& data) {
  picojson::object& error_map = error.get<picojson::object>();
  picojson::object& data_map = data.get<picojson::object>();

  // FIXME(halton): Add actual implementation
  picojson::value units = picojson::value(picojson::array(2));
  picojson::array& units_arr = units.get<picojson::array>();

  picojson::value unit1 = picojson::value(picojson::object());
  picojson::object& unit1_map = unit1.get<picojson::object>();
  unit1_map["type"] = picojson::value("INTERNAL");
  // 20g
  unit1_map["capacity"] = picojson::value((double)20971520);
  // 5g
  unit1_map["availableCapacity"] = picojson::value((double)5242880);
  unit1_map["isRemovable"] = picojson::value(true);
  // deperacated, same as isRemovable
  unit1_map["isRemoveable"] = picojson::value(true);
  units_arr[0] = unit1;

  picojson::value unit2 = picojson::value(picojson::object());
  picojson::object& unit2_map = unit2.get<picojson::object>();
  unit2_map["type"] = picojson::value("USB_HOST");
  // 10g
  unit2_map["capacity"] = picojson::value((double)10485760);
  // 2g
  unit2_map["availableCapacity"] = picojson::value((double)2097152);
  unit2_map["isRemovable"] = picojson::value(true);
  // deperacated, same as isRemovable
  unit2_map["isRemoveable"] = picojson::value(true);
  units_arr[1] = unit2;

  data_map["units"] = units;
  error_map["message"] = picojson::value("");

  // delete this so that the value can be get from JS
  //error_map["message"] = picojson::value("Get Storage failed.");
}

void SystemInfoContext::HandleGetPropertyValue(const picojson::value& input,
                                               picojson::value& output) {
  picojson::value error;
  picojson::value data;
  std::string prop;

  error = picojson::value(picojson::object());
  data = picojson::value(picojson::object());

  picojson::object& error_map = error.get<picojson::object>();
  error_map["message"] = picojson::value("");

  prop = input.get("prop").to_str();
  if (prop == "BATTERY") {
    GetBattery(error, data);
  } else if (prop == "CPU") {
    GetCPU(error, data);
  } else if (prop == "STORAGE") {
    GetStorage(error, data);
  } else {
    error_map["message"] = picojson::value("Not supportted property " + prop);
  }

  picojson::object& output_map = output.get<picojson::object>();
  if (!error.get("message").to_str().empty()) {
    output_map["error"] = error;
  } else {
    output_map["data"] = data;
  }
}

void SystemInfoContext::HandleMessage(const char* message) {
  picojson::value input;
  picojson::value output;

  std::string err;
  picojson::parse(input, message, message + strlen(message), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  picojson::object& input_map = input.get<picojson::object>();
  output = picojson::value(picojson::object());
  picojson::object& output_map = output.get<picojson::object>();
  std::string reply_id = input.get("_reply_id").to_str();
  output_map["_reply_id"] = picojson::value(reply_id);

  std::string cmd = input.get("cmd").to_str();
  if (cmd == "getPropertyValue")
    HandleGetPropertyValue(input, output);

  std::string result = output.serialize();
  api_->PostMessage(result.c_str());
}
