import type { Documentation } from '../utils/doc-utils'
import { Link as LinkIcon, CircleCheck, CircleX } from 'lucide-react'
import { Fragment } from 'react'
import parse from 'html-react-parser'
import type { HTMLReactParserOptions, DOMNode } from 'html-react-parser'

export interface DocsProps {
  docs: Documentation
  braces: boolean
  assign: boolean
}

// Support tables (e.g. the nvidia variable) use ✅ / ❌ emoji. Swap them for
// lucide icons at render time while keeping the YAML/man page plain text.
const parseOptions: HTMLReactParserOptions = {
  replace: (node: DOMNode) => {
    if (!('data' in node) || !/[✅❌]/.test(node.data)) return
    return (
      <>
        {node.data.split(/([✅❌])/).map((part, i) => {
          if (part === '✅') {
            return (
              <CircleCheck
                key={i}
                size={18}
                aria-label="Supported"
                className="inline-block align-middle text-green-600 dark:text-green-400"
              />
            )
          }
          if (part === '❌') {
            return (
              <CircleX
                key={i}
                size={18}
                aria-label="Not supported"
                className="inline-block align-middle text-red-600 dark:text-red-400"
              />
            )
          }
          return <Fragment key={i}>{part}</Fragment>
        })}
      </>
    )
  },
}

export default function Docs({ docs, braces, assign }: DocsProps) {
  // Push removed settings to the bottom; everything else keeps its YAML order.
  const values = docs.values
    .map((doc, i) => ({ doc, i }))
    .sort(
      (a, b) =>
        Number(a.doc.status === 'removed') -
          Number(b.doc.status === 'removed') || a.i - b.i,
    )
    .map(({ doc }) => doc)

  return (
    <>
      <div className="px-2 lg:px-4">{parse(docs.desc_html, parseOptions)}</div>
      <div className="divide-y divide-gray-700/25 dark:divide-gray-300/25">
        {values.map((doc) => {
          return (
            <div
              id={doc.name}
              key={doc.name}
              className={
                'p-2 sm:p-3 scroll-mt-16 overflow-auto bg-slate-200 dark:bg-slate-800 target:bg-rose-300 target:dark:bg-rose-900 hover:bg-opacity-25 dark:hover:bg-opacity-25 hover:ring-1 ring-black dark:ring-white ring-inset'
              }
            >
              <div className="flex items-baseline gap-2 pb-2 lg:pb-4">
                <a href={`#${doc.name}`} className="mt-1 h-fit self-stretch">
                  <LinkIcon size={18} strokeWidth={2} />
                </a>
                <div className="flex flex-wrap items-center">
                  {braces && (
                    <code className="self-baseline text-lg">$&#123;</code>
                  )}
                  <a
                    href={`#${doc.name}`}
                    data-anchor-name={doc.name}
                    className="no-underline self-baseline"
                  >
                    <code className="text-lg px-1.5 mx-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
                      {doc.name}
                    </code>
                  </a>
                  {doc.args?.length > 0 && (
                    <>
                      {assign && <code className="ps-1 text-lg">=</code>}
                      {doc.args.map((arg) => (
                        <code
                          className="text-base italic px-1 bg-slate-200 dark:bg-slate-800 whitespace-nowrap"
                          key={arg}
                        >
                          {arg}
                        </code>
                      ))}
                    </>
                  )}
                  {braces && (
                    <code className="self-baseline-last text-lg">&#125;</code>
                  )}
                </div>
                <span className="grow shrink"></span>
                {doc.status === 'deprecated' && (
                  <span
                    className="align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-amber-200 text-amber-900 dark:bg-amber-900 dark:text-amber-200"
                    title={`This setting is deprecated${doc.deprecated_since ? ` since ${doc.deprecated_since}` : ''} and may be removed in a future release.`}
                  >
                    Deprecated
                    {doc.deprecated_since && ` since ${doc.deprecated_since}`}
                  </span>
                )}
                {doc.status === 'removed' && (
                  <span
                    className="align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-red-200 text-red-900 dark:bg-red-900 dark:text-red-200"
                    title={`This setting was removed${doc.removed_since ? ` in ${doc.removed_since}` : ''} and no longer has any effect.`}
                  >
                    Removed{doc.removed_since && ` in ${doc.removed_since}`}
                  </span>
                )}
              </div>
              <div className="prose-base lg:prose-lg lg:px-2">
                {parse(doc.desc_html, parseOptions)}
                {typeof doc.default !== 'undefined' && (
                  <div className="p-1">
                    Default:{' '}
                    <code className="px-2 bg-slate-200 dark:bg-slate-800">
                      {doc.default}
                    </code>
                  </div>
                )}
              </div>
            </div>
          )
        })}
      </div>
    </>
  )
}
