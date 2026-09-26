import * as fs from 'fs/promises'
import { buildSearchIndex } from './search'

export async function writeSearchIndex() {
  await fs.mkdir('public').catch(() => {})
  await fs.mkdir('public/static').catch(() => {})
  // MiniSearch serializes via JSON.stringify; the client restores it with
  // MiniSearch.loadJSON using the same MINISEARCH_OPTIONS.
  const index = buildSearchIndex()
  await fs.writeFile('public/static/search-index.json', JSON.stringify(index))
}
