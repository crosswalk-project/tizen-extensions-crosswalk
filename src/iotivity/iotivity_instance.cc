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
#include "iotivity/iotivity_instance.h"


#include <stdio.h>
#include <stdlib.h>
#include <string>


char *pDebugEnv = NULL;

IotivityInstance::IotivityInstance() {
    pDebugEnv = getenv("IOTIVITY_DEBUG");
}

IotivityInstance::~IotivityInstance() {
}

void IotivityInstance::HandleMessage(const char* message) {
  std::string resp = PrepareMessage(message);
  PostMessage(resp.c_str());
}

void IotivityInstance::HandleSyncMessage(const char* message) {
  std::string resp = PrepareMessage(message);
  SendSyncReply(resp.c_str());
}

std::string IotivityInstance::PrepareMessage(const std::string & message) const {

  const char *msg = message.c_str();
  std::string resp = "";

  if (pDebugEnv != NULL)
    printf("IotivityInstance::PrepareMessage: %s\n", msg);

  picojson::value v;
  std::string error;

  picojson::parse(v, msg, msg + strlen(msg), &error);
  if (!error.empty()) {
    std::cout << "Ignoring message.\n";
    return resp;
  }

  std::string cmd = v.get("cmd").to_str();
  

  if (cmd == "ExecuteCommand")
  {
 
  }

  return resp;
}



