#ifndef IOTIVITY_EXTENSION_H_
#define IOTIVITY_EXTENSION_H_

#include "common/extension.h"

class IotivityExtension : public common::Extension {
 public:
  IotivityExtension();
  virtual ~IotivityExtension();

 private:
  // common::Extension implementation.
  virtual common::Instance* CreateInstance();
};

#endif  // IOTIVITY_EXTENSION_H_


