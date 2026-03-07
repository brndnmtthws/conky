import { ArrowRight } from 'lucide-react'
import Link from 'next/link'
import { getDocuments } from '../utils/mdx-utils'
import type { Document } from '../utils/mdx-utils'
import Layout from '../components/Layout'
import SEO from '../components/SEO'

const pages = [
  {
    slug: '/variables',
    title: 'Variables',
    desc: 'Variables let you define the various objects displayed in Conky including text, bars, graphs, and more.',
  },
  {
    slug: '/config_settings',
    title: 'Configuration settings',
    desc: 'Global configuration parameters let you customize how Conky behaves.',
  },
  {
    slug: '/lua',
    title: 'Lua API',
    desc: 'Extend Conky with custom behavior by using the Lua API.',
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
    <div className="backdrop-blur-sm transition first:rounded-t-2xl last:rounded-b-2xl border border-zinc-300/80 bg-white/70 hover:bg-white dark:border-white/10 dark:bg-white/5 dark:hover:bg-white/10 border-b-0 last:border-b hovered-sibling:border-t-0">
      <Link
        as={props.as}
        href={props.href}
        className="block px-4 py-4 focus:outline-none focus:ring-4 focus:ring-zinc-400/40 lg:px-5 lg:py-5"
      >
        <h2 className="text-xl md:text-[2rem] md:leading-tight">{props.title}</h2>
        {props.desc && (
          <p className="mt-4 max-w-3xl text-lg leading-8 text-zinc-600 dark:text-zinc-400">
            {props.desc}
          </p>
        )}
        <ArrowRight
          className="mt-5 text-zinc-900 dark:text-zinc-100"
          size={24}
          strokeWidth={2}
        />
      </Link>
    </div>
  )
}

interface IndexProps {
  documents: Document[]
}

export default function Index({ documents }: IndexProps) {
  return (
    <Layout>
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
            desc="The wiki on GitHub contains user configs, Lua scripts, FAQs, and more."
          />
        </div>
      </main>
    </Layout>
  )
}

export function getStaticProps() {
  const documents = getDocuments()

  return { props: { documents } }
}
