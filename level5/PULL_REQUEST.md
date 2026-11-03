# Fix CMS search highlighting for HTML special characters

The CMS sidebar searches raw product text but previously highlighted matches in
HTML-escaped text. A search for `&` therefore matched a different representation,
and a search for `amp` could insert markup into the generated `&amp;` entity.

Match the original text, then escape each matched and unmatched segment before
adding `<mark>` tags. This keeps literal searches consistent with the sidebar
search, preserves case-insensitive matching, and prevents matching generated
entities. No production dependencies or architectural changes are introduced.

Validation: six regression cases passed against the actual TypeScript source,
using Node's built-in TypeScript stripping and strict assertions in the available
JavaScript runtime. Cases cover case preservation, HTML special characters,
entity corruption, regex literals, HTML-like input, and empty/no-match inputs.
`git diff --check` passed. The full CMS frontend build was not run because Node
and npm are unavailable on the shell PATH.

The added `npm test` command uses Node 22.6+ with built-in TypeScript stripping;
it adds no test dependencies or changes to the production runtime requirement.
