// Copyright (c) 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var signature_to_type = { 'n': 'number',
                          'f': 'function',
                          'b': 'boolean',
                          's': 'string',
                          'o': 'object'
                        };

function Utils() {
}

Utils.prototype.validateArguments = function(signature, args) {
  var full_args = Array.prototype.slice.call(args);

  // After '?' everything is optional.
  var mandatory_len = signature.indexOf('?') === -1 ? signature.length : signature.indexOf('?');

  if (full_args.length < mandatory_len)
    return false;

  // Mandatory arguments.
  for (var i = 0; i < mandatory_len; i++) {
    if (typeof full_args[i] !== signature_to_type[signature[i]] || full_args[i] === null)
      return false;
  }

  // Optional args may be null.
  for (var i = mandatory_len; i < full_args.length && i < signature.length - 1; i++) {
    if (full_args[i] !== null && typeof full_args[i] !== signature_to_type[signature[i + 1]])
      return false;
  }

  return true;
};

Utils.prototype.validateObject = function(object, signature, attributes) {
  for (var i = 0; i < signature.length; i++) {
    if (object.hasOwnProperty(attributes[i]) &&
        typeof object[attributes[i]] !== signature_to_type[signature[i]]) {
      return false;
    }
  }

  return true;
};

exports = new Utils();
