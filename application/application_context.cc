// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_context.h"

#include <app_manager.h>

picojson::value* ApplicationContext::GetAllRunning() {
  return NULL;
}

ApplicationContext::ApplicationContext(const std::string& context_id)
    : err_code_(WebApiAPIErrors::NO_ERROR),
      context_id_(context_id) {
  int id = atoi(context_id.c_str());
  if (id <= 0)
    err_code_ = WebApiAPIErrors::NOT_FOUND_ERR;

  char* app_id = NULL;
  int ret = app_manager_get_app_id(id, &app_id);
  if (ret == APP_MANAGER_ERROR_NONE && app_id) {
    app_id_ = app_id;
    free(app_id);
    return;
  }

  switch (ret) {
    case APP_MANAGER_ERROR_NO_SUCH_APP:
    case APP_MANAGER_ERROR_INVALID_PARAMETER:
      std::cerr << "app_manager_get_app_id error : no found." << std::endl;
      err_code_ = WebApiAPIErrors::NOT_FOUND_ERR;
      break;
    default:
      std::cerr << "app_manager_get_app_id error :" << ret << std::endl;
      err_code_ = WebApiAPIErrors::UNKNOWN_ERR;
      break;
  }
}

std::string ApplicationContext::Serialize() const {
  picojson::value result(picojson::object_type, true);
  if (err_code_ != WebApiAPIErrors::NO_ERROR) {
    picojson::value error(picojson::object_type, true);
    error.get<picojson::object>()["code"] =
        picojson::value(static_cast<double>(err_code_));
    result.get<picojson::object>()["error"] = error;
  } else {
    picojson::value data(picojson::object_type, true);
    data.get<picojson::object>()["id"] = picojson::value(context_id_);
    data.get<picojson::object>()["appId"] = picojson::value(app_id_);
    result.get<picojson::object>()["data"] = data;
  }
  return result.serialize();
}
