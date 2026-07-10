import type { Options, SearchOptions } from 'minisearch'

/**
 * A document in the search index. `desc` holds the full plain text and is
 * indexed (for recall) but NOT stored; `summary` is the short, table-collapsed
 * excerpt that is stored for display.
 */
export interface SearchItem {
  id: number
  kind: string
  name: string
  desc: string
  summary: string
  deprecated_since?: string
  removed_since?: string
  status?: 'deprecated' | 'removed'
}

/** A result row: the stored display fields plus MiniSearch's id and score. */
export type SearchHit = Pick<
  SearchItem,
  'kind' | 'name' | 'summary' | 'deprecated_since' | 'removed_since' | 'status'
> & { id: number; score: number }

/**
 * Index/load options. These MUST be identical on the build side
 * (`new MiniSearch`) and the client side (`MiniSearch.loadJSON`) or the
 * serialized index won't deserialize correctly. Note `desc` is indexed but not
 * stored, so the full text never ships verbatim — only its terms.
 */
export const MINISEARCH_OPTIONS: Options<SearchItem> = {
  idField: 'id',
  fields: ['name', 'desc'],
  storeFields: [
    'kind',
    'name',
    'summary',
    'deprecated_since',
    'removed_since',
    'status',
  ],
}

// Multipliers that demote deprecated/removed entries in ranking without hiding
// them — the BM25 equivalent of the old additive STATUS_PENALTY.
const STATUS_BOOST: Record<string, number> = { deprecated: 0.6, removed: 0.3 }

export const SEARCH_OPTIONS: SearchOptions = {
  // Name matches outrank description matches; exact > prefix > fuzzy.
  boost: { name: 3 },
  prefix: true,
  fuzzy: 0.2,
  weights: { fuzzy: 0.4, prefix: 0.7 },
  boostDocument: (_id, _term, stored) => {
    const status = stored?.status
    return typeof status === 'string' ? (STATUS_BOOST[status] ?? 1) : 1
  },
}
