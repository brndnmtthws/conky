import Link from 'next/link'
import { getDocuments, Document } from '../utils/mdx-utils'
import Layout from '../components/Layout'
import ArrowIcon from '../components/ArrowIcon'
import SEO from '../components/SEO'
import { getSearchIndex, SearchIndex } from '../utils/search'

const pages = [
  {
    slug: '/variables',
    title: 'Variables',
    desc: 'Variables let you define the various objects displayed in Conky including text, bars, graphs, and more.',
  },
  {
    slug: '/config_settings',
    title: 'Configuration settings',
    desc: 'Global configuration pramaters for Conky allow you to customize various behaviours.',
  },
  {
    slug: '/lua',
    title: 'Lua API',
    desc: 'Program wild things into your Conky with the Lua API',
  },
]

interface IndexItemProps {
  href: string
  as?: string
  title: string
  desc?: string
}

const IndexItem: React.FunctionComponent<IndexItemProps> = (props) => {
  return (
    <div className="md:first:rounded-t-lg md:last:rounded-b-lg backdrop-blur-lg bg-slate-300 dark:bg-black dark:bg-opacity-30 bg-opacity-10 hover:bg-opacity-30 dark:hover:bg-opacity-50 transition border border-gray-800 dark:border-white border-opacity-10 dark:border-opacity-10 border-b-0 last:border-b hover:border-b hovered-sibling:border-t-0">
      <Link
        as={props.as}
        href={props.href}
        className="py-2 lg:py-4 px-2 lg:px-4 block focus:outline-none focus:ring-4"
      >
        <h2 className="text-xl md:text-2xl">{props.title}</h2>
        {props.desc && <p className="mt-3 text-lg opacity-60">{props.desc}</p>}
        <ArrowIcon className="mt-4" />
      </Link>
    </div>
  )
}

interface IndexProps {
  documents: Document[]
  searchIndex: SearchIndex
}

export default function Index({ documents, searchIndex }: IndexProps) {
  return (
    <Layout searchIndex={searchIndex}>
      <SEO title="Conky" description="Conky documentation" />
      <main className="w-full">
        <div className="w-full">
          {pages.map((p) => (
            <IndexItem
              key={p.slug}
              href={p.slug}
              title={p.title}
              desc={p.desc}
            />
          ))}
          {documents.map((document) => (
            <IndexItem
              key={document.filePath}
              as={`/documents/${document.filePath.replace(/\.mdx?$/, '')}`}
              href={`/documents/[slug]`}
              title={document.data.title ?? 'Untitled'}
              desc={document.data.description}
            />
          ))}
          <IndexItem
            href="https://github.com/brndnmtthws/conky/wiki"
            title="Wiki"
            desc="The Wiki (hosted on GitHub) contains a number of user configs, Lua scripts, FAQs and more."
          />
        </div>
      </main>
    </Layout>
  )
}

export function getStaticProps() {
  const documents = getDocuments()
  const searchIndex = getSearchIndex()

  return { props: { documents, searchIndex } }
}
