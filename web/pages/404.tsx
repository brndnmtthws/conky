import Layout from '../components/Layout'
import SEO from '../components/SEO'
import Doggy from '../components/Doggy'

export default function FourOhFour() {
  return (
    <Layout>
      <SEO title="Conky" description="Conky documentation" />
      <main className="w-full">
        <div className="flex flex-col items-center">
          <h1 className="text-4xl py-8 font-extrabold">404: Not found 😢</h1>
          <Doggy />
          <h2 className="text-3xl py-4 font-serif">Here&rsquo;s a doggy 😀</h2>
        </div>
      </main>
    </Layout>
  )
}

export function getStaticProps() {
  return { props: {} }
}
