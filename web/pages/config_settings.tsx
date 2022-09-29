import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { ConfigSetting, getConfigSettings } from '../utils/doc-utils'
import { Link } from 'react-feather'

export interface ConfigSettingsProps {
  config_settings: ConfigSetting[]
}

export default function ConfigSettings(props: ConfigSettingsProps) {
  return (
    <Layout>
      <SEO
        title="Conky – Config settings"
        description="Conky configuration settings"
      />
      <main className="w-full">
        <div>
          <h1 className="text-2xl">Configuration settings</h1>
          <p></p>
        </div>
        <div>
          {props.config_settings.map((cs) => (
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
  const config_settings = getConfigSettings()

  return { props: { config_settings } }
}
