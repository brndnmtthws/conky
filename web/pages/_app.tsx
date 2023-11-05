import '@fontsource-variable/fira-code'
import '@fontsource-variable/inter'
import '@fontsource-variable/newsreader'
import 'prismjs/themes/prism-tomorrow.css'
import '../styles/globals.css'
import { AppProps } from 'next/app'

function App({ Component, pageProps }: AppProps) {
  return <Component {...pageProps} />
}

export default App
