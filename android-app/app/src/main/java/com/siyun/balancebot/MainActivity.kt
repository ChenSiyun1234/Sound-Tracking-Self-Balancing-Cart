package com.siyun.balancebot

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothSocket
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.view.MotionEvent
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.io.OutputStream
import java.util.UUID

/**
 * BalanceBot Remote (Android) — drives the self-balancing robot over its HC-05
 * Bluetooth link using the Serial Port Profile (SPP). Sends single-char commands
 * (F/B/L/R/S) plus a speed digit; press-and-hold to move, release to stop.
 */
class MainActivity : AppCompatActivity() {

    // Standard SPP UUID used by HC-05 / classic-Bluetooth serial modules.
    private val sppUuid: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
    private val hc05Name = "HC-05"               // change if your module is renamed

    private var socket: BluetoothSocket? = null
    private var output: OutputStream? = null
    private var speed = 5

    private lateinit var status: TextView

    private val requestPerms = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { granted ->
        if (granted.values.all { it }) connect() else status.text = "Bluetooth permission needed"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        status = findViewById(R.id.status)

        findViewById<Button>(R.id.btnConnect).setOnClickListener { ensurePermsThenConnect() }

        bindHold(R.id.btnFwd, 'F')
        bindHold(R.id.btnBack, 'B')
        bindHold(R.id.btnLeft, 'L')
        bindHold(R.id.btnRight, 'R')
        findViewById<Button>(R.id.btnStop).setOnClickListener { send('S') }

        findViewById<SeekBar>(R.id.seekSpeed).setOnSeekBarChangeListener(
            object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(sb: SeekBar?, p: Int, fromUser: Boolean) { speed = p }
                override fun onStartTrackingTouch(sb: SeekBar?) {}
                override fun onStopTrackingTouch(sb: SeekBar?) {}
            })
    }

    /** Press-and-hold: send the direction on touch-down, send STOP on release. */
    @SuppressLint("ClickableViewAccessibility")
    private fun bindHold(viewId: Int, cmd: Char) {
        findViewById<Button>(viewId).setOnTouchListener { _, e ->
            when (e.action) {
                MotionEvent.ACTION_DOWN -> { send(cmd); true }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> { send('S'); true }
                else -> false
            }
        }
    }

    private fun neededPerms(): Array<String> =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            arrayOf(Manifest.permission.BLUETOOTH_CONNECT)
        else
            arrayOf(Manifest.permission.BLUETOOTH, Manifest.permission.BLUETOOTH_ADMIN)

    private fun hasPerms(): Boolean = neededPerms().all {
        ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED
    }

    private fun ensurePermsThenConnect() {
        if (hasPerms()) connect() else requestPerms.launch(neededPerms())
    }

    @SuppressLint("MissingPermission")
    private fun connect() {
        val manager = getSystemService(BLUETOOTH_SERVICE) as BluetoothManager
        val adapter: BluetoothAdapter? = manager.adapter
        if (adapter == null || !adapter.isEnabled) {
            status.text = "Turn on Bluetooth and pair the HC-05 first"; return
        }
        val device: BluetoothDevice? = adapter.bondedDevices.firstOrNull {
            (it.name ?: "").contains(hc05Name, ignoreCase = true)
        }
        if (device == null) { status.text = "HC-05 not found in paired devices"; return }

        status.text = "Connecting to ${device.name}…"
        Thread {
            try {
                val s = device.createRfcommSocketToServiceRecord(sppUuid)
                adapter.cancelDiscovery()          // discovery slows down a connect
                s.connect()                        // blocking call -> off the UI thread
                socket = s
                output = s.outputStream
                runOnUiThread { status.text = "Connected to ${device.name}" }
            } catch (ex: Exception) {
                runOnUiThread { status.text = "Connect failed: ${ex.message}" }
            }
        }.start()
    }

    private fun send(cmd: Char) {
        val o = output ?: run { status.text = "Not connected"; return }
        val payload = if (cmd == 'S') "S" else "$cmd$speed"
        Thread {
            try {
                o.write(payload.toByteArray(Charsets.US_ASCII))
            } catch (ex: Exception) {
                runOnUiThread { status.text = "Send failed: ${ex.message}" }
            }
        }.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        try { output?.write("S".toByteArray()); socket?.close() } catch (_: Exception) {}
    }
}
