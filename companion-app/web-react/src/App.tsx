import { useEffect, useRef, useState } from 'react'
import type { PidGains, Telemetry } from './types'
import { getGains, setGains } from './api'
import Sparkline from './components/Sparkline'
import PidForm from './components/PidForm'

const HISTORY = 160

export default function App() {
  const [latest, setLatest] = useState<Telemetry | null>(null)
  const [history, setHistory] = useState<number[]>([])
  const [gains, setGainsState] = useState<PidGains | null>(null)
  const [connected, setConnected] = useState(false)
  const esRef = useRef<EventSource | null>(null)

  useEffect(() => {
    getGains().then(setGainsState).catch(() => undefined)
    const es = new EventSource('/api/stream')
    esRef.current = es
    es.onopen = () => setConnected(true)
    es.onerror = () => setConnected(false)
    es.onmessage = (ev) => {
      const t = JSON.parse(ev.data) as Telemetry
      setConnected(true)
      setLatest(t)
      setHistory((h) => [...h.slice(-(HISTORY - 1)), t.pitch_deg])
    }
    return () => es.close()
  }, [])

  async function applyGains(next: PidGains) {
    const saved = await setGains(next)
    setGainsState(saved)
  }

  return (
    <main className="app">
      <header>
        <h1>BalanceBot — Live Dashboard</h1>
        <span className={connected ? 'pill ok' : 'pill off'}>
          {connected ? 'streaming' : 'disconnected'}
        </span>
      </header>

      <section className="status">
        <Stat label="Pitch" value={latest ? `${latest.pitch_deg.toFixed(1)}°` : '—'} />
        <Stat label="PID out" value={latest ? latest.pid_output.toFixed(1) : '—'} />
        <Stat label="Heading" value={latest ? `${Math.round(latest.heading_deg)}°` : '—'} />
        <Stat
          label="State"
          value={latest ? (latest.upright ? 'upright' : 'fallen') : '—'}
          tone={latest ? (latest.upright ? 'ok' : 'bad') : undefined}
        />
        <Stat label="Source" value={latest ? latest.source : '—'} />
      </section>

      <section className="chart-wrap">
        <Sparkline values={history} />
      </section>

      <PidForm gains={gains} onApply={applyGains} />
    </main>
  )
}

function Stat({
  label,
  value,
  tone,
}: {
  label: string
  value: string
  tone?: 'ok' | 'bad'
}) {
  return (
    <div className={`stat ${tone ?? ''}`}>
      <span className="stat-label">{label}</span>
      <span className="stat-value">{value}</span>
    </div>
  )
}
