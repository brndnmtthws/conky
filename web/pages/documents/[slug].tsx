import {
  getNextDocumentBySlug,
  getDocumentBySlug,
  getPreviousDocumentBySlug,
  documentFilePaths,
} from '../../utils/mdx-utils'

import { MDXRemote, MDXRemoteProps } from 'next-mdx-remote'
import Head from 'next/head'
import CustomLink from '../../components/CustomLink'
import Layout from '../../components/Layout'
import SEO from '../../components/SEO'
import { GetStaticProps } from 'next'

// Custom components/renderers to pass to MDX.
// Since the MDX files aren't loaded by webpack, they have no knowledge of how
// to handle import statements. Instead, you must include components in scope
// here.
const components = {
  a: CustomLink,
  // It also works with dynamically-imported components, which is especially
  // useful for conditionally loading components for certain routes.
  // See the notes in README.md for more details.
  Head,
}

interface FrontMatter {
  title: string
  description: string
}

interface DocumentPageProps {
  source: MDXRemoteProps
  frontMatter: FrontMatter
}

export default function DocumentPage({
  source,
  frontMatter,
}: DocumentPageProps) {
  return (
    <Layout>
      <SEO
        title={`${frontMatter.title}`}
        description={frontMatter.description}
      />
      <article className="px-6 md:px-0">
        <header>
          <h1 className="text-3xl md:text-5xl dark:text-white mb-12">
            {frontMatter.title}
          </h1>
          {frontMatter.description && (
            <p className="text-xl mb-4">{frontMatter.description}</p>
          )}
        </header>
        <main>
          <article className="prose dark:prose-invert prose-lg lg:prose-xl">
            <MDXRemote {...source} components={components as any} />
          </article>
        </main>
      </article>
    </Layout>
  )
}

export const getStaticProps: GetStaticProps = async ({ params }) => {
  if (params) {
    const { mdxSource, data } = await getDocumentBySlug(params.slug as string)
    const prevDocument = getPreviousDocumentBySlug(params.slug as string)
    const nextDocument = getNextDocumentBySlug(params.slug as string)

    return {
      props: {
        source: mdxSource,
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
