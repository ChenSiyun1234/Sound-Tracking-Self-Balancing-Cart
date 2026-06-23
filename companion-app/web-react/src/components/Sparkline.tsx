interface SparklineProps {
  values: number[]
  width?: number
  height?: number
  min?: number
  max?: number
}

/** A tiny dependency-free line chart drawn as inline SVG. */
export default function Sparkline({
  values,
  width = 800,
  height = 180,
  min = -30,
  max = 30,
}: SparklineProps) {
  if (values.length < 2) {
    return <svg className="spark" viewBox={`0 0 ${width} ${height}`} preserveAspectRatio="none" />
  }
  const span = max - min || 1
  const stepX = width / (values.length - 1)
  const points = values
    .map((v, i) => {
      const clamped = Math.max(min, Math.min(max, v))
      const y = height - ((clamped - min) / span) * height
      return `${(i * stepX).toFixed(1)},${y.toFixed(1)}`
    })
    .join(' ')
  const zeroY = height - ((0 - min) / span) * height

  return (
    <svg
      className="spark"
      viewBox={`0 0 ${width} ${height}`}
      preserveAspectRatio="none"
      role="img"
      aria-label="Pitch angle over time"
    >
      <line x1={0} y1={zeroY} x2={width} y2={zeroY} className="spark-axis" />
      <polyline points={points} className="spark-line" fill="none" />
    </svg>
  )
}
