import { Dispatch, SetStateAction } from 'react'

const sunIcon = (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width="25"
    height="24"
    fill="none"
    viewBox="0 0 25 24"
    className="dark:opacity-50"
  >
    <g
      stroke="#fff"
      strokeLinecap="round"
      strokeLinejoin="round"
      strokeWidth="2"
      clipPath="url(#clip0_192_823)"
    >
      <path d="M12.5 17a5 5 0 100-10 5 5 0 000 10zM12.5 1v2M12.5 21v2M4.72 4.22l1.42 1.42M18.86 18.36l1.42 1.42M1.5 12h2M21.5 12h2M4.72 19.78l1.42-1.42M18.86 5.64l1.42-1.42"></path>
    </g>
    <defs>
      <clipPath id="clip0_192_823">
        <path
          className="fill-current text-white"
          d="M0 0H24V24H0z"
          transform="translate(.5)"
        ></path>
      </clipPath>
    </defs>
  </svg>
)

const moonIcon = (
  <svg
    xmlns="http://www.w3.org/2000/svg"
    width="25"
    height="24"
    fill="none"
    viewBox="0 0 21 20"
  >
    <path
      stroke="#fff"
      strokeLinecap="round"
      strokeLinejoin="round"
      strokeWidth="2"
      className="stroke-current text-gray-400 dark:text-white"
      d="M19.5 10.79A9 9 0 119.71 1a7 7 0 009.79 9.79v0z"
    ></path>
  </svg>
)

type Props = {
  setDarkMode: (state: boolean) => void
}
export default function ThemeSwitcher({ setDarkMode }: Props) {
  return (
    <div className="flex bg-gray-200 dark:bg-gray-700 rounded-2xl">
      <button
        type="button"
        aria-label="Use Dark Mode"
        title="Use Dark Mode"
        onClick={() => {
          setDarkMode(true)
        }}
        className="flex items-center dark:bg-purple-500 rounded-3xl justify-center align-center p-1 w-10 h-full transition"
      >
        {moonIcon}
      </button>

      <button
        type="button"
        aria-label="Use Light Mode"
        title="Use Light Mode"
        onClick={() => {
          setDarkMode(false)
        }}
        className="flex items-center bg-purple-500 dark:bg-transparent rounded-3xl justify-center align-center p-1 w-10 h-full transition"
      >
        {sunIcon}
      </button>
    </div>
  )
}
