import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getLua, Documentation } from '../utils/doc-utils'
import Docs from '../components/Docs'

export interface LuaProps {
  lua: Documentation
}

export default function Lua(props: LuaProps) {
  return (
    <Layout>
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

  return { props: { lua } }
}
