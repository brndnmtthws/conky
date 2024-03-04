import {
  getNextDocumentBySlug,
  getDocumentBySlug,
  getPreviousDocumentBySlug,
  documentFilePaths,
} from '../../utils/mdx-utils'

import Layout from '../../components/Layout'
import SEO from '../../components/SEO'
import { GetStaticProps } from 'next'
import Link from 'next/link'
import { unified } from 'unified'
import rehypeReact from 'rehype-react'
import * as prod from 'react/jsx-runtime'
import rehypeParse from 'rehype-parse'
import { createElement, Fragment, useEffect, useState } from 'react'

const production = { Fragment: prod.Fragment, jsx: prod.jsx, jsxs: prod.jsxs }

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
  const [children, setChildren] = useState(createElement(Fragment))

  useEffect(
    function () {
      ;(async function () {
        const file = await unified()
          .use(rehypeParse, { fragment: true })
          // @ts-expect-error: the react types are missing.
          .use(rehypeReact, { ...production, components: { a: Link } })
          .process(source)

        setChildren(file.result)
      })()
    },
    [source]
  )

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
          <article className="prose dark:prose-invert">{children}</article>
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
