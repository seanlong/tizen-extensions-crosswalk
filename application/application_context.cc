// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_context.h"

#include <app_manager.h>
#include <memory>
#include <tuple>

#include "application/application_manager.h"

namespace {

typedef std::vector<std::shared_ptr<ApplicationContext> > ContextsVector;

bool GetAppContextCallback(app_context_h app_context, void* user_data) {
  ApplicationManager* manager;
  ContextsVector* contexts;
  std::tie(manager, contexts) = *(static_cast<std::tuple<ApplicationManager*,
      ContextsVector*>*>(user_data));

  pid_t pid;
  int ret = app_context_get_pid(app_context, &pid);
  if((ret != APP_MANAGER_ERROR_NONE) || (pid <= 0)) {
    std::cerr << "Fail to get pid from context handle.\n";
    contexts->clear();
    return false;
  }

  std::shared_ptr<ApplicationContext> context(
      new ApplicationContext(manager, std::to_string(pid)));
  contexts->push_back(context);
  return true;
}

}  // namespace

picojson::value* ApplicationContext::GetAllRunning(
    ApplicationManager* manager) {
  ContextsVector app_contexts;
  auto tuple_data = std::make_tuple(manager, &app_contexts);
  int ret = app_manager_foreach_app_context(
      GetAppContextCallback, &tuple_data);

  picojson::array json_contexts;
  auto ctx_it = app_contexts.begin();
  for (; ctx_it != app_contexts.end(); ++ctx_it) {
    if (!(*ctx_it)->IsValid()) {
      json_contexts.clear();
      break;
    }
    json_contexts.push_back(picojson::value((*ctx_it)->Serialize()));
  }

  picojson::value* result = new picojson::value(picojson::object_type, true);
  if (json_contexts.empty())
    (*result).get<picojson::object>()["error"] =
        picojson::value(static_cast<double>(WebApiAPIErrors::UNKNOWN_ERR));
  else
    (*result).get<picojson::object>()["data"] = picojson::value(json_contexts);
  return result;
}

ApplicationContext::ApplicationContext(ApplicationManager* manager,
                                       const std::string& context_id)
    : err_code_(WebApiAPIErrors::NO_ERROR),
      context_id_(context_id) {
  int pid = std::stoi(context_id);
  if (pid <= 0)
    err_code_ = WebApiAPIErrors::NOT_FOUND_ERR;

  // We can't directly call app_manager_get_app_id here, because of it's
  // implemented on aul_app_get_appid_bypid which is an AUL API. So the
  // xwalk-launcher will handle this for us.
  manager->GetAppIdByPid(pid, app_id_, err_code_);
}

ApplicationContext::~ApplicationContext() {
}

std::string ApplicationContext::Serialize() const {
  picojson::value result(picojson::object_type, true);
  picojson::object obj;
  if (!IsValid()) {
    obj["code"] = picojson::value(static_cast<double>(err_code_));
    result.get<picojson::object>()["error"] = picojson::value(obj);
  } else {
    obj["id"] = picojson::value(context_id_);
    obj["appId"] = picojson::value(app_id_);
    result.get<picojson::object>()["data"] = picojson::value(obj);
  }
  return result.serialize();
}

bool ApplicationContext::IsValid() const {
  return err_code_ == WebApiAPIErrors::NO_ERROR;
}
