import Fuse from 'fuse.js'
import type { FuseIndexRecords } from 'fuse.js'
import { getConfigSettings, getLua, getVariables } from './doc-utils'

export interface SearchItem {
  kind: string
  name: string
  desc: string
  deprecated_since?: string
  removed_since?: string
  status?: 'deprecated' | 'removed'
}

export interface SearchIndex {
  index: {
    keys: readonly string[]
    records: FuseIndexRecords
  }
  list: SearchItem[]
}

export function createSearchIndex(): SearchIndex {
  const cs: SearchItem[] = getConfigSettings().values.map((v) => ({
    kind: 'config',
    name: v.name,
    desc: v.desc.substring(0, 121),
    deprecated_since: v.deprecated_since,
    removed_since: v.removed_since,
    status: v.status,
  }))
  const vars: SearchItem[] = getVariables().values.map((v) => ({
    kind: 'var',
    name: v.name,
    desc: v.desc.substring(0, 121),
    deprecated_since: v.deprecated_since,
    removed_since: v.removed_since,
    status: v.status,
  }))
  const lua: SearchItem[] = getLua().values.map((v) => ({
    kind: 'lua',
    name: v.name,
    desc: v.desc.substring(0, 121),
    deprecated_since: v.deprecated_since,
    removed_since: v.removed_since,
    status: v.status,
  }))
  const list: SearchItem[] = [...cs, ...vars, ...lua]

  return {
    list,
    index: Fuse.createIndex<SearchItem>(['name', 'desc'], list).toJSON(),
  }
}
