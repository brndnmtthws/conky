import fs from 'fs'
import path from 'path'
import yaml from 'js-yaml'
import { unified } from 'unified'
import remarkParse from 'remark-parse'
import remarkGfm from 'remark-gfm'
import remarkRehype from 'remark-rehype'
import rehypeStringify from 'rehype-stringify'

const DOC_PATH = path.join(process.cwd(), '..', 'doc')

export interface Documentation {
  desc: string
  desc_md: string
  values: DocItem[]
}
export interface DocItem {
  name: string
  desc: string
  desc_md: string
  default: string | undefined
  args: string[]
}

function getDocumentation(source: string): Documentation {
  const configSettingsFile = fs.readFileSync(
    path.join(DOC_PATH, source),
    'utf-8'
  )
  const parsed = yaml.load(configSettingsFile.toString()) as Documentation
  const docs = {
    ...parsed,
    desc_md: processMarkdown(parsed.desc),
    values: parsed.values.map((c) => ({
      ...c,
      desc_md: processMarkdown(c.desc),
    })),
  }

  return docs
}

export function getConfigSettings(): Documentation {
  return getDocumentation('config_settings.yaml')
}

export function getVariables(): Documentation {
  return getDocumentation('variables.yaml')
}
export function getLua(): Documentation {
  return getDocumentation('lua.yaml')
}

function processMarkdown(input: string): string {
  return unified()
    .use(remarkParse)
    .use(remarkGfm)
    .use(remarkRehype)
    .use(rehypeStringify)
    .processSync(input)
    .toString()
}
