// Copyright 2023 Ron Buckton. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Ron Buckton
description: >
  ignoreCase (`i`) modifier can be removed via `(?i:)` or `(?i-:)`.
info: |
  Runtime Semantics: CompileAtom
  The syntax-directed operation CompileAtom takes arguments direction (forward or backward) and modifiers (a Modifiers Record) and returns a Matcher.

  Atom :: `(` `?` RegularExpressionFlags `-` RegularExpressionFlags `:` Disjunction `)`
    1. Let addModifiers be the source text matched by the first RegularExpressionFlags.
    2. Let removeModifiers be the source text matched by the second RegularExpressionFlags.
    3. Let newModifiers be UpdateModifiers(modifiers, CodePointsToString(addModifiers), CodePointsToString(removeModifiers)).
    4. Return CompileSubpattern of Disjunction with arguments direction and newModifiers.

  UpdateModifiers ( modifiers, add, remove )
  The abstract operation UpdateModifiers takes arguments modifiers (a Modifiers Record), add (a String), and remove (a String) and returns a Modifiers. It performs the following steps when called:

  1. Let dotAll be modifiers.[[DotAll]].
  2. Let ignoreCase be modifiers.[[IgnoreCase]].
  3. Let multiline be modifiers.[[Multiline]].
  4. If add contains "s", set dotAll to true.
  5. If add contains "i", set ignoreCase to true.
  6. If add contains "m", set multiline to true.
  7. If remove contains "s", set dotAll to false.
  8. If remove contains "i", set ignoreCase to false.
  9. If remove contains "m", set multiline to false.
  10. Return the Modifiers Record { [[DotAll]]: dotAll, [[IgnoreCase]]: ignoreCase, [[Multiline]]: multiline }.

esid: sec-compileatom
features: [regexp-modifiers]
---*/

var re1 = /(?-i:fo)o/i;
assert(!re1.test("FOO"), "Pattern should not match as modified group does not ignore case");
assert(!re1.test("FOo"), "Pattern should not match as modified group does not ignore case");
assert(re1.test("foo"), "Pattern should not ignore case in modified group");
assert(re1.test("foO"), "Pattern should not ignore case in modified group");

var re2 = new RegExp("(?-i:fo)o", "i");
assert(!re2.test("FOO"), "Pattern should not match as modified group does not ignore case");
assert(!re2.test("FOo"), "Pattern should not match as modified group does not ignore case");
assert(re2.test("foo"), "Pattern should not ignore case in modified group");
assert(re2.test("foO"), "Pattern should not ignore case in modified group");

var re3 = /b(?-i:ar)/i;
assert(!re3.test("BAR"), "Pattern should not match as modified group does not ignore case");
assert(!re3.test("bAR"), "Pattern should not match as modified group does not ignore case");
assert(re3.test("bar"), "Pattern should not ignore case in modified group");
assert(re3.test("Bar"), "Pattern should not ignore case in modified group");

var re4 = new RegExp("b(?-i:ar)", "i");
assert(!re4.test("BAR"), "Pattern should not match as modified group does not ignore case");
assert(!re4.test("bAR"), "Pattern should not match as modified group does not ignore case");
assert(re4.test("bar"), "Pattern should not ignore case in modified group");
assert(re4.test("Bar"), "Pattern should not ignore case in modified group");

var re5 = /b(?-i:a)z/i;
assert(re5.test("baz"), "a should match a in baz");
assert(!re5.test("bAz"), "A should not match a in baz");
assert(re5.test("Baz"), "B should match b in baz");
assert(re5.test("baZ"), "Z should match z in baz");
assert(re5.test("BaZ"), "should match baz");
assert(!re5.test("BAZ"), "should not match baz");

var re6 = new RegExp("b(?-i:a)z", "i");
assert(re6.test("baz"), "a should match a in baz");
assert(!re6.test("bAz"), "A should not match a in baz");
assert(re6.test("Baz"), "B should match b in baz");
assert(re6.test("baZ"), "Z should match z in baz");
assert(re6.test("BaZ"), "should match baz");
assert(!re6.test("BAZ"), "should not match baz");
