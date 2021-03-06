// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 15.4.4.20-9-c-iii-29
description: >
    Array.prototype.filter - false prevents element added to output
    Array
---*/

        var called = 0;

        function callbackfn(val, idx, obj) {
            called++;
            return val > 10;
        }

        var obj = { 0: 11, 1: 8, length: 20 };

        var newArr = Array.prototype.filter.call(obj, callbackfn);

assert.sameValue(newArr.length, 1, 'newArr.length');
assert.notSameValue(newArr[0], 8, 'newArr[0]');
assert.sameValue(called, 2, 'called');

reportCompare(0, 0);
