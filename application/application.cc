// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application.h"

#include <iostream>

#include "application/application_dbus_agent.h"
#include "application/application_information.h"
#include "tizen/tizen.h"

Application* Application::Create(const std::string& pkg_id) {
  std::string app_id = ApplicationInformation::PkgIdToAppId(pkg_id);
  if (app_id.empty()) {
    std::cerr << "Can't translate app package ID to application ID.\n";
    return NULL;
  }

  ApplicationDBusAgent* dbus_agent = ApplicationDBusAgent::Create(pkg_id);
  if (!dbus_agent) {
    std::cerr << "Can't create app DBus agent.\n";
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
  // TODO(xiang): parse result to JSON
  return picojson::value();
}

picojson::value Application::Hide() {
  GVariant* value = dbus_agent_->HideCurrentApp();
  // TODO(xiang): parse result to JSON 
  return picojson::value();
}

void Application::KillApp(const std::string& context_id,
                          AsyncMessageCallback callback) {
  AppDBusCallback dbus_callback =
      std::bind(&Application::HandleErrorCodeResult,
                this, callback, std::placeholders::_1);
  dbus_agent_->KillApp(context_id, dbus_callback);
}

void Application::LaunchApp(const std::string& app_id,
                            AsyncMessageCallback callback) {
  AppDBusCallback dbus_callback =
      std::bind(&Application::HandleErrorCodeResult,
                this, callback, std::placeholders::_1);
  dbus_agent_->LaunchApp(app_id, dbus_callback);
}

picojson::object Application::HandleErrorCodeResult(
    AsyncMessageCallback callback,
    GVariant* value) {
  gint error;
  g_variant_get(value, "(i)", &error);
  picojson::object obj = picojson::object();

  if (static_cast<WebApiAPIErrors>(error) != WebApiAPIErrors::NO_ERROR)
    obj["error"] = picojson::value(static_cast<double>(error));
  callback(obj);
}
