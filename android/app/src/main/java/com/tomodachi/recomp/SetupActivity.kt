package com.tomodachi.recomp

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.io.File

/**
 * First-launch setup activity.
 *
 * Guides the user through:
 *  1. Granting storage permission (MANAGE_EXTERNAL_STORAGE on API 30+).
 *  2. Picking their game NSP file.
 *  3. Picking their prod.keys file.
 *  4. Copying both to internal app storage via the native installer.
 *  5. Launching GameActivity when the install step has finished.
 *
 * On subsequent launches, if the install marker exists in internal storage,
 * SetupActivity goes directly to GameActivity without prompting again.
 */
class SetupActivity : AppCompatActivity() {

    private val installMarker: File
        get() = File(filesDir, "install.ok")

    private var pendingNspUri: Uri? = null

    // ── Storage permission ──────────────────────────────────────────────────────

    private val requestStorageLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            checkStoragePermissionAndContinue()
        }

    private val requestLegacyStorage =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
            if (granted) startSetupFlow()
            else showPermissionRequiredDialog()
        }

    // ── File pickers ────────────────────────────────────────────────────────────

    private val nspPicker =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            if (uri == null) return@registerForActivityResult
            pendingNspUri = uri
            pickKeys()
        }

    private val keysPicker =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            if (uri == null) return@registerForActivityResult
            val nspUri = pendingNspUri ?: return@registerForActivityResult
            runInstall(nspUri, uri)
        }

    // ── Lifecycle ───────────────────────────────────────────────────────────────

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (installMarker.exists()) {
            launchGame()
            return
        }

        setContentView(R.layout.activity_setup)
        checkStoragePermissionAndContinue()
    }

    // ── Permission flow ─────────────────────────────────────────────────────────

    private fun checkStoragePermissionAndContinue() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (Environment.isExternalStorageManager()) {
                startSetupFlow()
            } else {
                AlertDialog.Builder(this)
                    .setTitle(R.string.permission_storage_title)
                    .setMessage(R.string.permission_storage_message)
                    .setPositiveButton(R.string.grant) { _, _ ->
                        requestStorageLauncher.launch(
                            Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                                Uri.parse("package:$packageName"))
                        )
                    }
                    .setNegativeButton(R.string.cancel) { _, _ -> finish() }
                    .show()
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
                startSetupFlow()
            } else {
                requestLegacyStorage.launch(Manifest.permission.READ_EXTERNAL_STORAGE)
            }
        }
    }

    private fun showPermissionRequiredDialog() {
        AlertDialog.Builder(this)
            .setTitle(R.string.permission_required_title)
            .setMessage(R.string.permission_required_message)
            .setPositiveButton(android.R.string.ok) { _, _ -> finish() }
            .show()
    }

    // ── Setup flow ──────────────────────────────────────────────────────────────

    private fun startSetupFlow() {
        AlertDialog.Builder(this)
            .setTitle(R.string.setup_title)
            .setMessage(R.string.setup_message)
            .setPositiveButton(R.string.pick_nsp) { _, _ ->
                nspPicker.launch("*/*")
            }
            .setNegativeButton(R.string.cancel) { _, _ -> finish() }
            .show()
    }

    private fun pickKeys() {
        AlertDialog.Builder(this)
            .setTitle(R.string.pick_keys_title)
            .setMessage(R.string.pick_keys_message)
            .setPositiveButton(R.string.pick_keys) { _, _ ->
                keysPicker.launch("*/*")
            }
            .setNegativeButton(R.string.cancel) { _, _ -> finish() }
            .show()
    }

    private fun runInstall(nspUri: Uri, keysUri: Uri) {
        Toast.makeText(this, R.string.installing, Toast.LENGTH_LONG).show()

        // Copy files to internal storage before passing to native installer.
        val nspDest = File(filesDir, "game.nsp")
        val keysDest = File(filesDir, "keys/prod.keys")
        keysDest.parentFile?.mkdirs()

        contentResolver.openInputStream(nspUri)?.use { src ->
            nspDest.outputStream().use { dst -> src.copyTo(dst) }
        }
        contentResolver.openInputStream(keysUri)?.use { src ->
            keysDest.outputStream().use { dst -> src.copyTo(dst) }
        }

        // Kick off the native install pipeline in a background thread.
        Thread {
            val rc = TomodachiNative.install(
                nspPath      = nspDest.absolutePath,
                keysDir      = keysDest.parent!!,
                dataDir      = filesDir.absolutePath,
                externalDir  = getExternalFilesDir(null)?.absolutePath ?: filesDir.absolutePath,
            )
            runOnUiThread {
                if (rc == 0) {
                    installMarker.createNewFile()
                    launchGame()
                } else {
                    AlertDialog.Builder(this)
                        .setTitle(R.string.install_failed_title)
                        .setMessage(getString(R.string.install_failed_message, rc))
                        .setPositiveButton(android.R.string.ok, null)
                        .show()
                }
            }
        }.start()
    }

    private fun launchGame() {
        startActivity(Intent(this, GameActivity::class.java))
        finish()
    }
}
