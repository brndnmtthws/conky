import Link from 'next/link'
import CopyleftIcon from './CopyleftIcon'
import { buildMeta } from '../utils/build-meta'

const Footer: React.FunctionComponent = () => {
  const { date, year, gitHash } = buildMeta
  const formattedDate = date
    ? new Date(date).toLocaleString('en-US', {
        dateStyle: 'short',
        timeStyle: 'medium',
        timeZone: 'UTC',
      })
    : 'local dev'

  return (
    <div className="max-w-4xl mx-auto flex items-center px-4 py-6 text-zinc-500 dark:text-zinc-400">
      <div className="pr-3">
        <CopyleftIcon width={20} height={20} />
      </div>
      <div className="font-sans text-xs">
        <p>
          {year} Conky developers, updated {formattedDate}
          {gitHash && (
            <>
              {' '}
              <Link
                target="_blank"
                href={`https://github.com/brndnmtthws/conky/commit/${gitHash}`}
                className="hover:text-zinc-800 dark:hover:text-zinc-100"
              >
                {`(${gitHash})`}
              </Link>
            </>
          )}
        </p>
      </div>
    </div>
  )
}

export default Footer
