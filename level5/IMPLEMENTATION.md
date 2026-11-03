# Level 5: CMS search highlighting fix

Reviewed `Victor-Xtreme/Plastopol-Website` at commit
`a78f6bdd9bceef1ded4a50f56678ef0bfa47051c` and made one focused improvement in
`Plastopol CMS/src/lib/search.ts`.

## Problem and change

The CMS sidebar highlights product names using HTML strings. The original
helper escaped the name before searching it, allowing matches to split HTML
entities. Searching `&` in `A & B` produced `A <mark>&</mark>amp; B`; searching
`amp` could highlight characters absent from the original name.

The fix matches the original text first, then escapes every matched and
unmatched segment before adding `<mark>` tags. It preserves case-insensitive
matching, literal regex characters, whitespace trimming, and the existing API.
No new dependencies or architectural changes are introduced.

## Deliverable

`0001-fix-preserve-HTML-entities-in-CMS-search-highlightin.patch` is a Git mail
patch containing the implementation, eight regression tests, a CMS `npm test`
script, and test documentation. It can be applied to the upstream repository
without copying the whole codebase into this exercise repository.

From a clean checkout of `Plastopol-Website` at the reviewed commit, apply it
using the actual path to this submission:

```sh
git switch -c fix/cms-search-highlighting
git am /path/to/fosscell_openai/level5/0001-fix-preserve-HTML-entities-in-CMS-search-highlightin.patch
cd "Plastopol CMS"
npm test
```

Tests use Node's built-in runner and TypeScript stripping; verified with Node
24.19.0. No npm installation or running Tauri app is needed for these tests.

## Validation

Before the fix, five of eight regression tests failed. After the fix, all
eight passed, covering ampersands, angle brackets, entity-looking names,
escaped markup, repeated case-insensitive matches, literal regex symbols,
and empty/unmatched queries. The complete application build was not run.

## Proposed upstream PR

**Title:** Fix CMS search highlighting for HTML special characters

Match original product-name text before HTML escaping so highlighting cannot
split entities or match characters introduced by escaping. Escape all output
segments and preserve existing search behavior. Add eight passing regression
tests without external test dependencies.

This submission is stored in `iam-darshan/fosscell_openai`. An upstream PR has
not been opened; pushing directly to the upstream repository was denied.
