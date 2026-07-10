import MiniSearch from 'minisearch'
import React, {
  Fragment,
  useCallback,
  useDeferredValue,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react'
import {
  MINISEARCH_OPTIONS,
  SEARCH_OPTIONS,
  type SearchHit,
  type SearchItem,
} from '../utils/search-config'
import {
  Dialog,
  Transition,
  Combobox,
  DialogPanel,
  Label,
  ComboboxInput,
  ComboboxOptions,
  ComboboxOption,
  TransitionChild,
} from '@headlessui/react'
import { Search as SearchIcon } from 'lucide-react'
import { useRouter } from 'next/router'
import styles from './Search.module.css'

interface SearchResultProps {
  result: SearchHit
  active: boolean
}

// Results are grouped into sections by kind; the section header replaces the
// old per-row kind tag. Group order is by relevance, not this map.
const KIND_LABELS: Record<string, string> = {
  var: 'Variables',
  config: 'Config settings',
  lua: 'Lua API',
}

const SearchResult: React.FunctionComponent<SearchResultProps> = ({
  active,
  result,
}) => {
  const selection = active ? 'bg-slate-300 dark:bg-slate-700' : ''
  const excerpt =
    result.summary.length <= 120
      ? result.summary
      : `${result.summary.slice(0, 120)}…`

  return (
    <div
      className={`mx-1 mb-1.25 rounded flex flex-col px-4 py-2 ${selection} cursor-pointer`}
    >
      <div className="flex items-center items-top">
        <code className="text-lg font-bold">{result.name}</code>
        <div className="grow shrink"></div>
        {result.status === 'deprecated' && (
          <span
            className="ml-2 align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-amber-200 text-amber-900 dark:bg-amber-900 dark:text-amber-200"
            title={`This setting is deprecated${result.deprecated_since ? ` since ${result.deprecated_since}` : ''} and may be removed in a future release.`}
          >
            Deprecated
          </span>
        )}
        {result.status === 'removed' && (
          <span
            className="ml-2 align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-red-200 text-red-900 dark:bg-red-900 dark:text-red-200"
            title={`This setting was removed${result.removed_since ? ` in ${result.removed_since}` : ''} and no longer has any effect.`}
          >
            Removed
          </span>
        )}
      </div>
      <p className="px-1 text-sm text-gray-600 dark:text-gray-400">{excerpt}</p>
    </div>
  )
}

async function loadIndex(): Promise<MiniSearch<SearchItem>> {
  const res = await fetch('/static/search-index.json')
  const json = await res.text()
  return MiniSearch.loadJSON<SearchItem>(json, MINISEARCH_OPTIONS)
}

const Search: React.FunctionComponent = () => {
  const router = useRouter()
  const inputRef = useRef<HTMLInputElement | null>(null)
  const [searchText, setSearchText] = useState('')
  const [index, setIndex] = useState<MiniSearch<SearchItem>>()

  // Keep typing responsive
  const deferredText = useDeferredValue(searchText)
  const searchResults = useMemo<SearchHit[]>(() => {
    if (!index || deferredText === '') return []
    // MiniSearch returns BM25-ranked results best-first; `boostDocument` in
    // SEARCH_OPTIONS already demotes deprecated/removed entries.
    const results = index.search(
      deferredText,
      SEARCH_OPTIONS
    ) as unknown as SearchHit[]
    // Promote an exact name match to the top. BM25 can rank a short-desc partial
    // match (e.g. `nvidia_display`) above the exact `nvidia`, but if the user
    // typed the exact name that's almost certainly what they want. Stable sort,
    // so everything else keeps its relevance order.
    const query = deferredText.trim().toLowerCase()
    results.sort(
      (a, b) =>
        Number(b.name.toLowerCase() === query) -
        Number(a.name.toLowerCase() === query)
    )
    return results.slice(0, 30)
  }, [index, deferredText])

  // Load the index once, lazily. The ref dedupes concurrent triggers (idle
  // preload vs. the user opening search) so we only ever fetch/parse once.
  const indexPromise = useRef<Promise<MiniSearch<SearchItem>> | null>(null)
  const ensureIndex = useCallback(() => {
    if (!indexPromise.current) {
      indexPromise.current = loadIndex()
      void indexPromise.current.then(setIndex)
    }
    return indexPromise.current
  }, [])

  // Preload during idle time, after the page has painted and settled, so the
  // index is usually ready before the user opens search — without competing
  // with initial load/hydration. `timeout` guarantees it still runs on a busy
  // page; falls back to a timer where requestIdleCallback is unavailable.
  useEffect(() => {
    const ric = window.requestIdleCallback
    if (ric) {
      const id = ric(() => void ensureIndex(), { timeout: 1000 })
      return () => window.cancelIdleCallback(id)
    }
    const t = window.setTimeout(() => void ensureIndex(), 500)
    return () => window.clearTimeout(t)
  }, [ensureIndex])

  const [isOpen, setIsOpen] = useState(false)
  const handleKeyPress = useCallback(
    (event: KeyboardEvent) => {
      if (event.key === 'Escape' && isOpen) {
        // The nested Combobox swallows the first Escape (to close itself), so
        // close the dialog here directly. This listener is on document, so it
        // still fires even though Combobox called preventDefault.
        setIsOpen(false)
      } else if (
        event.key === 'k' &&
        (event.metaKey || event.ctrlKey) &&
        isOpen
      ) {
        setIsOpen(false)
        event.preventDefault()
      } else if (
        (event.key === '/' ||
          (event.key === 'k' && (event.metaKey || event.ctrlKey))) &&
        !isOpen
      ) {
        setIsOpen(true)
        event.preventDefault()
      }
    },
    [isOpen],
  )
  useEffect(() => {
    document.addEventListener('keydown', handleKeyPress)

    return () => {
      document.removeEventListener('keydown', handleKeyPress)
    }
  }, [handleKeyPress])

  useEffect(() => {
    if (!isOpen) {
      return
    }

    const timeoutId = window.setTimeout(() => {
      inputRef.current?.focus()
      inputRef.current?.select()
    }, 0)

    return () => {
      window.clearTimeout(timeoutId)
    }
  }, [isOpen])

  // Ensure loading has started by the time the dialog opens, in case the user
  // beat the idle preload. Deduped, so it's a no-op if already loading/loaded.
  useEffect(() => {
    if (isOpen) void ensureIndex()
  }, [isOpen, ensureIndex])

  const setSearch = (value: string) => {
    setSearchText(value)
  }
  const onChange = (value: SearchHit | null | undefined) => {
    if (value) {
      if (value.kind === 'var') {
        void router.push(`/variables#${value.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.kind === 'config') {
        void router.push(`/config_settings#${value.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.kind === 'lua') {
        void router.push(`/lua#${value.name}`, undefined, {
          scroll: false,
        })
      }
      setIsOpen(false)
    }
  }

  const closeModal = () => {
    setIsOpen(false)
  }

  const openModal = () => {
    setIsOpen(true)
  }

  // Group results by kind, ordering both the groups and the rows within them by
  // relevance: MiniSearch returns results best-first, so the kind whose best
  // match appears earliest leads. This keeps grouping while letting a strong
  // match (e.g. an exact "own_window" setting) surface above weaker matches of
  // another kind.
  const groups: {
    kind: string
    label: string
    items: SearchHit[]
  }[] = []
  const groupByKind = new Map<string, (typeof groups)[number]>()
  for (const result of searchResults) {
    let group = groupByKind.get(result.kind)
    if (!group) {
      group = {
        kind: result.kind,
        label: KIND_LABELS[result.kind],
        items: [],
      }
      groupByKind.set(result.kind, group)
      groups.push(group)
    }
    group.items.push(result)
  }

  return (
    <>
      <div className="flex items-center">
        <button
          onClick={openModal}
          onPointerEnter={() => void ensureIndex()}
          onFocus={() => void ensureIndex()}
          title="Search (/ or ⌘K)"
          className="inline-flex h-10 w-10 items-center justify-center text-zinc-950 transition hover:text-zinc-600 dark:text-white dark:hover:text-zinc-300"
        >
          <SearchIcon size={28} strokeWidth={2} />
        </button>
      </div>

      <Transition appear show={isOpen} as={Fragment}>
        <Dialog
          as="div"
          className="relative z-10"
          initialFocus={inputRef}
          onClose={closeModal}
        >
          <TransitionChild
            as={Fragment}
            enter="ease-out duration-300"
            enterFrom="opacity-0"
            enterTo="opacity-100"
          >
            <div className="fixed inset-0 bg-black/80 backdrop-blur-xs" />
          </TransitionChild>

          <TransitionChild
            as={Fragment}
            enter="ease-out duration-300"
            enterFrom="opacity-0 scale-95"
            enterTo="opacity-100 scale-100"
          >
            <div className="fixed inset-0">
              {/* Close on outside click directly: the nested Combobox swallows
                  the first one otherwise. Only fires for clicks on this wrapper,
                  not inside the panel. Keyboard users dismiss via Escape (handled
                  in handleKeyPress), so the bare click handler doesn't
                  actually hurt accessibility. */}
              {/* eslint-disable-next-line jsx-a11y/no-static-element-interactions, jsx-a11y/click-events-have-key-events */}
              <div
                className="flex h-screen w-screen items-start justify-center p-16 text-center"
                onClick={(e) => {
                  if (e.target === e.currentTarget) closeModal()
                }}
              >
                <DialogPanel className="flex flex-col max-h-full w-full max-w-2xl bg-gray-200 dark:bg-gray-800 rounded-xl text-left align-middle shadow transition-all border border-gray-800/10 dark:border-white/10">
                  <Combobox value={null} immediate onChange={onChange}>
                    <div className="flex p-1">
                      <Label className="flex items-center ml-2">
                        <SearchIcon size={28} strokeWidth={2} />
                      </Label>
                      <ComboboxInput
                        ref={inputRef}
                        placeholder="Search docs (/ or ⌘K)"
                        className="mx-1 p-2 w-full bg-gray-200 dark:bg-gray-800 outline-none"
                        displayValue={() => searchText}
                        onChange={(e) => setSearch(e.target.value)}
                      />
                    </div>
                    <ComboboxOptions
                      static
                      className={`${styles.results} flex flex-col h-full overflow-auto bg-gray-300 dark:bg-gray-900 rounded-b-xl`}
                    >
                      {searchResults.length === 0
                        ? searchText !== '' && (
                            <div className="relative cursor-default select-none py-4 px-4 text-gray-500">
                              {index ? 'No results.' : 'Loading…'}
                            </div>
                          )
                        : groups.map((group) => (
                            <Fragment key={group.kind}>
                              <div
                                className={`${styles.group} px-3 pt-3 pb-1 text-xs font-semibold uppercase tracking-wider text-gray-500 dark:text-gray-400`}
                              >
                                {group.label}
                              </div>
                              {group.items.map((r: SearchHit) => (
                                <ComboboxOption key={r.id} value={r}>
                                  {({ focus }) => (
                                    <SearchResult active={focus} result={r} />
                                  )}
                                </ComboboxOption>
                              ))}
                            </Fragment>
                          ))}
                    </ComboboxOptions>
                  </Combobox>
                </DialogPanel>
              </div>
            </div>
          </TransitionChild>
        </Dialog>
      </Transition>
    </>
  )
}

export default Search
