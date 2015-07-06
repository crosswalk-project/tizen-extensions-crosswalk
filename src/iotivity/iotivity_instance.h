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

#include "OCPlatform.h"
#include "OCApi.h"

using namespace OC;
using namespace std;


class IotivityInstance : public common::Instance {
 public:
  IotivityInstance();
  ~IotivityInstance();

  // common::Instance implementation
  void HandleMessage(const char* message); // js::extension.postMessage(msg)
  void HandleSyncMessage(const char* message); // js::extension.internal.sendSyncMessage(msg);


  void handleFactoryReset(const picojson::value& value);
  void handleReboot(const picojson::value& value);


  void foundResourceCallback(std::shared_ptr<OCResource> resource);
  void handleFindResources(const picojson::value& value);

  void foundDeviceCallback(const OCRepresentation& rep);
  void handleFindDevices(const picojson::value& value);

  void handleConfigure(const picojson::value& value);


  void onPut(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);
  void onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);
  void onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode);

  void handleCRUDNRRequest(const picojson::value& value);
  void handleCreateResource(const picojson::value& value);
  void handleRetrieveResource(const picojson::value& value);
  void handleUpdateResource(const picojson::value& value);
  void handleDeleteResource(const picojson::value& value);
  void handleStartObserving(const picojson::value& value);
  void handleCancelObserving(const picojson::value& value);

  void postEntityHandler(std::shared_ptr<OCResourceRequest> request);
  OCEntityHandlerResult entityHandlerCallback(std::shared_ptr<OCResourceRequest> request);
  void handleRegisterResource(const picojson::value& value);
  void handleUnregisterResource(const picojson::value& value);
  void handleEnablePresence(const picojson::value& value);
  void handleDisablePresence(const picojson::value& value);
  void handleNotify(const picojson::value& value);


  void handleSendResponse(const picojson::value& value);
  void handleSendError(const picojson::value& value);


  void postResult(const char* completed_operation, double async_operation_id);
  void postError(double async_operation_id);


  void postRegisterResource(double async_operation_id, OCResourceHandle resHandle, const picojson::value& param);

  private:
    std::string PrepareMessage(const std::string & message);
};

#endif  // IOTIVITY_INSTANCE_H_



