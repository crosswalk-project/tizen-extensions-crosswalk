#include <string>
#include <map>

#include "iotivity/iotivity_instance.h"
#include "iotivity/iotivity_device.h"
#include "iotivity/iotivity_server.h"
#include "iotivity/iotivity_client.h"
#include "iotivity/iotivity_resource.h"

std::map<int, OCRepresentation> ResourcesMap;

IotivityInstance::IotivityInstance() {
  pDebugEnv = getenv("IOTIVITY_DEBUG");
  m_device = new IotivityDevice(this, NULL);
}

IotivityInstance::~IotivityInstance() {
  delete m_device;
}

void IotivityInstance::HandleMessage(const char* message) {
  std::string resp = PrepareMessage(message);
  (void)resp;  // Warning fix
  // PostMessage(resp.c_str());
  // to javascript extension.setMessageListener()
}

void IotivityInstance::HandleSyncMessage(const char* message) {
  // std::string resp = PrepareMessage(message);
  // SendSyncReply(resp.c_str());
}

std::string IotivityInstance::PrepareMessage(const std::string & message) {
  const char *msg = message.c_str();
  std::string resp = "";

  if (pDebugEnv != NULL)
    printf("\n\n[JS==>Native] IotivityInstance::PrepareMessage: %s\n", msg);

  picojson::value v;
  std::string error;

  picojson::parse(v, msg, msg + strlen(msg), &error);
  if (!error.empty()) {
    std::cout << "Ignoring message.\n";
    return resp;
  }

  std::string cmd = v.get("cmd").to_str();

  // Device
  if (cmd == "configure")
    m_device->handleConfigure(v);
  else if (cmd == "factoryReset")
    m_device->handleFactoryReset(v);
  else if (cmd == "reboot")
    m_device->handleReboot(v);
  // Client
  else if (cmd == "findResources")
    m_device->getClient()->handleFindResources(v);
  else if (cmd == "findDevices")
    m_device->getClient()->handleFindDevices(v);
  else if (cmd == "createResource")
    m_device->getClient()->handleCreateResource(v);
  else if (cmd == "retrieveResource")
    m_device->getClient()->handleRetrieveResource(v);
  else if (cmd == "updateResource")
    m_device->getClient()->handleUpdateResource(v);
  else if (cmd == "deleteResource")
    m_device->getClient()->handleDeleteResource(v);
  else if (cmd == "startObserving")
    m_device->getClient()->handleStartObserving(v);
  else if (cmd == "cancelObserving")
    m_device->getClient()->handleCancelObserving(v);
  // Server
  else if (cmd == "registerResource")
    m_device->getServer()->handleRegisterResource(v);
  else if (cmd == "unregisterResource")
    m_device->getServer()->handleUnregisterResource(v);
  else if (cmd == "enablePresence")
    m_device->getServer()->handleEnablePresence(v);
  else if (cmd == "disablePresence")
    m_device->getServer()->handleDisablePresence(v);
  else if (cmd == "notify")
    m_device->getServer()->handleNotify(v);
  else if (cmd == "sendResponse")
    handleSendResponse(v);
  else if (cmd == "sendError")
    handleSendError(v);
  else
    ERROR_MSG(std::string("Received unknown message: " + cmd + "\n").c_str());

  return resp;
}


void IotivityInstance::handleSendResponse(const picojson::value& value) {
  DEBUG_MSG("handleSendResponse: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();

  picojson::value resource = value.get("resource");
  picojson::value OicRequestEvent = value.get("OicRequestEvent");
  IotivityRequestEvent iotivityRequestEvent;
  iotivityRequestEvent.deserialize(OicRequestEvent);
  OCStackResult result = iotivityRequestEvent.sendResponse();

  if (OC_STACK_OK != result) {
    m_device->postError(async_call_id);
    return;
  }

  m_device->postResult("sendResponseCompleted", async_call_id);
}

void IotivityInstance::handleSendError(const picojson::value& value) {
  DEBUG_MSG("handleSendError: v=%s\n", value.serialize().c_str());

  double async_call_id = value.get("asyncCallId").get<double>();
  std::string errorMsg = value.get("error").to_str();
  picojson::value OicRequestEvent = value.get("OicRequestEvent");
  IotivityRequestEvent iotivityRequestEvent;
  iotivityRequestEvent.deserialize(OicRequestEvent);
  OCStackResult result = iotivityRequestEvent.sendError();

  if (OC_STACK_OK != result) {
    m_device->postError(async_call_id);
    return;
  }

  m_device->postResult("sendResponseCompleted", async_call_id);
}
