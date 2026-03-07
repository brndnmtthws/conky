import { Moon, Sun } from 'lucide-react'

type Props = {
  setDarkMode: (state: boolean) => void
}
export default function ThemeSwitcher({ setDarkMode }: Props) {
  return (
    <div className="flex rounded-full border border-zinc-900/10 bg-zinc-100/90 p-0.5 dark:border-white/10 dark:bg-zinc-900">
      <button
        type="button"
        aria-label="Use Dark Mode"
        title="Use Dark Mode"
        onClick={() => {
          setDarkMode(true)
        }}
        className="flex h-9 w-9 items-center justify-center rounded-full text-zinc-500 transition hover:text-zinc-900 dark:bg-zinc-800 dark:text-zinc-200 dark:hover:text-white"
      >
        <Moon size={20} strokeWidth={2} />
      </button>

      <button
        type="button"
        aria-label="Use Light Mode"
        title="Use Light Mode"
        onClick={() => {
          setDarkMode(false)
        }}
        className="flex h-9 w-9 items-center justify-center rounded-full bg-white text-zinc-900 shadow-sm transition hover:bg-zinc-50 dark:bg-transparent dark:text-zinc-500 dark:shadow-none dark:hover:text-white"
      >
        <Sun size={20} strokeWidth={2} />
      </button>
    </div>
  )
}
