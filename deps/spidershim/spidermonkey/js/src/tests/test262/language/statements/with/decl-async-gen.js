// |reftest| error:SyntaxError
// Copyright (C) 2017 André Bargull. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-with-statement
description: >
  AsyncGeneratorDeclaration is not allowed in statement position
info: |
  ExpressionStatement[Yield, Await] :
    [lookahead ∉ { {, function, async [no LineTerminator here] function, class, let [ }]
    Expression[+In, ?Yield, ?Await] ;
negative:
  phase: early
  type: SyntaxError
features: [async-iteration]
flags: [noStrict]
---*/

throw "Test262: This statement should not be evaluated.";

with ({}) async function* g() {}
