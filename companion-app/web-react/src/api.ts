import type { PidGains } from './types'

/** Read the current PID gains from the API. */
export async function getGains(): Promise<PidGains> {
  const res = await fetch('/api/pid')
  if (!res.ok) throw new Error(`GET /api/pid failed: ${res.status}`)
  return res.json() as Promise<PidGains>
}

/** Apply a (possibly partial) gain update; returns the merged gains. */
export async function setGains(partial: Partial<PidGains>): Promise<PidGains> {
  const res = await fetch('/api/pid', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(partial),
  })
  if (!res.ok) {
    const err = (await res.json().catch(() => ({ error: res.statusText }))) as {
      error?: string
    }
    throw new Error(err.error ?? 'request failed')
  }
  return res.json() as Promise<PidGains>
}
