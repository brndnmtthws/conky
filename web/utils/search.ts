import Fuse from 'fuse.js'
import { getConfigSettings, getLua, getVariables } from './doc-utils'
export interface SearchItem {
  kind: string
  name: string
  desc: string
}
export interface SearchIndex {
  index: {
    keys: readonly string[]
    records: Fuse.FuseIndexRecords
  }
  list: SearchItem[]
}

export function getSearchIndex() {
  const cs: SearchItem[] = getConfigSettings().values.map((v) => ({
    kind: 'config',
    name: v.name,
    desc: v.desc,
  }))
  const vars: SearchItem[] = getVariables().values.map((v) => ({
    kind: 'var',
    name: v.name,
    desc: v.desc,
  }))
  const lua: SearchItem[] = getLua().values.map((v) => ({
    kind: 'lua',
    name: v.name,
    desc: v.desc,
  }))
  const list: SearchItem[] = [...cs, ...vars, ...lua]

  return {
    list: list.map((item) => ({
      ...item,
      desc: item.desc.substring(0, 121),
    })),
    index: Fuse.createIndex<SearchItem>(['name', 'desc'], list).toJSON(),
  }
}
