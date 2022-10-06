import '@fontsource/fira-code/variable.css'
import '@fontsource/inter/variable.css'
import '@fontsource/newsreader/variable.css'
import 'prismjs/themes/prism-tomorrow.css'
import '../styles/globals.css'
import { AppProps } from 'next/app'

function App({ Component, pageProps }: AppProps) {
  return <Component {...pageProps} />
}

export default App
