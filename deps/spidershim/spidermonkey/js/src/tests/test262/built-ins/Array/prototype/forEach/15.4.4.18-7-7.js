// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-array.prototype.foreach
es5id: 15.4.4.18-7-7
description: >
    Array.prototype.forEach - considers new value of elements in array
    after the call
---*/

        var result = false;
        var arr = [1, 2, 3, 4, 5];

        function callbackfn(val, Idx, obj) {
            arr[4] = 6;
            if (val >= 6) {
                result = true;
            }
        }

        arr.forEach(callbackfn);

assert(result, 'result !== true');

reportCompare(0, 0);
