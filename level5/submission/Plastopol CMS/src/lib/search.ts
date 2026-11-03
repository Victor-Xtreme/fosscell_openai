// src/lib/search.ts

import type { Product } from "./types";

export interface SearchMatch {
  product: Product;
  fields: string[]; // which fields matched
}

export function searchProducts(
  products: Product[],
  query: string
): SearchMatch[] {
  const q = query.toLowerCase().trim();
  if (!q) return products.map((p) => ({ product: p, fields: [] }));

  const results: SearchMatch[] = [];

  for (const product of products) {
    const fields: string[] = [];

    if (product.modelName.toLowerCase().includes(q)) fields.push("modelName");
    if (product.description.toLowerCase().includes(q)) fields.push("description");
    if (product.category.toLowerCase().includes(q)) fields.push("category");
    if (product.tags.some((t) => t.toLowerCase().includes(q))) fields.push("tags");
    if (product.slug.toLowerCase().includes(q)) fields.push("slug");

    if (fields.length > 0) results.push({ product, fields });
  }

  return results;
}

export function highlightText(text: string, query: string): string {
  if (!query.trim()) return escapeHtml(text);
  const escaped = escapeRegex(query.trim());
  const re = new RegExp(`(${escaped})`, "gi");
  // Match original text so HTML entities cannot be split or matched by accident.
  return text
    .split(re)
    .map((part, index) =>
      index % 2 === 1 ? `<mark>${escapeHtml(part)}</mark>` : escapeHtml(part)
    )
    .join("");
}

function escapeHtml(str: string): string {
  return str
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function escapeRegex(str: string): string {
  return str.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
