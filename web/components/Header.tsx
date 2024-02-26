import Link from 'next/link'
import { useRouter } from 'next/router'
import GitHub from './GitHub'
import ThemeSwitcher from './ThemeSwitcher'

import * as React from 'react'
import Search from './Search'

type HeaderProps = {
  name: string
  setDarkMode: (state: boolean) => void
}

interface NavLinkProps {
  href: string
  name: string
}

const NavLink: React.FunctionComponent<NavLinkProps> = (props) => {
  const router = useRouter()
  const bg = router.asPath.startsWith(props.href)
    ? 'bg-rose-100 dark:bg-rose-900'
    : ''
  return (
    <Link
      href={props.href}
      className={`m-0.5 p-1 self-end hover:ring-1 ring-black dark:ring-white hover:bg-rose-300 dark:hover:bg-rose-700 ${bg} rounded`}
    >
      {props.name}
    </Link>
  )
}

export default function Header({ name, setDarkMode }: HeaderProps) {
  const router = useRouter()

  return (
    <div className="border-b-1 backdrop-blur-lg bg-white dark:bg-black bg-opacity-20 dark:bg-opacity-20 transition">
      <header className="max-w-3xl mx-auto m-0 p-1 grow flex w-full">
        <h1 className="px-2 text-3xl dark:text-white self-end">
          <Link href="/" className="font-bold" data-cy="top-link">
            {name}
          </Link>
        </h1>
        {router.asPath != '/' && (
          <div className="flex text-md items-stretch self-stretch mx-1">
            <NavLink href="/variables" name="Vars" />
            <NavLink href="/config_settings" name="Config" />
            <NavLink href="/lua" name="Lua" />
          </div>
        )}
        <div className="flex-grow" />
        <Search />
        <div className="flex">
          <div className="border-r mx-1 px-1 border-slate-700">
            <a href="https://github.com/brndnmtthws/conky">
              <GitHub />
            </a>
          </div>
          <div className="mx-1 px-1 flex place-content-center place-items-center">
            <ThemeSwitcher setDarkMode={setDarkMode} />
          </div>
        </div>
      </header>
    </div>
  )
}
