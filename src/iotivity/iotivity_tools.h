/*
 * Copyright (c) 2015 Cisco and/or its affiliates. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Cisco nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef IOTIVITY_IOTIVITY_TOOLS_H_
#define IOTIVITY_IOTIVITY_TOOLS_H_

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <mutex>               // NOLINT
#include <condition_variable>  //NOLINT

#include "common/picojson.h"

#include "OCPlatform.h"
#include "OCApi.h"

using namespace OC;   // NOLINT
using namespace std;  // NOLINT

#ifdef __cplusplus
extern "C" {
#endif

#define INFO_MSG(msg, ...) \
  { printf(msg, ##__VA_ARGS__); }
#define DEBUG_MSG(msg, ...)                    \
  {                                            \
    if (pDebugEnv) printf(msg, ##__VA_ARGS__); \
  }
#define ERROR_MSG(msg) \
  { std::cerr << msg << std::endl; }
#define SUCCESS_RESPONSE 0

extern char *pDebugEnv;

void PrintfOcResource(const OCResource &oCResource);
void PrintfOcRepresentation(const OCRepresentation &oCRepresentation);
void UpdateOcRepresentation(const OCRepresentation &oCReprSource,
                            OCRepresentation &oCReprDest,
                            std::vector<std::string> &updatedPropertyNames);
void TranslateOCRepresentationToPicojson(
    const OCRepresentation &oCRepresentation, picojson::object &objectRes);
void CopyInto(std::vector<std::string> &src, picojson::array &dest);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // IOTIVITY_IOTIVITY_TOOLS_H_
