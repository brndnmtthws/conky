import Link from 'next/link'
import { Url } from 'url'

interface CustomLinkProps {
  as: Url
  href: Url
}

export default function CustomLink({
  as,
  href,
  ...otherProps
}: CustomLinkProps) {
  return (
    <>
      <Link as={as} href={href}>
        <a {...otherProps} />
      </Link>
    </>
  )
}
