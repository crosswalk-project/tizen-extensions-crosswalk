/****************************************************************************
**
** Copyright © 1992-2014 Cisco and/or its affiliates. All rights reserved.
** All rights reserved.
**
** $CISCO_BEGIN_LICENSE:APACHE$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** http://www.apache.org/licenses/LICENSE-2.0
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $CISCO_END_LICENSE$
**
****************************************************************************/
#ifndef IOTIVITY_INSTANCE_H_
#define IOTIVITY_INSTANCE_H_

#include <string>

#include "common/extension.h"
#include "common/picojson.h"

class IotivityInstance : public common::Instance {
 public:
  IotivityInstance();
  ~IotivityInstance();

  // common::Instance implementation
  void HandleMessage(const char* message);
  void HandleSyncMessage(const char* message);

  private:
    std::string PrepareMessage(const std::string & message) const;
};

#endif  // IOTIVITY_INSTANCE_H_



