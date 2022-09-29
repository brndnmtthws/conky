import 'inter-ui/inter.css'
import '@fontsource/fira-code'
import 'prismjs/themes/prism-tomorrow.css'
import '../styles/globals.css'
import { AppProps } from 'next/app'

function App({ Component, pageProps }: AppProps) {
  return (
    <>
      <Component {...pageProps} />
    </>
  )
}

export default App
