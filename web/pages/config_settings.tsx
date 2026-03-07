import Layout from '../components/Layout'
import SEO from '../components/SEO'
import {
  getConfigSettings,
  filterDesc,
} from '../utils/doc-utils'
import type { Documentation } from '../utils/doc-utils'
import Docs from '../components/Docs'

export interface ConfigSettingsProps {
  config_settings: Documentation
}

export default function ConfigSettings(props: ConfigSettingsProps) {
  return (
    <Layout>
      <SEO
        title="Conky – Config settings"
        description="Conky configuration settings"
      />
      <main className="w-full pt-4">
        <div>
          <h1 className="text-2xl px-2 lg:px-4" data-cy="page-heading">
            Configuration settings
          </h1>
        </div>
        <Docs docs={props.config_settings} braces={false} assign={true} />
      </main>
    </Layout>
  )
}

export function getStaticProps() {
  const config_settings = filterDesc(getConfigSettings())

  return { props: { config_settings } }
}
