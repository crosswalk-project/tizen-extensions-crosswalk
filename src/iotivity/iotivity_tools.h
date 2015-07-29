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
#ifndef IOTIVITY_IOTIVITY_TOOLS_H_
#define IOTIVITY_IOTIVITY_TOOLS_H_

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <mutex> // NOLINT
#include <condition_variable> //NOLINT

#include "common/picojson.h"

#include "OCPlatform.h"
#include "OCApi.h"

using namespace OC; // NOLINT
using namespace std; // NOLINT

#ifdef __cplusplus
extern "C" {
#endif

#define INFO_MSG(msg, ...) { printf(msg, ##__VA_ARGS__);}
#define DEBUG_MSG(msg, ...) { if (pDebugEnv) printf(msg, ##__VA_ARGS__);}
#define ERROR_MSG(msg) { std::cerr << msg << std::endl; }  // TODO(aphao) dlog
#define SUCCESS_RESPONSE 0

extern char *pDebugEnv;

void PrintfOcResource(const OCResource & oCResource);
void PrintfOcRepresentation(const OCRepresentation & oCRepresentation);
void UpdateOcRepresentation(const OCRepresentation & oCReprSource,
                            OCRepresentation & oCReprDest,
                            std::vector<std::string> & updatedPropertyNames);
void TranslateOCRepresentationToPicojson(
    const OCRepresentation & oCRepresentation,
    picojson::object & objectRes);
void CopyInto(std::vector<std::string> &src, picojson::array &dest);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // IOTIVITY_IOTIVITY_TOOLS_H_
