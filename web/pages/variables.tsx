import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getVariables, Documentation, filterDesc } from '../utils/doc-utils'
import Docs from '../components/Docs'
import { getSearchIndex, SearchIndex } from '../utils/search'

export interface VariablesProps {
  variables: Documentation
  searchIndex: SearchIndex
}

export default function Variables(props: VariablesProps) {
  return (
    <Layout searchIndex={props.searchIndex}>
      <SEO
        title="Conky – Variables"
        description="Conky object variables documentation"
      />
      <main className="w-full">
        <h1 className="text-2xl" data-cy="page-heading">
          Variables
        </h1>
        <Docs docs={props.variables} braces={true} assign={false} />
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const variables = filterDesc(getVariables())
  const searchIndex = getSearchIndex()

  return { props: { variables, searchIndex } }
}
