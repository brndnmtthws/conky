import fs from 'fs'
import path from 'path'
import matter from 'gray-matter'
import { unified } from 'unified'
import remarkGfm from 'remark-gfm'
import remarkParse from 'remark-parse'
import remarkRehype from 'remark-rehype'
import rehypeStringify from 'rehype-stringify'

// DOCUMENTS_PATH is useful when you want to get the path to a specific file
export const DOCUMENTS_PATH = path.join(process.cwd(), 'documents')

// documentFilePaths is the list of all mdx files inside the DOCUMENTS_PATH directory
export const documentFilePaths = fs
  .readdirSync(DOCUMENTS_PATH)
  // Only include md(x) files
  .filter((path) => /\.mdx?$/.test(path))

export interface DocumentData {
  title?: string
  description?: string
  indexWeight?: number
}

export interface Document {
  content: string
  data: DocumentData
  filePath: string
}

export const getDocuments = (): Document[] => {
  const documents = documentFilePaths.map((filePath) => {
    const source = fs.readFileSync(path.join(DOCUMENTS_PATH, filePath), 'utf-8')
    const { content, data } = matter(source)

    return {
      content,
      data,
      filePath,
    }
  })

  documents.sort((a, b) => {
    if (a.data.indexWeight > b.data.indexWeight) {
      return 1
    }
    if (a.data.indexWeight < b.data.indexWeight) {
      return -1
    }
    return a.data.title.localeCompare(b.data.title)
  })

  return documents
}

export const getDocumentBySlug = async (slug: string) => {
  const documentFilePath = path.join(DOCUMENTS_PATH, `${slug}.mdx`)
  const source = fs.readFileSync(documentFilePath, 'utf-8')

  const { content, data } = matter(source)

  const result = await unified()
    .use(remarkParse)
    .use(remarkGfm)
    .use(remarkRehype, {allowDangerousHtml: true})
    // .use(rehypePrism)
    .use(rehypeStringify, {allowDangerousHtml: true})
    .process(content)

  return { source: result.value, data, documentFilePath }
}

export const getNextDocumentBySlug = (slug: string) => {
  const documents = getDocuments()
  const currentFileName = `${slug}.mdx`
  const currentDocument = documents.find(
    (document) => document.filePath === currentFileName
  )
  if (!currentDocument) return null
  const currentDocumentIndex = documents.indexOf(currentDocument)

  const document = documents[currentDocumentIndex - 1]
  // no prev document found
  if (!document) return null

  const nextDocumentSlug = document?.filePath.replace(/\.mdx?$/, '')

  return {
    title: document.data.title,
    slug: nextDocumentSlug,
  }
}

export const getPreviousDocumentBySlug = (slug: string) => {
  const documents = getDocuments()
  const currentFileName = `${slug}.mdx`
  const currentDocument = documents.find(
    (document) => document.filePath === currentFileName
  )
  if (!currentDocument) return null
  const currentDocumentIndex = documents.indexOf(currentDocument)

  const document = documents[currentDocumentIndex + 1]
  // no prev document found
  if (!document) return null

  const previousDocumentSlug = document?.filePath.replace(/\.mdx?$/, '')

  return {
    title: document.data.title,
    slug: previousDocumentSlug,
  }
}
