// |reftest| error:SyntaxError
// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
info: "StringLiteral :: \"\\\\\\\" or '\\\\\\' is not correct"
es5id: 7.8.4_A3.2_T1
description: Checking if execution of "\\\" fails
negative:
  phase: early
  type: SyntaxError
---*/

throw "Test262: This statement should not be evaluated.";

//CHECK#1
"\\\"
