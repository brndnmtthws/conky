import Fuse from 'fuse.js'
import type { FuseResult } from 'fuse.js'
import React, {
  Fragment,
  useCallback,
  useDeferredValue,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react'
import type { SearchIndex, SearchItem } from '../utils/search'
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
  result: FuseResult<SearchItem>
  active: boolean
}

// Results are grouped into sections by kind; the section header replaces the
// old per-row kind tag. Group order is by relevance, not this map.
const KIND_LABELS: Record<string, string> = {
  var: 'Variables',
  config: 'Config settings',
  lua: 'Lua API',
}

// Small ranking penalties added to a result's Fuse score (lower = better), so
// active entries outrank equally-matching deprecated/removed ones. Applied after
// search, so it only reorders — these items are still found, just demoted.
const STATUS_PENALTY: Record<string, number> = {
  deprecated: 0.1,
  removed: 0.2,
}

const SearchResult: React.FunctionComponent<SearchResultProps> = ({
  active,
  result,
}) => {
  const selection = active ? 'bg-slate-300 dark:bg-slate-700' : ''
  const excerpt =
    result.item.desc.length <= 120
      ? result.item.desc
      : `${result.item.desc.slice(0, 120)}…`

  return (
    <div
      className={`mx-1 rounded flex flex-col px-4 py-2 ${selection} cursor-pointer`}
    >
      <div className="flex items-center items-top">
        <code className="text-lg font-bold">{result.item.name}</code>
        <div className="grow shrink"></div>
        {result.item.status === 'deprecated' && (
          <span
            className="ml-2 align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-amber-200 text-amber-900 dark:bg-amber-900 dark:text-amber-200"
            title={`This setting is deprecated${result.item.deprecated_since ? ` since ${result.item.deprecated_since}` : ''} and may be removed in a future release.`}
          >
            Deprecated
          </span>
        )}
        {result.item.status === 'removed' && (
          <span
            className="ml-2 align-middle rounded px-2 py-0.5 text-xs font-semibold uppercase tracking-wide bg-red-200 text-red-900 dark:bg-red-900 dark:text-red-200"
            title={`This setting was removed${result.item.removed_since ? ` in ${result.item.removed_since}` : ''} and no longer has any effect.`}
          >
            Removed
          </span>
        )}
      </div>
      <p className="px-1 text-sm text-gray-600 dark:text-gray-400">{excerpt}</p>
    </div>
  )
}

const Search: React.FunctionComponent = () => {
  const router = useRouter()
  const inputRef = useRef<HTMLInputElement | null>(null)
  const [searchText, setSearchText] = useState('')
  const [fuse, setFuse] = React.useState<Fuse<SearchItem>>()

  // Keep typing responsive
  const deferredText = useDeferredValue(searchText)
  const searchResults = useMemo(() => {
    if (!fuse) return []
    // Re-rank by Fuse score plus the status penalty so deprecated/removed
    // entries sink below equally-matching active ones. The sort is stable, so
    // entries with the same adjusted score keep Fuse's relevance order.
    const adjusted = (r: FuseResult<SearchItem>) =>
      (r.score ?? 0) + (STATUS_PENALTY[r.item.status ?? ''] ?? 0)
    return [...fuse.search(deferredText, { limit: 30 })].sort(
      (a, b) => adjusted(a) - adjusted(b),
    )
  }, [fuse, deferredText])

  const loadFuse = async () => {
    const data = await fetch('/static/fuse-index.json')
    const searchIndex: SearchIndex = await data.json()
    return new Fuse(
      searchIndex.list,
      {
        // Name matches matter far more than description matches, and we want a
        // tighter threshold so unrelated fuzzy hits (e.g. "own_window" vaguely
        // matching "mpd_random") drop out. ignoreLocation lets a match count
        // anywhere in the string, not just near the start.
        keys: [
          { name: 'name', weight: 0.7 },
          { name: 'desc', weight: 0.3 },
        ],
        threshold: 0.4,
        ignoreLocation: true,
        includeScore: true,
      },
      Fuse.parseIndex(searchIndex.index),
    )
  }

  React.useEffect(() => {
    void loadFuse().then((nextFuse) => setFuse(nextFuse))
  }, [])

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

  if (!fuse) {
    return (
      <div className="flex h-10 w-10 items-center justify-center text-zinc-950 dark:text-white">
        <SearchIcon size={28} strokeWidth={2} />
      </div>
    )
  }

  const setSearch = (value: string) => {
    setSearchText(value)
  }
  const onChange = (value: FuseResult<SearchItem> | null | undefined) => {
    if (value) {
      if (value.item.kind === 'var') {
        void router.push(`/variables#${value.item.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.item.kind === 'config') {
        void router.push(`/config_settings#${value.item.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.item.kind === 'lua') {
        void router.push(`/lua#${value.item.name}`, undefined, {
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
  // relevance: Fuse returns results best-first, so the kind whose best match
  // appears earliest leads. This keeps grouping while letting a strong match
  // (e.g. an exact "own_window" setting) surface above weaker matches of
  // another kind.
  const groups: {
    kind: string
    label: string
    items: FuseResult<SearchItem>[]
  }[] = []
  const groupByKind = new Map<string, (typeof groups)[number]>()
  for (const result of searchResults) {
    let group = groupByKind.get(result.item.kind)
    if (!group) {
      group = {
        kind: result.item.kind,
        label: KIND_LABELS[result.item.kind],
        items: [],
      }
      groupByKind.set(result.item.kind, group)
      groups.push(group)
    }
    group.items.push(result)
  }

  return (
    <>
      <div className="flex items-center">
        <button
          onClick={openModal}
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
                <DialogPanel className="flex flex-col max-h-full w-full max-w-2xl bg-gray-200 dark:bg-gray-800 transform rounded-xl text-left align-middle shadow transition-all border border-gray-800/10 dark:border-white/10">
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
                      className={`${styles.results} flex flex-col h-full overflow-auto bg-gray-300 dark:bg-gray-900 rounded-b-xl ${searchResults.length && 'pb-1'}`}
                    >
                      {searchResults.length === 0
                        ? searchText !== '' && (
                            <div className="relative cursor-default select-none py-2 px-4 text-gray-500">
                              No results.
                            </div>
                          )
                        : groups.map((group) => (
                            <Fragment key={group.kind}>
                              <div
                                className={`${styles.group} px-3 pt-3 pb-1 text-xs font-semibold uppercase tracking-wider text-gray-500 dark:text-gray-400`}
                              >
                                {group.label}
                              </div>
                              {group.items.map((r) => (
                                <ComboboxOption key={r.refIndex} value={r}>
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
