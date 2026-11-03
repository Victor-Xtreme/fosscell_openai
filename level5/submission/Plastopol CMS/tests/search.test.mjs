// Run with Node.js 22.6+ using npm test in Plastopol CMS.
import assert from "node:assert/strict";
import test from "node:test";
import { highlightText } from "../src/lib/search.ts";

test("highlights matches case-insensitively while preserving case", () => {
  assert.equal(highlightText("Chair CHAIR", " chair "), "<mark>Chair</mark> <mark>CHAIR</mark>");
});

test("matches ampersands and angle brackets before HTML escaping", () => {
  assert.equal(highlightText("A & B", "&"), "A <mark>&amp;</mark> B");
  assert.equal(highlightText("<chair>", "<chair>"), "<mark>&lt;chair&gt;</mark>");
});

test("does not match or corrupt generated HTML entities", () => {
  assert.equal(highlightText("A & B", "amp"), "A &amp; B");
  assert.equal(highlightText("<chair>", "lt"), "&lt;chair&gt;");
  assert.equal(highlightText("A & B amp", "amp"), "A &amp; B <mark>amp</mark>");
});

test("treats regular expression syntax as literal text", () => {
  assert.equal(highlightText("a+b [x] a+b", "a+b"), "<mark>a+b</mark> [x] <mark>a+b</mark>");
  assert.equal(highlightText("[x]", "[x]"), "<mark>[x]</mark>");
  assert.equal(highlightText("$&", "$&"), "<mark>$&amp;</mark>");
});

test("escapes HTML-like content even inside a highlighted match", () => {
  assert.equal(highlightText('<img src=x onerror="alert(1)">', '<img'),
    '<mark>&lt;img</mark> src=x onerror="alert(1)"&gt;');
});

test("handles empty queries, no matches, and empty text", () => {
  assert.equal(highlightText("<&>", "  "), "&lt;&amp;&gt;");
  assert.equal(highlightText("<&>", "chair"), "&lt;&amp;&gt;");
  assert.equal(highlightText("", "chair"), "");
  assert.equal(highlightText("", ""), "");
});
