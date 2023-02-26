import { useEffect, useMemo, useRef } from 'react'
import * as d3 from 'd3'
import { random } from 'colord'

type DataPoint = { x: number; y: number }
type LineChartProps = {
  width: number
  height: number
  darkMode: boolean
}

function getRandomArbitrary(min: number, max: number): number {
  return Math.random() * (max - min) + min
}

function updateData(width: number, data: Array<DataPoint>): Array<DataPoint> {
  while (data.length >= width) {
    data.shift()
  }
  const prev = data.at(data.length - 1)
  if (prev) {
    data.push({
      x: prev.x + 1,
      y: Math.max(0, prev.y + getRandomArbitrary(-120, 120)),
    })
  } else {
    data.push({
      x: 0,
      y: getRandomArbitrary(0, 100),
    })
  }
  return data
}

function fetchData() {
  const storedData =
    typeof window === 'undefined'
      ? '[]'
      : localStorage.getItem('chartData') || '[]'
  const data = JSON.parse(storedData)
  return data || []
}
const data = fetchData()

export const LineChart = ({ width, height, darkMode }: LineChartProps) => {
  const timerRef = useRef<NodeJS.Timeout>()
  const svgRef = useRef(null)
  const stroke = useMemo(() => random(), [])

  useEffect(() => {
    const doIt = (data: DataPoint[]) => {
      data = updateData(width, data)

      // Y axis
      const [min, max] = d3.extent(data, (d) => d.y)
      const yScale = d3
        .scaleLinear()
        .domain([min || 0, max || 0])
        .range([height, 0])

      // X axis
      const [, xMax] = d3.extent(data, (d) => d.x)
      const xScale = d3
        .scaleLinear()
        .domain([(xMax || 0) - width, xMax || 0])
        .range([0, width])

      if (!svgRef.current) {
        return
      }
      d3.select(svgRef.current)
        .selectAll('rect')
        .data(data)
        .join(
          (enter) =>
            enter
              .append('rect')
              .style(
                'stroke',
                darkMode ? stroke.lighten().toHex() : stroke.darken().toHex()
              )
              .style('stroke-opacity', '0.7')
              .attr('x', (d) => xScale(d.x))
              .attr('y', (d) => yScale(d.y))
              .attr('height', (d) => yScale((max || 0) - d.y))
              .attr('width', () => 1),
          (update) =>
            update
              .transition()
              .duration(1000)
              .style(
                'stroke',
                darkMode ? stroke.lighten().toHex() : stroke.darken().toHex()
              )
              .attr('x', (d) => xScale(d.x))
              .attr('y', (d) => yScale(d.y))
              .attr('height', (d) => yScale((max || 0) - d.y))
              .attr('width', () => 1),
          (exit) => exit.call((d) => d.transition().remove())
        )

      localStorage.setItem('chartData', JSON.stringify(data))

      timerRef.current = setTimeout(() => {
        doIt(data)
      }, 1000)
    }

    doIt(data)

    return () => clearTimeout(timerRef.current)
  }, [darkMode, stroke, height, width])

  return (
    <svg
      width={width}
      height={height}
      viewBox={`0 0 ${width} ${height}`}
      preserveAspectRatio="none"
      ref={svgRef}
      className="fill-transparent bg-transparent grow"
    />
  )
}
