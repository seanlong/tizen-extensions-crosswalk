// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_instance.h"

#include <memory>
#include <iostream>
#include <string>
#include <sstream>

#include "application/application.h"
#include "application/application_context.h"
#include "application/application_extension.h"
#include "application/application_information.h"

namespace {

const char kJSCbKey[] = "_callback";

double GetJSCallbackId(const picojson::value& msg) {
  assert(msg.contains(kJSCbKey));
  const picojson::value& id_value = msg.get(kJSCbKey);
  return id_value.get<double>();
}

void SetJSCallbackId(picojson::value& msg, double id) {
  assert(msg.is<picojson::object>());
  msg.get<picojson::object>()[kJSCbKey] = picojson::value(id);
}

}  // namespace

ApplicationInstance::ApplicationInstance(Application* current_app)
    : application_(current_app) {
}

ApplicationInstance::~ApplicationInstance() {
}

void ApplicationInstance::HandleMessage(const char* msg) {
  picojson::value v;

  std::string err;
  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetAppsInfo") {
    HandleGetAppsInfo(v);
  } else if (cmd == "GetAppsContext") {
    HandleGetAppsContext(v);
  } else if (cmd == "KillApp") {
    HandleKillApp(v);
  } else if (cmd == "LaunchApp") {
    HandleLaunchApp(v);
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void ApplicationInstance::HandleSyncMessage(const char* msg) {
  picojson::value v;
  std::string err;

  picojson::parse(v, msg, msg + strlen(msg), &err);
  if (!err.empty()) {
    std::cout << "Ignoring message.\n";
    return;
  }

  std::string cmd = v.get("cmd").to_str();
  if (cmd == "GetAppInfo") {
    HandleGetAppInfo(v);
  } else if (cmd == "GetAppContext") {
    HandleGetAppContext(v); 
  } else if (cmd == "GetCurrentApp") {
    HandleGetCurrentApp(v);
  } else if (cmd == "ExitCurrentApp") {
    HandleExitCurrentApp(v);
  } else if (cmd == "HideCurrentApp") {
    HandleHideCurrentApp(v); 
  } else {
    std::cout << "ASSERT NOT REACHED.\n";
  }
}

void ApplicationInstance::HandleGetAppInfo(picojson::value& msg) {
  std::string app_id;
  if (msg.contains("id") && msg.get("id").is<std::string>())
    app_id = msg.get("id").to_str();
  else
    app_id = application_->app_id();

  ApplicationInformation app_info(app_id);
  SendSyncReply(app_info.Serialize().c_str());
}

void ApplicationInstance::HandleGetAppContext(picojson::value& msg) {
  std::string ctx_id;
  if (msg.contains("id") && msg.get("id").is<std::string>()) {
    ctx_id = msg.get("id").to_str();
  } else {
    ctx_id = application_->GetContextId();
  }

  ApplicationContext app_ctx(ctx_id);
  SendSyncReply(app_ctx.Serialize().c_str());
}

void ApplicationInstance::HandleGetCurrentApp(picojson::value& msg) {
  std::string app_id = application_->app_id();
  std::string ctx_id = application_->GetContextId();
  ApplicationInformation app_info(app_id);
  ApplicationContext app_ctx(ctx_id);

  //TODO avoid this parse
  picojson::value val;
  picojson::value result = picojson::value(picojson::object_type, true);
  std::istringstream buf(app_info.Serialize());
  picojson::parse(val, buf);
  result.get<picojson::object>()["appInfo"] = val;
  std::istringstream buf2(app_info.Serialize());
  picojson::parse(val, buf2);
  result.get<picojson::object>()["appContext"] = val;
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleExitCurrentApp(picojson::value& msg) {
  picojson::value result = application_->Exit();
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleHideCurrentApp(picojson::value& msg) {
  picojson::value result = application_->Hide();
  SendSyncReply(result.serialize().c_str());
}

void ApplicationInstance::HandleGetAppsInfo(picojson::value& msg) {
  std::unique_ptr<picojson::value> result(
      ApplicationInformation::GetAllInstalled());
  ReturnMessageAsync(GetJSCallbackId(msg), result->get<picojson::object>());
}

void ApplicationInstance::HandleGetAppsContext(picojson::value& msg) {
  std::unique_ptr<picojson::value> result(ApplicationContext::GetAllRunning());
  ReturnMessageAsync(GetJSCallbackId(msg), result->get<picojson::object>());
}

void ApplicationInstance::HandleKillApp(picojson::value& msg) {
  std::string context_id = msg.get("id").to_str();
  AsyncMessageCallback callback =
      std::bind(&ApplicationInstance::ReturnMessageAsync,
                this, GetJSCallbackId(msg), std::placeholders::_1);
  application_->KillApp(context_id, callback);
}

void ApplicationInstance::HandleLaunchApp(picojson::value& msg) {
  std::string app_id = msg.get("id").to_str();
  AsyncMessageCallback callback =
      std::bind(&ApplicationInstance::ReturnMessageAsync,
                this, GetJSCallbackId(msg), std::placeholders::_1);
  application_->LaunchApp(app_id, callback);
}

void ApplicationInstance::ReturnMessageAsync(double callback_id,
                                             const picojson::object& obj) {
  picojson::value ret_msg(obj);
  SetJSCallbackId(ret_msg, callback_id);
  PostMessage(ret_msg.serialize().c_str());
}
