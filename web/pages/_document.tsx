import React from 'react'
import Document, { Html, Head, Main, NextScript } from 'next/document'

const themeScript = `
(() => {
  const theme = window.localStorage.getItem('theme');
  const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
  if (theme === 'dark' || (!theme && prefersDark)) {
    document.documentElement.classList.add('dark');
  }
})();
`

class MyDocument extends Document {
  render() {
    return (
      <Html lang="en" className="theme-compiled scroll-smooth" suppressHydrationWarning>
        <Head>
          <link rel="icon" href="/favicon.svg" type="image/svg+xml" />
          <script dangerouslySetInnerHTML={{ __html: themeScript }} />
        </Head>
        <body className="antialiased leading-base">
          <Main />
          <NextScript />
        </body>
      </Html>
    )
  }
}

export default MyDocument
