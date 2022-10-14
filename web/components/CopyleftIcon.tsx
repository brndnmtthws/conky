interface CopyleftIconProps {
  className?: string
  width?: number
  height?: number
}

export default function CopyleftIcon({
  className,
  width,
  height,
}: CopyleftIconProps) {
  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      width={width || 980}
      height={height || 980}
      fill="none"
      viewBox="0 0 980 980"
      className={className}
    >
      <circle
        cx="490"
        cy="490"
        r="440"
        fill="none"
        strokeWidth="100"
        className="stroke-current"
      />
      <path
        d="M219,428H350a150,150 0 1 1 0,125H219a275,275 0 1 0 0-125z"
        strokeWidth="1"
        className="stroke-current fill-current"
      />
    </svg>
  )
}
