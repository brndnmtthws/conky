import { Doc } from '../utils/doc-utils'
import { Link } from 'react-feather'

export interface DocsProps {
  docs: Doc[]
}

export default function Docs({ docs }: DocsProps) {
  return (
    <>
      {docs.map((doc) => (
        <div
          id={doc.name}
          key={doc.name}
          className="scroll-mt-16 target:bg-rose-300 target:dark:bg-rose-900 hover:bg-slate-100 dark:hover:bg-slate-800 my-4"
        >
          <div className="flex">
            <div className="p-2">
              <a href={`#${doc.name}`}>
                <Link size={16} />
              </a>
            </div>
            <div className="flex-col p-1">
              <code className="text-lg p-1 bg-fuchsia-200 dark:bg-fuchsia-900 font-bold">
                {doc.name}
              </code>
              <div
                className="py-2"
                dangerouslySetInnerHTML={{ __html: doc.desc }}
              />
              {doc.default && (
                <div>
                  Default: <code>{doc.default}</code>
                </div>
              )}
            </div>
          </div>
        </div>
      ))}
    </>
  )
}
