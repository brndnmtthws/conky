import Layout from '../components/Layout'
import SEO from '../components/SEO'
import { getConfigSettings, Documentation } from '../utils/doc-utils'
import Docs from '../components/Docs'
import { getSearchIndex, SearchIndex } from '../utils/search'

export interface ConfigSettingsProps {
  config_settings: Documentation
  searchIndex: SearchIndex
}

export default function ConfigSettings(props: ConfigSettingsProps) {
  return (
    <Layout searchIndex={props.searchIndex}>
      <SEO
        title="Conky – Config settings"
        description="Conky configuration settings"
      />
      <main className="w-full">
        <div>
          <h1 className="text-2xl">Configuration settings</h1>
        </div>
        <Docs docs={props.config_settings} braces={false} assign={true} />
      </main>
    </Layout>
  )
}

export async function getStaticProps() {
  const config_settings = getConfigSettings()
  const searchIndex = getSearchIndex()

  return { props: { config_settings, searchIndex } }
}
