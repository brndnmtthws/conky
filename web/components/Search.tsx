import Fuse, { FuseResult } from 'fuse.js'
import React, { Fragment, useCallback, useEffect, useState } from 'react'
import { Search as SearchIcon } from 'react-feather'
import { SearchIndex, SearchItem } from '../utils/search'
import { Dialog, Transition, Combobox } from '@headlessui/react'
import { useRouter } from 'next/router'

export interface SearchProps {}

interface SearchResultProps {
  result: FuseResult<SearchItem>
  selected: boolean
  active: boolean
}

const SearchResult: React.FunctionComponent<SearchResultProps> = (props) => {
  const excerpt = (s: string) => {
    if (s.length < 120) {
      return <>{s}</>
    } else {
      return <>{s.substring(0, 120)}&hellip;</>
    }
  }
  const bg_for = (s: string) => {
    const bg = 'p-1 rounded bg-opacity-20 '
    if (s === 'var') {
      return bg + 'bg-blue-500'
    }
    if (s === 'config') {
      return bg + 'bg-green-500'
    }
    if (s === 'lua') {
      return bg + 'bg-red-500'
    }
    return bg
  }
  const selection = props.active ? 'bg-slate-300 dark:bg-slate-700' : ''

  return (
    <div className={`m-1 rounded flex flex-col p-2 ${selection}`}>
      <hr className="border-black/10 dark:border-white/10 mb-2"/>
      <div className="flex justify-between">
        <div>
          <code className="text-lg p-1 mx-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
            {props.result.item.name}
          </code>
        </div>
        <div className={bg_for(props.result.item.kind)}>
          {props.result.item.kind}
        </div>
      </div>
      <div>
        <p className="w-11/12">{excerpt(props.result.item.desc)}</p>
      </div>
    </div>
  )
}

const Search: React.FunctionComponent<SearchProps> = () => {
  const router = useRouter()
  const [searchText, setSearchText] = useState('')
  const [searchResults, setSearchResults] = useState<FuseResult<SearchItem>[]>(
    []
  )
  const [fuse, setFuse] = React.useState<Fuse<SearchItem> | undefined>(
    undefined
  )
  const fusePromise = async () => {
    const data = await fetch('/static/fuse-index.json')
    console.log(data)
    const searchIndex: SearchIndex = await data.json()
    console.log(searchIndex)
    return new Fuse(searchIndex.list, {}, Fuse.parseIndex(searchIndex.index))
  }
  React.useEffect(() => {
    fusePromise().then((fuse) => setFuse(fuse))
  }, [])

  const [isOpen, setIsOpen] = useState(false)
  const handleKeyPress = useCallback(
    (event: KeyboardEvent) => {
      if (event.key === 'k' && (event.metaKey || event.ctrlKey) && isOpen) {
        setIsOpen(false)
        event.preventDefault()
      } else if (
        (event.key == '/' ||
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

  if (!fuse) {
    return (
      <div className="flex items-center ml-2">
        <SearchIcon size={32} />
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
        router.push(`/variables#${value.item.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.item.kind === 'config') {
        router.push(`/config_settings#${value.item.name}`, undefined, {
          scroll: false,
        })
      }
      if (value.item.kind === 'lua') {
        router.push(`/lua#${value.item.name}`, undefined, { scroll: false })
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
      <div className="flex items-center ml-2">
        <button onClick={openModal} title="Search (/ or ⌘K)">
          <SearchIcon size={32} />
        </button>
      </div>

      <Transition appear show={isOpen} as={Fragment}>
        <Dialog as="div" className="relative z-10" onClose={closeModal}>
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
                        <SearchIcon size={32} />
                      </Combobox.Label>
                      <Combobox.Input
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
                            {({ selected, active }) => (
                              <SearchResult
                                result={r}
                                selected={selected}
                                active={active}
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
