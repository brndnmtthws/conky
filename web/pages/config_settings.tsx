import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { Doc, getConfigSettings } from '../utils/doc-utils'
import Docs from '../components/Docs'

export interface ConfigSettingsProps {
  config_settings: Doc[]
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
        </div>
        <Docs docs={props.config_settings} />
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const config_settings = getConfigSettings()

  return { props: { config_settings } }
}
