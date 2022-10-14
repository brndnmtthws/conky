import CopyleftIcon from './CopyleftIcon'

const Footer: React.FunctionComponent = () => {
  return (
    <div className="flex py-4 justify-center items-center">
      <div className="px-1">
        <CopyleftIcon width={20} height={20} />
      </div>
      <div className="px-1 font-sans text-sm">
        <p>{new Date().getFullYear()} Conky developers</p>
      </div>
    </div>
  )
}

export default Footer
