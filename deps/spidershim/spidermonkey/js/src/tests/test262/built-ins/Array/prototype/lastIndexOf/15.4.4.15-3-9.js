// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-array.prototype.lastindexof
es5id: 15.4.4.15-3-9
description: >
    Array.prototype.lastIndexOf - value of 'length' is a number (value
    is -Infinity)
---*/

        var obj = { 0: 0, length: -Infinity };

assert.sameValue(Array.prototype.lastIndexOf.call(obj, 0), -1, 'Array.prototype.lastIndexOf.call(obj, 0)');

reportCompare(0, 0);
