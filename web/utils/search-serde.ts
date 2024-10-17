import * as fs from 'fs/promises'
import { createSearchIndex } from './search'

export async function writeSearchIndex() {
  await fs.mkdir('public').catch(() => {})
  await fs.mkdir('public/static').catch(() => {})
  const index = createSearchIndex()
  await fs.writeFile('public/static/fuse-index.json', JSON.stringify(index))
}
