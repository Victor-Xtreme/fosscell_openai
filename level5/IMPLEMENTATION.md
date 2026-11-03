# Level 5 submission

Reviewed upstream `Victor-Xtreme/Plastopol-Website` at commit
`a78f6bd` and fixed a concrete CMS search-highlighting defect. The existing
`README.md` contains the original assignment and is unchanged.

## Files

- `submission/Plastopol CMS/src/lib/search.ts`: updated implementation.
- `submission/Plastopol CMS/tests/search.test.mjs`: six regression cases.
- `submission/Plastopol CMS/package.json`: adds the test command.
- `0001-fix-cms-search-highlighting.patch`: complete, directly applicable Git
  patch, including the implementation, tests, and package-script change.
- `PULL_REQUEST.md`: prepared PR title, explanation, and validation details.

`submission` is a review copy of the three changed files, not a standalone app.
The complete working checkout is `/tmp/fosscell-plastopol-level5`, on branch
`codex/fix-search-highlighting`, with commit `b21dbe6`. That checkout has a clean
working tree. Temporary directories may be removed by the operating system;
the patch and review copies in this clone preserve the work.

## Apply to another Plastopol checkout

From a clean checkout based on the reviewed upstream revision:

```sh
git switch -c codex/fix-search-highlighting
git am /Users/keshavmahesh/nigga/fosscell_openai/level5/0001-fix-cms-search-highlighting.patch
cd "Plastopol CMS"
npm test
```

The tests require Node 22.6+ and no installed npm dependencies. The full app still
uses its existing dependencies for builds. `PULL_REQUEST.md` contains the
validation performed here and the build limitation.

## Publication status

The feature branch and commit are local. No pull request has been published.
The connected GitHub account has no accessible `Plastopol-Website` fork, the
GitHub connector provides no fork-creation tool, and the GitHub CLI is absent.
To submit, create a fork, push this branch to it, and open a PR targeting
`Victor-Xtreme/Plastopol-Website:main` using `PULL_REQUEST.md` as the description.
