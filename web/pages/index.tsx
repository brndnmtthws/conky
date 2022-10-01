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
            <div
              key={p.slug}
              className="md:first:rounded-t-lg md:last:rounded-b-lg backdrop-blur-lg bg-slate-300 dark:bg-black dark:bg-opacity-30 bg-opacity-10 hover:bg-opacity-30 dark:hover:bg-opacity-50 transition border border-gray-800 dark:border-white border-opacity-10 dark:border-opacity-10 border-b-0 last:border-b hover:border-b hovered-sibling:border-t-0"
            >
              <Link as={p.slug} href={p.slug}>
                <a className="py-6 lg:py-10 px-6 lg:px-16 block focus:outline-none focus:ring-4">
                  <h2 className="text-2xl md:text-3xl">{p.title}</h2>
                  <p className="mt-3 text-lg opacity-60">{p.desc}</p>
                  <ArrowIcon className="mt-4" />
                </a>
              </Link>
            </div>
          ))}
          {documents.map((document) => (
            <div
              key={document.filePath}
              className="md:first:rounded-t-lg md:last:rounded-b-lg backdrop-blur-lg bg-slate-300 dark:bg-black dark:bg-opacity-30 bg-opacity-10 hover:bg-opacity-30 dark:hover:bg-opacity-50 transition border border-gray-800 dark:border-white border-opacity-10 dark:border-opacity-10 border-b-0 last:border-b hover:border-b hovered-sibling:border-t-0"
            >
              <Link
                as={`/documents/${document.filePath.replace(/\.mdx?$/, '')}`}
                href={`/documents/[slug]`}
              >
                <a className="py-6 lg:py-10 px-6 lg:px-16 block focus:outline-none focus:ring-4">
                  <h2 className="text-2xl md:text-3xl">
                    {document.data.title}
                  </h2>
                  {document.data.description && (
                    <p className="mt-3 text-lg opacity-60">
                      {document.data.description}
                    </p>
                  )}
                  <ArrowIcon className="mt-4" />
                </a>
              </Link>
            </div>
          ))}
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
