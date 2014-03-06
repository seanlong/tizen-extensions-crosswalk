// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application.h"

#include <iostream>

#include "application/application_dbus_agent.h"
#include "application/application_information.h"
#include "tizen/tizen.h"

namespace {

picojson::object HandleErrorCodeResult(GVariant* value) {
  gint error;
  g_variant_get(value, "(i)", &error);

  picojson::object obj = picojson::object();
  if (static_cast<WebApiAPIErrors>(error) != WebApiAPIErrors::NO_ERROR)
    obj["error"] = picojson::value(static_cast<double>(error));
  return obj;
}

}  // namespace

AppDBusCallback::AppDBusCallback(Application* runner,
                                 AsyncMessageCallback* msg_callback,
                                 PtrDBusCallback callback)
    : runner_(runner),
      msg_callback_(msg_callback),
      callback_(callback) {
}

void AppDBusCallback::Run(GVariant* value) {
  picojson::object result = (runner_->*callback_)(value);
  msg_callback_->Run(result);
  delete this;
}

Application* Application::Create(const std::string& pkg_id) {
  std::string app_id = ApplicationInformation::PkgIdToAppId(pkg_id);
  if (app_id.empty()) {
    std::cerr << "Can't translate app package ID to application ID.\n";
    return NULL;
  }

  ApplicationDBusAgent* dbus_agent = ApplicationDBusAgent::Create(pkg_id);
  if (!dbus_agent) {
    std::cerr << "Can't create app DBus agent, some APIs will not work.\n";
    return NULL;
  }

  return new Application(pkg_id, app_id, dbus_agent);
}

Application::Application(const std::string& pkg_id,
                         const std::string& app_id,
                         ApplicationDBusAgent* dbus_agent)
    : pkg_id_(pkg_id),
      app_id_(app_id),
      dbus_agent_(dbus_agent) {
}

Application::~Application() {
}

const std::string& Application::GetContextId() {
  pid_t pid = dbus_agent_->GetLauncherPid();
  return context_id_; 
}

picojson::value Application::Exit() {
  GVariant* value = dbus_agent_->ExitCurrentApp();
  //parse result to JSON
  return picojson::value();
}

picojson::value Application::Hide() {
  GVariant* value = dbus_agent_->HideCurrentApp();
  //parse result to JSON 
  return picojson::value();
}

void Application::KillApp(const std::string& context_id,
                          AsyncMessageCallback* callback) {
  AppDBusCallback* app_callback = new AppDBusCallback(
      this, callback, &Application::KillAppCallback);
  dbus_agent_->KillApp(context_id, app_callback);
}

void Application::LaunchApp(const std::string& app_id,
                            AsyncMessageCallback* callback) {
  AppDBusCallback* app_callback = new AppDBusCallback(
      this, callback, &Application::LaunchAppCallback);
  dbus_agent_->LaunchApp(app_id, app_callback);
}

picojson::object Application::KillAppCallback(GVariant* value) {
  return HandleErrorCodeResult(value);
}

picojson::object Application::LaunchAppCallback(GVariant* value) {
  return HandleErrorCodeResult(value);
}
