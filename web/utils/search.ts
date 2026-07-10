import MiniSearch from 'minisearch'
import {
  getConfigSettings,
  getLua,
  getVariables,
  toPlainText,
} from './doc-utils'
import type { DocItem } from './doc-utils'
import { MINISEARCH_OPTIONS, type SearchItem } from './search-config'

function toSearchItem(id: number, kind: string, v: DocItem): SearchItem {
  return {
    id,
    kind,
    name: v.name,
    // Full text — indexed for recall (table contents stay searchable).
    desc: toPlainText(v.desc_html),
    // Short display excerpt with `<table>` collapsed to `[table]`.
    summary: toPlainText(v.desc_html, 121, true),
    deprecated_since: v.deprecated_since,
    removed_since: v.removed_since,
    status: v.status,
  }
}

export function buildSearchIndex(): MiniSearch<SearchItem> {
  const sources: [string, DocItem][] = [
    ...getConfigSettings().values.map((v) => ['config', v] as [string, DocItem]),
    ...getVariables().values.map((v) => ['var', v] as [string, DocItem]),
    ...getLua().values.map((v) => ['lua', v] as [string, DocItem]),
  ]
  const docs = sources.map(([kind, v], id) => toSearchItem(id, kind, v))

  const index = new MiniSearch<SearchItem>(MINISEARCH_OPTIONS)
  index.addAll(docs)
  return index
}
