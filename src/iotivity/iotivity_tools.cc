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
#include "iotivity/iotivity_tools.h"

char* pDebugEnv = NULL;

void PrintfOcResource(const OCResource& oCResource) {
  DEBUG_MSG("PrintfOcResource\n");
  DEBUG_MSG("Res[sId] = %s\n", oCResource.sid().c_str());
  DEBUG_MSG("Res[Uri] = %s\n", oCResource.uri().c_str());
  DEBUG_MSG("Res[Host] = %s\n", oCResource.host().c_str());
  DEBUG_MSG("Res[Resource types] \n");

  for (const auto& resourceTypes : oCResource.getResourceTypes()) {
    DEBUG_MSG("\t\t%s\n", resourceTypes.c_str());
  }

  DEBUG_MSG("Res[Resource interfaces] \n");

  for (const auto& resourceInterfaces : oCResource.getResourceInterfaces()) {
    DEBUG_MSG("\t\t%s\n", resourceInterfaces.c_str());
  }
}

void PrintfOcRepresentation(const OCRepresentation& oCRepr) {
  DEBUG_MSG("PrintfOcRepresentation\n");

  std::string uri = oCRepr.getUri();
  DEBUG_MSG("uri=%s\n", uri.c_str());

  for (const auto& cur : oCRepr) {
    std::string attrname = cur.attrname();

    if (AttributeType::String == cur.type()) {
      std::string curStr = cur.getValue<string>();
      DEBUG_MSG("Rep[String]: key=%s, value=%s\n", attrname.c_str(),
                curStr.c_str());
    } else if (AttributeType::Integer == cur.type()) {
      DEBUG_MSG("Rep[Integer]: key=%s, value=%d\n", attrname.c_str(),
                cur.getValue<int>());
    } else if (AttributeType::Double == cur.type()) {
      DEBUG_MSG("Rep[Double]: key=%s, value=%f\n", attrname.c_str(),
                cur.getValue<double>());
    } else if (AttributeType::Boolean == cur.type()) {
      DEBUG_MSG("Rep[Boolean]: key=%s, value=%d\n", attrname.c_str(),
                cur.getValue<bool>());
    }
  }
}

static void UpdateDestOcRepresentationString(OCRepresentation& oCReprDest,
                                             std::string attributeName,
                                             std::string value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::String == cur.type()) {
        std::string oldValue = cur.getValue<string>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%s, new=%s\n", attrname.c_str(),
                  oldValue.c_str(), value.c_str());
      } else {
        DEBUG_MSG("Updated[%s] mismatch String\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationInt(OCRepresentation& oCReprDest,
                                          std::string attributeName,
                                          int value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Integer == cur.type()) {
        int oldValue = cur.getValue<int>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else if (AttributeType::Double == cur.type()) {
        // Force int to double
        double oldValue = cur.getValue<double>();
        cur = value;
        DEBUG_MSG("F-Updated[%s] old=%f, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Int\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationBool(OCRepresentation& oCReprDest,
                                           std::string attributeName,
                                           bool value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Boolean == cur.type()) {
        bool oldValue = cur.getValue<bool>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%d, new=%d\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Bool\n", attrname.c_str());
        return;
      }
    }
  }
}

static void UpdateDestOcRepresentationDouble(OCRepresentation& oCReprDest,
                                             std::string attributeName,
                                             double value) {
  for (auto& cur : oCReprDest) {
    std::string attrname = cur.attrname();
    if (attrname == attributeName) {
      if (AttributeType::Double == cur.type()) {
        double oldValue = cur.getValue<double>();
        cur = value;
        DEBUG_MSG("Updated[%s] old=%f, new=%f\n", attrname.c_str(), oldValue,
                  value);
      } else if (AttributeType::Integer == cur.type()) {
        int oldValue = cur.getValue<int>();
        cur = value;
        DEBUG_MSG("F-Updated[%s] old=%d, new=%f\n", attrname.c_str(), oldValue,
                  value);
      } else {
        DEBUG_MSG("Updated[%s] mismatch Double\n", attrname.c_str());
        return;
      }
    }
  }
}

void UpdateOcRepresentation(const OCRepresentation& oCReprSource,
                            OCRepresentation& oCReprDest,
                            std::vector<std::string>& updatedPropertyNames) {
  DEBUG_MSG("\n\nUpdateOcRepresentation: Source\n");
  PrintfOcRepresentation(oCReprSource);

  DEBUG_MSG("UpdateOcRepresentation: Destination\n");
  PrintfOcRepresentation(oCReprDest);

  std::vector<std::string> foundPropertyNames;
  for (auto& cur : oCReprSource) {
    std::string attrname = cur.attrname();

    DEBUG_MSG("SRC attrname=%s\n", attrname.c_str());

    AttributeValue destAttrValue;
    if (oCReprDest.getAttributeValue(attrname, destAttrValue) == false) {
      DEBUG_MSG("DST attrname=%s NOT FOUND !!!\n", attrname.c_str());
      continue;
    }

    if (std::find(updatedPropertyNames.begin(), updatedPropertyNames.end(),
                  attrname) != updatedPropertyNames.end()) {
      if (AttributeType::String == cur.type()) {
        DEBUG_MSG("cur.type String\n");
        std::string newValue = cur.getValue<string>();
        UpdateDestOcRepresentationString(oCReprDest, attrname, newValue);
      } else if (AttributeType::Integer == cur.type()) {
        DEBUG_MSG("cur.type Integer\n");
        int newValue = cur.getValue<int>();
        UpdateDestOcRepresentationInt(oCReprDest, attrname, newValue);
      } else if (AttributeType::Boolean == cur.type()) {
        DEBUG_MSG("cur.type Boolean\n");
        bool newValue = cur.getValue<bool>();
        UpdateDestOcRepresentationBool(oCReprDest, attrname, newValue);
      } else if (AttributeType::Double == cur.type()) {
        DEBUG_MSG("cur.type Double\n");
        double newValue = cur.getValue<double>();
        UpdateDestOcRepresentationDouble(oCReprDest, attrname, newValue);
      }
    }
  }
}

// Translate OCRepresentation to picojson
void TranslateOCRepresentationToPicojson(const OCRepresentation& oCRepr,
                                         picojson::object& objectRes) {
  objectRes["uri"] = picojson::value(oCRepr.getUri());
  for (auto& cur : oCRepr) {
    std::string attrname = cur.attrname();

    if (AttributeType::String == cur.type()) {
      std::string curStr = cur.getValue<string>();
      objectRes[attrname] = picojson::value(curStr);
    } else if (AttributeType::Integer == cur.type()) {
      int intValue = cur.getValue<int>();
      objectRes[attrname] = picojson::value(static_cast<double>(intValue));
    } else if (AttributeType::Double == cur.type()) {
      double doubleValue = cur.getValue<double>();
      objectRes[attrname] = picojson::value(static_cast<double>(doubleValue));
    } else if (AttributeType::Boolean == cur.type()) {
      bool boolValue = cur.getValue<bool>();
      objectRes[attrname] = picojson::value(static_cast<bool>(boolValue));
    }
  }
}

void CopyInto(std::vector<std::string>& src, picojson::array& dest) {
  for (int i = 0; i < src.size(); i++) {
    std::string str = src[i];
    dest.push_back(picojson::value(str));
  }
}
