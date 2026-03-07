import { useEffect, useState } from 'react'
import Footer from './Footer'
import Header from './Header'

interface LayoutProps {
  children: React.ReactNode
}

export default function Layout({ children }: LayoutProps) {
  const [darkMode, setDarkMode] = useState(
    () =>
      typeof document !== 'undefined' &&
      document.documentElement.classList.contains('dark')
  )

  useEffect(() => {
    if (darkMode) {
      document.documentElement.classList.add('dark')
      localStorage.setItem('theme', 'dark')
    } else {
      document.documentElement.classList.remove('dark')
      localStorage.setItem('theme', 'light')
    }
  }, [darkMode])

  useEffect(() => {
    const darkQuery = window.matchMedia('(prefers-color-scheme: dark)')
    const onChange = (event: MediaQueryListEvent) => {
      if (!window.localStorage.getItem('theme')) {
        setDarkMode(event.matches)
      }
    }

    darkQuery.addEventListener('change', onChange)

    return () => {
      darkQuery.removeEventListener('change', onChange)
    }
  }, [])

  return (
    <div className="min-h-screen bg-transparent text-zinc-950 dark:text-zinc-50">
      <div className="sticky top-0 z-10">
        <Header name="Conky" setDarkMode={setDarkMode} />
      </div>
      <div className="relative pb-6 pt-4 md:pt-6">
        <div className="mx-auto flex w-full max-w-4xl flex-col items-center px-4">
          {children}
        </div>
      </div>
      <div className="relative">
        <Footer />
      </div>
    </div>
  )
}
