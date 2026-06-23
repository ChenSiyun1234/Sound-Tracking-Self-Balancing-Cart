export interface Telemetry {
  ts: number
  pitch_deg: number
  velocity: number
  heading_deg: number
  pid_output: number
  upright: boolean
  source: string
}

export interface PidGains {
  angle_kp: number
  angle_kd: number
  vel_kp: number
  vel_ki: number
  heading_kp: number
  heading_kd: number
}
