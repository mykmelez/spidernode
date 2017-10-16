// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-array.prototype.indexof
es5id: 15.4.4.14-1-7
description: Array.prototype.indexOf applied to string primitive
---*/

assert.sameValue(Array.prototype.indexOf.call("abc", "b"), 1, 'Array.prototype.indexOf.call("abc", "b")');

reportCompare(0, 0);
