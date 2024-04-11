import getConfig from 'next/config'
import CopyleftIcon from './CopyleftIcon'

const Footer: React.FunctionComponent = () => {
  const { publicRuntimeConfig } = getConfig()
  const { modifiedDate, modifiedYear } = publicRuntimeConfig
  return (
    <div className="flex py-4 justify-center items-center">
      <div className="px-1">
        <CopyleftIcon width={20} height={20} />
      </div>
      <div className="px-1 font-sans text-xs">
        <p>
          {modifiedYear} Conky developers, updated <code>{modifiedDate}</code>
        </p>
      </div>
    </div>
  )
}

export default Footer
