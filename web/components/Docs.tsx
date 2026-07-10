import type { Documentation } from '../utils/doc-utils'
import { Link as LinkIcon, CircleCheck, CircleX } from 'lucide-react'
import type { MouseEvent, MouseEventHandler } from 'react'
import { Fragment, useCallback, useEffect } from 'react'
import parse from 'html-react-parser'
import type { HTMLReactParserOptions, DOMNode } from 'html-react-parser'
import Router from 'next/router'
import styles from './Docs.module.css'
import Link from 'next/link'

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
  // On hash navigation, center the target entry when it fits entirely in the
  // viewport; otherwise fall back to top-alignment (which honours scroll-mt-16)
  // so the start of a tall entry stays visible.

  const focusEntry = useCallback((el: Element) => {
    const prev: HTMLElement | null = document.querySelector('[data-target]')
    if (prev === el) {
      return
    }
    if (prev) {
      prev.removeAttribute('data-target')
    }
    el.setAttribute('data-target', '')

    requestAnimationFrame(() => {
      const bounds = el.getBoundingClientRect()
      // Element.scrollIntoView({block: "center"}) was VERY inconsistent; here goes a manual implementation:
      const marginTop = Number(window.getComputedStyle(el).scrollMarginTop.slice(0,-2))
      const fits = bounds.height <= (window.innerHeight - marginTop) * /* allowance: */ 0.8
      const elPosY = window.scrollY + bounds.top
      if (!fits) {
        window.scrollTo(window.scrollX, elPosY - marginTop)
      } else {
        window.scrollTo(window.scrollX, elPosY - (window.innerHeight - bounds.height) / 2)
      }
    })

    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) return

    let prev_scroll = window.scrollY
    let still = 0
    let moved = false
    const watch = () => {
      const y = window.scrollY
      if (y !== prev_scroll) {
        moved = true
        still = 0
        prev_scroll = y
      } else {
        still += 1
      }
      if ((moved && still >= 2) || (!moved && still >= 3)) {
        // Mix the flash into the entry's own background so there's no second
        // source of truth to keep in sync with its bg-slate-* classes.
        el.animate(
          [
            { '--glint-offset': "-15%", '--glint-opacity': "80%" },
            { '--glint-offset': "50%", '--glint-opacity': "100%" },
            { '--glint-offset': "140%", '--glint-opacity': "80%" }
          ],
          { duration: 1000, easing: 'ease-out' },
        )
        return
      }
      requestAnimationFrame(watch)
    }
    requestAnimationFrame(watch)
  }, [])

  const focusHash = useCallback(
    (ev?: unknown) => {
      if (ev instanceof /* SVG or HTML */ Element) {
        const target = ev.closest('[id]')
        if (target) {
          focusEntry(target)
          return
        }
      }

      const id = decodeURIComponent(window.location.hash.slice(1))
      const el = id && document.getElementById(id)
      if (el) focusEntry(el)
    },
    [focusEntry],
  )

  useEffect(() => {
    // Next's router.push (used by the search box) navigates via history
    // pushState, which doesn't emit a native `hashchange`. Bridge Next's own
    // event into focusHash so same-page jumps from search still focus the entry.
    window.addEventListener('hashchange', focusHash)
    Router.events.on('hashChangeComplete', focusHash)
    requestAnimationFrame(focusHash)
    return () => {
      window.removeEventListener('hashchange', focusHash)
      Router.events.off('hashChangeComplete', focusHash)
    }
  }, [focusHash])

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
      <div
        className={`${styles['doc-listing']} divide-y divide-gray-400 dark:divide-gray-700`}
      >
        {values.map((doc) => {
          const handler = ((e: MouseEvent) => {
            e.preventDefault()
            history.pushState(null, '', `#${doc.name}`)
            focusHash(e.target)
          }) as MouseEventHandler<HTMLAnchorElement>

          return (
            <Fragment key={doc.name}>
              <div
                className={`${styles['focus-indicator']}`}
                aria-hidden="true"
              ></div>
              <div
                id={doc.name}
                className={`${styles["doc-entry"]} p-2 sm:p-3 scroll-mt-17 bg-slate-200 dark:bg-slate-800 hover:ring-1 ring-black dark:ring-white ring-inset`}
              >
                <div className="flex items-baseline gap-2 pb-2 lg:pb-4">
                  <Link
                    href={`#${doc.name}`}
                    onClick={handler}
                    className="mt-1 h-fit self-stretch"
                  >
                    <LinkIcon size={18} strokeWidth={2} />
                  </Link>
                  <div className="flex flex-wrap items-center">
                    {braces && (
                      <code className="self-baseline text-lg">$&#123;</code>
                    )}
                    <Link
                      href={`#${doc.name}`}
                      onClick={handler}
                      data-anchor-name={doc.name}
                      className="no-underline self-baseline"
                    >
                      <code className="text-lg px-1.5 mx-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
                        {doc.name}
                      </code>
                    </Link>
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
            </Fragment>
          )
        })}
      </div>
    </>
  )
}
