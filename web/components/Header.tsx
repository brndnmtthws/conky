import Link from 'next/link'
import GitHub from './GitHub'
import { LineChart } from './LineChart'
import ThemeSwitcher from './ThemeSwitcher'

type HeaderProps = {
  name: string
  darkMode: boolean
  setDarkMode: (state: boolean) => void
}

export default function Header({ name, darkMode, setDarkMode }: HeaderProps) {
  return (
    <div className="border-b-1 backdrop-blur-lg bg-white dark:bg-black bg-opacity-20 dark:bg-opacity-20 transition">
      <header className="max-w-2xl mx-auto m-1 p-1 grow flex w-full">
        <h1 className="px-2 text-3xl dark:text-white self-end">
          <Link href="/">
            <a>
              <strong>{name}</strong>
            </a>
          </Link>
        </h1>
        <LineChart width={400} height={40} darkMode={darkMode} />
        <div className="flex justify-end">
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
