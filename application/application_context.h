// Copyright (c) 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPLICATION_APPLICATION_CONTEXT_H_
#define APPLICATION_APPLICATION_CONTEXT_H_

#include <string>

#include "common/picojson.h"
#include "tizen/tizen.h"

class ApplicationContext {
 public:
  static picojson::value* GetAllRunning();

  explicit ApplicationContext(const std::string& context_id);
  ~ApplicationContext();

  std::string Serialize() const;

 private:
  WebApiAPIErrors err_code_;
  std::string context_id_;
  std::string app_id_;
};

#endif  // APPLICATION_APPLICATION_CONTEXT_H_
