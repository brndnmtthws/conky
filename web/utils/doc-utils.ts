import fs from 'fs'
import path from 'path'
import yaml from 'js-yaml'
import { unified } from 'unified'
import remarkParse from 'remark-parse'
import remarkGfm from 'remark-gfm'
import remarkRehype from 'remark-rehype'
import rehypeStringify from 'rehype-stringify'

const DOC_PATH = path.join(process.cwd(), '..', 'doc')

export interface ConfigSetting {
  name: string
  desc: string
  default: string | undefined
  args: string[]
}

export function getConfigSettings(): ConfigSetting[] {
  const configSettingsFile = fs.readFileSync(
    path.join(DOC_PATH, 'config_settings.yaml'),
    'utf-8'
  )
  const parsed = yaml.load(configSettingsFile.toString()) as ConfigSetting[]
  const docs = parsed.map((c) => ({ ...c, desc: processMarkdown(c.desc) }))

  return docs
}

export interface Variable {
  name: string
  desc: string
  default: string | undefined
  args: string[]
}

export function getVariables(): Variable[] {
  const variablesFile = fs.readFileSync(
    path.join(DOC_PATH, 'variables.yaml'),
    'utf-8'
  )
  const parsed = yaml.load(variablesFile.toString()) as Variable[]
  const docs = parsed.map((c) => ({ ...c, desc: processMarkdown(c.desc) }))

  return docs
}

export interface LuaDoc {
  name: string
  desc: string
  args: string[]
}

export function getLua(): LuaDoc[] {
  const variablesFile = fs.readFileSync(
    path.join(DOC_PATH, 'lua.yaml'),
    'utf-8'
  )
  const parsed = yaml.load(variablesFile.toString()) as LuaDoc[]
  const docs = parsed.map((c) => ({ ...c, desc: processMarkdown(c.desc) }))

  return docs
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
