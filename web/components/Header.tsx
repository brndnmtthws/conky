import { Github } from 'lucide-react'
import Link from 'next/link'
import { useRouter } from 'next/router'
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
      className={`mx-0.5 inline-flex h-10 items-center rounded px-3 hover:ring-1 ring-black dark:ring-white hover:bg-rose-300 dark:hover:bg-rose-700 ${bg}`}
    >
      {props.name}
    </Link>
  )
}

export default function Header({ name, setDarkMode }: HeaderProps) {
  const router = useRouter()

  return (
    <div className="border-b border-zinc-900/15 bg-white/80 backdrop-blur-md transition dark:border-white/10 dark:bg-black/40">
      <header className="mx-auto flex w-full max-w-4xl items-center px-4 py-3">
        <h1 className="mr-4 text-3xl font-bold leading-none tracking-tight text-zinc-950 dark:text-white">
          <Link href="/" data-cy="top-link" className="inline-flex h-10 items-center">
            {name}
          </Link>
        </h1>
        {router.asPath !== '/' && (
          <div className="mr-2 hidden items-center text-md sm:flex">
            <NavLink href="/variables" name="Vars" />
            <NavLink href="/config_settings" name="Config" />
            <NavLink href="/lua" name="Lua" />
          </div>
        )}
        <div className="flex-grow" />
        <div className="flex items-center gap-1.5">
          <Search />
          <div className="mx-1 flex h-10 items-center border-r border-zinc-900/15 px-2 dark:border-white/10">
            <a
              href="https://github.com/brndnmtthws/conky"
              className="inline-flex h-10 w-10 items-center justify-center text-zinc-950 transition hover:text-zinc-600 dark:text-white dark:hover:text-zinc-300"
            >
              <Github size={28} strokeWidth={2} />
            </a>
          </div>
          <ThemeSwitcher setDarkMode={setDarkMode} />
        </div>
      </header>
      {router.asPath !== '/' && (
        <div className="mx-auto flex max-w-4xl items-center px-3 pb-2 text-md sm:hidden">
          <NavLink href="/variables" name="Vars" />
          <NavLink href="/config_settings" name="Config" />
          <NavLink href="/lua" name="Lua" />
        </div>
      )}
    </div>
  )
}
