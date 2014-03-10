// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "application/application_dbus_agent.h"

namespace {

const char kRuntimeServiceName[] =  "org.crosswalkproject.Runtime1";

const char kRuntimeRunningManagerPath[] = "/running1";

const char kRuntimeRunningAppInterface[] =
    "org.crosswalkproject.Running.Application1";

const char kLauncherConnectionAddressPrefix[] =
    "unix:path=/tmp/xwalk-launcher-";

const char kLauncherDBusObjectPath[] = "/launcher1";

const char kLauncherDBusApplicationInterface[] =
    "org.crosswalkproject.Launcher.Application1";

const char kLauncherDBusApplicationInterfaceError[] =
    "org.crosswalkproject.Launcher.Application1.Error";

// Create proxy for running application on runtime process. The runtime process
// exports its object on session message bus.
GDBusProxy* ConnectToRuntimeRunningApp(const std::string& xwalk_app_id) {
  GError* error = NULL;
  GDBusConnection* connection =
      g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);         
  if (!connection) {
    fprintf(stderr, "Couldn't get the session bus connection: %s\n",
            error->message);
    g_error_free (error);
    return NULL;
  }

  std::string path = kRuntimeRunningManagerPath;
  path += "/" + xwalk_app_id;
  GDBusProxy* proxy = g_dbus_proxy_new_sync(
      connection, G_DBUS_PROXY_FLAGS_NONE, NULL, kRuntimeServiceName,
      path.c_str(), kRuntimeRunningAppInterface, NULL, &error);                                                   
  if (!proxy) {
    g_print("Couldn't create proxy for '%s': %s\n",
            kRuntimeRunningAppInterface, error->message);                                                  
    g_error_free(error);
    return NULL;
  }

  return proxy;
}

// Create proxy for application interface exported by xwalk-launcher. The
// xwalk-laucher will create a new DBus server at a custom address.
GDBusProxy* ConnectToLauncherApp(const std::string& xwalk_app_id) {
  std::string conn_address = kLauncherConnectionAddressPrefix + xwalk_app_id;
  GError* error = NULL;
  GDBusConnection* connection;

  connection = g_dbus_connection_new_for_address_sync(
      conn_address.c_str(), G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
      NULL, NULL, &error);
  if (!connection) {
    g_printerr("Error connecting to D-Bus at address %s: %s\n",
               conn_address.c_str(), error->message);
    g_error_free(error);
    return NULL;
  }

  GDBusProxy* proxy = g_dbus_proxy_new_sync(
      connection, G_DBUS_PROXY_FLAGS_NONE, NULL, NULL, kLauncherDBusObjectPath,
      kLauncherDBusApplicationInterface, NULL, &error);                                                   
  if (!proxy) {
    g_print("Couldn't create proxy for '%s': %s\n",
            kLauncherDBusApplicationInterface, error->message);                                                  
    g_error_free(error);
    return NULL;
  }

  return proxy;
}

void FinishAppOperation(GObject* source_object,
                        GAsyncResult* res,
                        gpointer user_data) {
  GError* error = NULL;
  GVariant* value;
  GDBusProxy* proxy = G_DBUS_PROXY(source_object);
  AppDBusCallback* callback = static_cast<AppDBusCallback*>(user_data);

  value = g_dbus_proxy_call_finish(proxy, res, &error);
  if (!value) {
    std::cerr << "Failed to run app operation:" << error->message;
    g_error_free(error);
    return;
  }

  (*callback)(value);
  g_variant_unref(value);
  delete callback;
}

}  // namespace

ApplicationDBusAgent*
ApplicationDBusAgent::Create(const std::string& xwalk_app_id) {
  GDBusProxy* runtime_app_proxy = ConnectToRuntimeRunningApp(xwalk_app_id);
  if (!runtime_app_proxy)
    return NULL;

  GDBusProxy* launcher_app_proxy = ConnectToLauncherApp(xwalk_app_id);
  if (!launcher_app_proxy)
    return NULL;

  return new ApplicationDBusAgent(runtime_app_proxy, launcher_app_proxy);
}

ApplicationDBusAgent::ApplicationDBusAgent(GDBusProxy* runtime_app_proxy,
                                           GDBusProxy* launcher_app_proxy)
    : runtime_proxy_(runtime_app_proxy),
      launcher_proxy_(launcher_app_proxy) {
}

pid_t ApplicationDBusAgent::GetLauncherPid() {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(launcher_proxy_, "GetPid",
                                            NULL, G_DBUS_CALL_FLAGS_NONE,
                                            -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'GetPid' method:" << error->message;
    g_error_free(error);
    return -1;
  }

  pid_t pid;
  g_variant_get(result, "(i)", &pid);
  return pid;
}

GVariant* ApplicationDBusAgent::ExitCurrentApp() {
  // Currently xwalk runtime only allow the caller of Launch() to call
  // Terminate(), so we need let xwalk-laucher to do this job. 
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(launcher_proxy_, "Exit",
                                            NULL, G_DBUS_CALL_FLAGS_NONE,
                                            -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'Exit' method:" << error->message;
    return NULL;
  }
  return result;
}

GVariant* ApplicationDBusAgent::HideCurrentApp() {
  GError* error = NULL;
  GVariant* result = g_dbus_proxy_call_sync(
      runtime_proxy_, "Hide", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (!result) {
    std::cerr << "Couldn't call 'Hide' method:" << error->message;
    return NULL;
  }
  return result;
}

void ApplicationDBusAgent::LaunchApp(const std::string& app_id,
                                     AppDBusCallback callback) {
  AppDBusCallback* saved_callback = new AppDBusCallback(callback);
  g_dbus_proxy_call(launcher_proxy_, "LaunchApp", g_variant_new("(s)",
                    app_id.c_str()), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    FinishAppOperation, saved_callback);
}

void ApplicationDBusAgent::KillApp(const std::string& context_id,
                                   AppDBusCallback callback) {
  AppDBusCallback* saved_callback = new AppDBusCallback(callback);
  g_dbus_proxy_call(launcher_proxy_, "KillApp", g_variant_new("(s)",
                    context_id.c_str()), G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                    FinishAppOperation, saved_callback);
}
