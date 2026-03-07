import {
  getNextDocumentBySlug,
  getDocumentBySlug,
  getPreviousDocumentBySlug,
  documentFilePaths,
} from '../../utils/mdx-utils'

import Layout from '../../components/Layout'
import SEO from '../../components/SEO'
import type { GetStaticProps } from 'next'

interface FrontMatter {
  title: string
  description: string
}

interface DocumentPageProps {
  source: string
  frontMatter: FrontMatter
}

export default function DocumentPage({
  source,
  frontMatter,
}: DocumentPageProps) {
  return (
    <Layout>
      <SEO
        title={frontMatter.title}
        description={frontMatter.description}
      />
      <article className="p-2 pt-4 lg:p-4 w-full">
        <header>
          <h1 className="text-3xl md:text-5xl dark:text-white mb-12">
            {frontMatter.title}
          </h1>
          {frontMatter.description && (
            <p className="text-xl mb-4">{frontMatter.description}</p>
          )}
        </header>
        <main>
          <article
            className="prose dark:prose-invert"
            dangerouslySetInnerHTML={{ __html: source }}
          />
        </main>
      </article>
    </Layout>
  )
}

export const getStaticProps: GetStaticProps = async ({ params }) => {
  if (params) {
    const { source, data } = await getDocumentBySlug(params.slug as string)
    const prevDocument = getPreviousDocumentBySlug(params.slug as string)
    const nextDocument = getNextDocumentBySlug(params.slug as string)

    return {
      props: {
        source,
        frontMatter: data,
        prevDocument,
        nextDocument,
      },
    }
  }
  return { props: {} }
}

export const getStaticPaths = async () => {
  const paths = documentFilePaths
    // Remove file extensions for page paths
    .map((path) => path.replace(/\.mdx?$/, ''))
    // Map the path into the static paths object required by Next.js
    .map((slug) => ({ params: { slug } }))

  return {
    paths,
    fallback: false,
  }
}
