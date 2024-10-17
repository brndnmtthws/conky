import Layout from '../components/Layout'
import SEO from '../components/SEO'
import Doggy from '../components/Doggy'

export default function FourOhFour() {
  return (
    <Layout>
      <SEO title="Conky" description="Conky documentation" />
      <main className="w-full">
        <div className="flex flex-col items-center">
          <h1 className="text-4xl py-8 px-2 lg:px-4 font-extrabold text-center">404: Not found 😢</h1>
          <Doggy />
          <h2 className="text-3xl py-4 font-serif mb-8">Here&rsquo;s a doggy 😀</h2>
        </div>
      </main>
    </Layout>
  )
}

export function getStaticProps() {
  return { props: {} }
}
