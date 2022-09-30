import '@fontsource/fira-code/400.css'
import '@fontsource/inter/400.css'
import '@fontsource/source-serif-pro/400.css'
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
