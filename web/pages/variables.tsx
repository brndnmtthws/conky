import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getVariables, Variable } from '../utils/doc-utils'
import { Link } from 'react-feather'

export interface VariablesProps {
  variables: Variable[]
}

export default function Variables(props: VariablesProps) {
  return (
    <Layout>
      <SEO
        title="Conky – Variables"
        description="Conky object variables documentation"
      />
      <main className="w-full">
        <div>
          <h1 className="text-2xl">Variables</h1>
        </div>
        <div>
          {props.variables.map((cs) => (
            <div
              id={cs.name}
              key={cs.name}
              className="scroll-mt-16 target:bg-rose-300 target:dark:bg-rose-900 hover:bg-slate-100 dark:hover:bg-slate-800 my-4"
            >
              <div className="flex">
                <div className="p-2">
                  <a href={`#${cs.name}`}>
                    <Link size={16} />
                  </a>
                </div>
                <div className="flex-col p-1">
                  <code className="text-lg p-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
                    {cs.name}
                  </code>
                  <div
                    className="py-2"
                    dangerouslySetInnerHTML={{ __html: cs.desc }}
                  />
                  <div className="">
                    Default:{' '}
                    {cs.default ? <code>{cs.default}</code> : <em>n/a</em>}
                  </div>
                </div>
              </div>
            </div>
          ))}
        </div>
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const variables = getVariables()

  return { props: { variables } }
}
