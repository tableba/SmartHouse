package com.example.smarthouse

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.runtime.*
import androidx.lifecycle.lifecycleScope
import com.example.smarthouse.data.ApiClient
import com.example.smarthouse.data.Device
import com.example.smarthouse.data.MockDevices
import com.example.smarthouse.screens.DeviceDetailsScreen
import com.example.smarthouse.screens.LoginScreen
import com.example.smarthouse.screens.RegisterScreen
import com.example.smarthouse.ui.screens.HomeScreen
import com.example.smarthouse.ui.theme.SmartHouseAndroidTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : ComponentActivity() {

    private val preferences by lazy {
        getSharedPreferences("smarthouse", MODE_PRIVATE)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            SmartHouseAndroidTheme {
                SmartHouseApp()
            }
        }
    }

    @Composable
    private fun SmartHouseApp() {

        var devices by remember {
            mutableStateOf<List<Device>>(emptyList())
        }

        var loggedIn by remember {
            mutableStateOf(
                preferences.getString("jwt_token", null) != null
            )
        }

        var loading by remember {
            mutableStateOf(false)
        }

        var errorMessage by remember {
            mutableStateOf("")
        }

        var selectedDevice by remember {
            mutableStateOf<Device?>(null)
        }

        var showRegisterScreen by remember {
            mutableStateOf(false)
        }

        /*
         * LOGIN / REGISTER
         */
        if (!loggedIn) {

            if (showRegisterScreen) {

                RegisterScreen(
                    onRegisterClick = { email, password ->

                        // Backend registration will be connected
                        // when the correct endpoint is confirmed.
                        errorMessage =
                            "Registration is not connected to the backend yet."
                    },

                    onBackClick = {
                        showRegisterScreen = false
                        errorMessage = ""
                    },

                    loading = false,
                    errorMessage = errorMessage
                )

            } else {

                LoginScreen(
                    onLoginClick = { email, password ->

                        loading = true
                        errorMessage = ""

                        lifecycleScope.launch {

                            try {

                                val token = withContext(Dispatchers.IO) {
                                    ApiClient.login(
                                        email = email,
                                        password = password
                                    )
                                }

                                preferences.edit()
                                    .putString("jwt_token", token)
                                    .apply()

                                val loadedDevices =
                                    withContext(Dispatchers.IO) {
                                        ApiClient.getDevices(token)
                                    }

                                devices = loadedDevices
                                loggedIn = true

                            } catch (e: Exception) {

                                errorMessage =
                                    e.message ?: "Something went wrong"

                            } finally {

                                loading = false
                            }
                        }
                    },

                    onRegisterClick = {
                        errorMessage = ""
                        showRegisterScreen = true
                    },

                    onDemoModeClick = {
                        devices = MockDevices.devices
                        loggedIn = true
                    },

                    loading = loading,
                    errorMessage = errorMessage
                )
            }

        } else {

            /*
             * HOME SCREEN
             */
            if (selectedDevice == null) {

                HomeScreen(
                    devices = devices,

                    onDeviceClick = { device ->
                        selectedDevice = device
                    }
                )

            } else {

                /*
                 * DEVICE DETAILS
                 */
                DeviceDetailsScreen(
                    device = selectedDevice!!,

                    onBackClick = {
                        selectedDevice = null
                    },

                    onStateChange = { newState ->

                        val currentDevice = selectedDevice

                        if (currentDevice != null) {

                            val updatedDevice = currentDevice.copy(
                                status = newState
                            )

                            selectedDevice = updatedDevice

                            devices = devices.map { device ->

                                if (device.id == updatedDevice.id) {
                                    updatedDevice
                                } else {
                                    device
                                }
                            }
                        }
                    }
                )
            }
        }
    }
}