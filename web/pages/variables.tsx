import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getVariables, Documentation, filterDesc } from '../utils/doc-utils'
import Docs from '../components/Docs'

export interface VariablesProps {
  variables: Documentation
}

export default function Variables(props: VariablesProps) {
  return (
    <Layout>
      <SEO
        title="Conky – Variables"
        description="Conky object variables documentation"
      />
      <main className="w-full pt-4">
        <h1 className="text-2xl px-2 lg:px-4" data-cy="page-heading">
          Variables
        </h1>
        <Docs docs={props.variables} braces={true} assign={false} />
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const variables = filterDesc(getVariables())

  return { props: { variables } }
}
