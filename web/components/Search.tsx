import Fuse from 'fuse.js'
import type { FuseResult } from 'fuse.js'
import React, {
  Fragment,
  useCallback,
  useEffect,
  useRef,
  useState,
} from 'react'
import type { SearchIndex, SearchItem } from '../utils/search'
import { Dialog, Transition, Combobox } from '@headlessui/react'
import { Search as SearchIcon } from 'lucide-react'
import { useRouter } from 'next/router'

interface SearchResultProps {
  result: FuseResult<SearchItem>
  active: boolean
}

const KIND_STYLES: Record<string, string> = {
  var: 'bg-blue-500',
  config: 'bg-green-500',
  lua: 'bg-red-500',
}

const SearchResult: React.FunctionComponent<SearchResultProps> = ({
  active,
  result,
}) => {
  const selection = active ? 'bg-slate-300 dark:bg-slate-700' : ''
  const kindClass = KIND_STYLES[result.item.kind] ?? ''
  const excerpt =
    result.item.desc.length <= 120
      ? result.item.desc
      : `${result.item.desc.slice(0, 120)}…`

  return (
    <div className={`m-1 rounded flex flex-col p-2 ${selection}`}>
      <hr className="mb-2 border-black/10 dark:border-white/10" />
      <div className="flex justify-between">
        <div>
          <code className="text-lg p-1 mx-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
            {result.item.name}
          </code>
        </div>
        <div className={`rounded bg-opacity-20 p-1 ${kindClass}`}>
          {result.item.kind}
        </div>
      </div>
      <div>
        <p className="w-11/12">{excerpt}</p>
      </div>
    </div>
  )
}

const Search: React.FunctionComponent = () => {
  const router = useRouter()
  const inputRef = useRef<HTMLInputElement | null>(null)
  const [searchText, setSearchText] = useState('')
  const [searchResults, setSearchResults] = useState<FuseResult<SearchItem>[]>(
    []
  )
  const [fuse, setFuse] = React.useState<Fuse<SearchItem>>()

  const loadFuse = async () => {
    const data = await fetch('/static/fuse-index.json')
    const searchIndex: SearchIndex = await data.json()
    return new Fuse(searchIndex.list, {}, Fuse.parseIndex(searchIndex.index))
  }

  React.useEffect(() => {
    void loadFuse().then((nextFuse) => setFuse(nextFuse))
  }, [])

  const [isOpen, setIsOpen] = useState(false)
  const handleKeyPress = useCallback(
    (event: KeyboardEvent) => {
      if (event.key === 'k' && (event.metaKey || event.ctrlKey) && isOpen) {
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
    [isOpen]
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
    const searchResult = fuse.search(value)
    setSearchResults(searchResult)
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
        void router.push(`/lua#${value.item.name}`, undefined, { scroll: false })
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
          <Transition.Child
            as={Fragment}
            enter="ease-out duration-300"
            enterFrom="opacity-0"
            enterTo="opacity-100"
          >
            <div className="fixed inset-0 bg-black bg-opacity-25 backdrop-blur-sm" />
          </Transition.Child>

          <Transition.Child
            as={Fragment}
            enter="ease-out duration-300"
            enterFrom="opacity-0 scale-95"
            enterTo="opacity-100 scale-100"
          >
            <div className="fixed inset-0">
              <div className="flex h-screen w-screen items-start justify-center p-16 text-center">
                <Dialog.Panel className="flex flex-col max-h-full w-full max-w-2xl p-1 bg-gray-200 dark:bg-gray-800 transform rounded-xl text-left align-middle shadow transition-all border border-gray-800 dark:border-white border-opacity-10 dark:border-opacity-10">
                  <Combobox nullable onChange={onChange}>
                    <div className="flex">
                      <Combobox.Label className="flex items-center ml-2">
                        <SearchIcon size={32} strokeWidth={2} />
                      </Combobox.Label>
                      <Combobox.Input
                        ref={inputRef}
                        placeholder="Search docs (/ or ⌘K)"
                        className="mx-1 p-2 w-full bg-gray-200 dark:bg-gray-800 outline-none"
                        onChange={(e) => setSearch(e.target.value)}
                      />
                    </div>
                    <Combobox.Options className="flex flex-col h-full overflow-auto">
                      {searchResults.length === 0 && searchText !== '' ? (
                        <div className="relative cursor-default select-none py-2 px-4 text-gray-500">
                          No results.
                        </div>
                      ) : (
                        searchResults.map((r) => (
                          <Combobox.Option key={r.refIndex} value={r}>
                            {({ selected: _selected, active }) => (
                              <SearchResult
                                active={active}
                                result={r}
                              />
                            )}
                          </Combobox.Option>
                        ))
                      )}
                    </Combobox.Options>
                  </Combobox>
                </Dialog.Panel>
              </div>
            </div>
          </Transition.Child>
        </Dialog>
      </Transition>
    </>
  )
}

export default Search
