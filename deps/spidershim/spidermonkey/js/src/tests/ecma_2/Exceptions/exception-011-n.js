/* -*- indent-tabs-mode: nil; js-indent-level: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


var SECTION = "exception-011";
var TITLE   = "Don't Crash throwing undefined";

writeHeaderToLog( SECTION + " "+ TITLE);

print("Undefined throw test.");

DESCRIPTION = "throw undefined";

new TestCase( "throw undefined",  "error", eval("throw (void 0)") );

test();

print("FAILED!: Should have exited with uncaught exception.");



