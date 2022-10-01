import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getLua, Documentation } from '../utils/doc-utils'
import Docs from '../components/Docs'
import { getSearchIndex, SearchIndex } from '../utils/search'

export interface LuaProps {
  lua: Documentation
  searchIndex: SearchIndex
}

export default function Lua(props: LuaProps) {
  return (
    <Layout searchIndex={props.searchIndex}>
      <SEO title="Conky – Lua API" description="Conky Lua API documentation" />
      <main className="w-full">
        <div>
          <h1 className="text-2xl">Lua API</h1>
        </div>
        <Docs docs={props.lua} braces={false} assign={false} />
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const lua = getLua()
  const searchIndex = getSearchIndex()

  return { props: { lua, searchIndex } }
}
