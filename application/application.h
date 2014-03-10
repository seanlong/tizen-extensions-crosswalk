// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include <gio/gio.h>

#include <functional>
#include <memory>
#include <string>

#include "application/application_instance.h"

class ApplicationDBusAgent;

using AsyncMessageCallback = ApplicationInstance::AsyncMessageCallback;

class Application {
 public:
  typedef std::function<picojson::object(GVariant* v)> AppDBusCallback;

  static Application* Create(const std::string& pkg_id);

  ~Application();

  const std::string& GetContextId();

  picojson::value Exit();
  picojson::value Hide();

  void KillApp(const std::string& context_id, AsyncMessageCallback callback);
  void LaunchApp(const std::string& app_id, AsyncMessageCallback callback);

  const std::string& app_id() const { return app_id_; }
  const std::string& pkg_id() const { return pkg_id_; }

 private:
  Application(const std::string& pkg_id,
              const std::string& app_id,
              ApplicationDBusAgent* dbus_agent);

  picojson::object HandleErrorCodeResult(AsyncMessageCallback callback,
                                         GVariant* value);

  std::string app_id_;
  std::string pkg_id_;
  std::string context_id_;
  std::unique_ptr<ApplicationDBusAgent> dbus_agent_;
};

#endif  // APPLICATION_APPLICATION_H_
