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
  desc_html: string
  values: DocItem[]
}
export interface DocItem {
  name: string
  desc: string
  desc_html: string
  default: string | undefined
  args: string[]
  /** Version the setting was deprecated in, e.g. "1.24.3". */
  deprecated_since?: string
  /** Version the setting was removed in, e.g. "1.23.0". */
  removed_since?: string
  /** Derived lifecycle marker; computed from the `*_since` fields. */
  status?: 'deprecated' | 'removed'
}

function getDocumentation(source: string): Documentation {
  const configSettingsFile = fs.readFileSync(
    path.join(DOC_PATH, source),
    'utf-8',
  )
  const parsed = yaml.load(configSettingsFile.toString()) as Documentation
  const docs = {
    ...parsed,
    desc_html: processMarkdown(parsed.desc),
    values: parsed.values.map((c) => {
      // `removed_since` takes precedence: a removed setting may also have been
      // deprecated in an earlier version. Omit `status` entirely when neither
      // applies — `getStaticProps` cannot serialize an explicit `undefined`.
      const status = c.removed_since
        ? ('removed' as const)
        : c.deprecated_since
          ? ('deprecated' as const)
          : undefined
      return {
        ...c,
        desc_html: processMarkdown(c.desc),
        ...(status ? { status } : {}),
      }
    }),
  }

  return docs
}

export function filterDesc(docs: Documentation): Documentation {
  return {
    ...docs,
    desc: '',
    values: docs.values.map((v) => ({
      ...v,
      desc: '',
    })),
  }
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

/**
 * Convert rendered description HTML (`desc_html`) to plain text for search and
 * excerpt use: strip tags, decode entities, collapse whitespace.
 *
 * With `limit`, only the first `limit` characters of the output are kept.
 *
 * With `eatTables`, whole `<table>…</table>` blocks collapse to `[table]`.
 * This avoids printing unreadable cell soup in e.g. search summaries.
 */
export function toPlainText(
  html: string,
  limit?: number,
  eatTables = false,
): string {
  const collapsed = eatTables
    ? html.replace(/<table[\s\S]*?<\/table>/gi, ' [table] ')
    : html
  return collapsed
    .replace(/<[^>]*$/, '') // drop a tag cut in half by the slice
    .replace(/<\/?[^>]+>/g, ' ') // strip tags
    .replace(/&#x([0-9a-f]+);/gi, (_, h: string) =>
      String.fromCodePoint(parseInt(h, 16)),
    )
    .replace(/&#(\d+);/g, (_, d: string) => String.fromCodePoint(Number(d)))
    .replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>')
    .replace(/&quot;/g, '"')
    .replace(/&#39;|&apos;/g, "'")
    .replace(/&amp;/g, '&') // decode last so "&amp;lt;" stays "&lt;", not "<"
    .replace(/\s+/g, ' ')
    .trim()
    .slice(0, limit)
}

function processMarkdown(input: string): string {
  return unified()
    .use(remarkParse)
    .use(remarkGfm)
    .use(remarkRehype, { allowDangerousHtml: true })
    .use(rehypeStringify, { allowDangerousHtml: true })
    .processSync(input)
    .toString()
}
