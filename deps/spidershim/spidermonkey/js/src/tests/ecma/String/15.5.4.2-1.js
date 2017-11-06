/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/**
   File Name:          15.5.4.2-1.js
   ECMA Section:       15.5.4.2 String.prototype.toString()

   Description:        Returns this string value.  Note that, for a String
   object, the toString() method happens to return the same
   thing as the valueOf() method.

   The toString function is not generic; it generates a
   runtime error if its this value is not a String object.
   Therefore it connot be transferred to the other kinds of
   objects for use as a method.

   Author:             christine@netscape.com
   Date:               1 october 1997
*/

var SECTION = "15.5.4.2-1";
var TITLE   = "String.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( "String.prototype.toString()",        "",     String.prototype.toString() );
new TestCase( "(new String()).toString()",          "",     (new String()).toString() );
new TestCase( "(new String(\"\")).toString()",      "",     (new String("")).toString() );
new TestCase( "(new String( String() )).toString()","",    (new String(String())).toString() );
new TestCase( "(new String( \"h e l l o\" )).toString()",       "h e l l o",    (new String("h e l l o")).toString() );
new TestCase( "(new String( 0 )).toString()",       "0",    (new String(0)).toString() );

test();
