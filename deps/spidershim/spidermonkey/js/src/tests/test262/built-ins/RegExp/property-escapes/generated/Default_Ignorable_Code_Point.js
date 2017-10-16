// |reftest| skip -- regexp-unicode-property-escapes is not supported
// Copyright 2017 Mathias Bynens. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Mathias Bynens
description: >
  Unicode property escapes for `Default_Ignorable_Code_Point`
info: |
  Generated by https://github.com/mathiasbynens/unicode-property-escapes-tests
  Unicode v10.0.0
  Emoji v5.0 (UTR51)
esid: sec-static-semantics-unicodematchproperty-p
features: [regexp-unicode-property-escapes]
includes: [regExpUtils.js]
---*/

const matchSymbols = buildString({
  loneCodePoints: [
    0x0000AD,
    0x00034F,
    0x00061C,
    0x003164,
    0x00FEFF,
    0x00FFA0
  ],
  ranges: [
    [0x00115F, 0x001160],
    [0x0017B4, 0x0017B5],
    [0x00180B, 0x00180E],
    [0x00200B, 0x00200F],
    [0x00202A, 0x00202E],
    [0x002060, 0x00206F],
    [0x00FE00, 0x00FE0F],
    [0x00FFF0, 0x00FFF8],
    [0x01BCA0, 0x01BCA3],
    [0x01D173, 0x01D17A],
    [0x0E0000, 0x0E0FFF]
  ]
});
testPropertyEscapes(
  /^\p{Default_Ignorable_Code_Point}+$/u,
  matchSymbols,
  "\\p{Default_Ignorable_Code_Point}"
);
testPropertyEscapes(
  /^\p{DI}+$/u,
  matchSymbols,
  "\\p{DI}"
);

const nonMatchSymbols = buildString({
  loneCodePoints: [],
  ranges: [
    [0x00DC00, 0x00DFFF],
    [0x000000, 0x0000AC],
    [0x0000AE, 0x00034E],
    [0x000350, 0x00061B],
    [0x00061D, 0x00115E],
    [0x001161, 0x0017B3],
    [0x0017B6, 0x00180A],
    [0x00180F, 0x00200A],
    [0x002010, 0x002029],
    [0x00202F, 0x00205F],
    [0x002070, 0x003163],
    [0x003165, 0x00DBFF],
    [0x00E000, 0x00FDFF],
    [0x00FE10, 0x00FEFE],
    [0x00FF00, 0x00FF9F],
    [0x00FFA1, 0x00FFEF],
    [0x00FFF9, 0x01BC9F],
    [0x01BCA4, 0x01D172],
    [0x01D17B, 0x0DFFFF],
    [0x0E1000, 0x10FFFF]
  ]
});
testPropertyEscapes(
  /^\P{Default_Ignorable_Code_Point}+$/u,
  nonMatchSymbols,
  "\\P{Default_Ignorable_Code_Point}"
);
testPropertyEscapes(
  /^\P{DI}+$/u,
  nonMatchSymbols,
  "\\P{DI}"
);

reportCompare(0, 0);
