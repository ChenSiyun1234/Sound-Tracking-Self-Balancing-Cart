import { useEffect, useState, type FormEvent } from 'react'
import type { PidGains } from '../types'

const FIELDS: { key: keyof PidGains; label: string; group: string }[] = [
  { key: 'angle_kp', label: 'Kp', group: 'Angle (PD)' },
  { key: 'angle_kd', label: 'Kd', group: 'Angle (PD)' },
  { key: 'vel_kp', label: 'Kp', group: 'Velocity (PI)' },
  { key: 'vel_ki', label: 'Ki', group: 'Velocity (PI)' },
  { key: 'heading_kp', label: 'Kp', group: 'Heading (PD)' },
  { key: 'heading_kd', label: 'Kd', group: 'Heading (PD)' },
]

interface PidFormProps {
  gains: PidGains | null
  onApply: (next: PidGains) => Promise<void>
}

export default function PidForm({ gains, onApply }: PidFormProps) {
  const [draft, setDraft] = useState<PidGains | null>(gains)
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)

  useEffect(() => setDraft(gains), [gains])

  if (!draft) return <p className="muted">Loading gains…</p>

  const groups = [...new Set(FIELDS.map((f) => f.group))]

  async function submit(event: FormEvent) {
    event.preventDefault()
    if (!draft) return
    setBusy(true)
    setError(null)
    try {
      await onApply(draft)
    } catch (err) {
      setError(err instanceof Error ? err.message : 'failed')
    } finally {
      setBusy(false)
    }
  }

  return (
    <form className="pid-form" onSubmit={submit}>
      {groups.map((group) => (
        <fieldset key={group}>
          <legend>{group}</legend>
          {FIELDS.filter((f) => f.group === group).map((f) => (
            <label key={f.key}>
              {f.label}
              <input
                type="number"
                step="0.001"
                value={draft[f.key]}
                onChange={(e) => setDraft({ ...draft, [f.key]: Number(e.target.value) })}
              />
            </label>
          ))}
        </fieldset>
      ))}
      <div className="actions">
        <button type="submit" disabled={busy}>
          {busy ? 'Applying…' : 'Apply gains'}
        </button>
        {error && <span className="error">{error}</span>}
      </div>
    </form>
  )
}
