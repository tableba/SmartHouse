package com.example.smarthouse.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.smarthouse.data.Device

@Composable
fun DeviceDetailsScreen(
    device: Device,
    onBackClick: () -> Unit,
    onStateChange: (String) -> Unit
) {

    var sliderValue by remember {
        mutableFloatStateOf(50f)
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(20.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {

        Button(
            onClick = onBackClick
        ) {
            Text("← Back")
        }

        Text(
            text = device.name,
            style = MaterialTheme.typography.headlineMedium
        )

        Text(
            text = "Device type: ${device.type}",
            style = MaterialTheme.typography.bodyLarge
        )

        Card(
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(
                modifier = Modifier.padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "Current status",
                    style = MaterialTheme.typography.titleMedium
                )

                Text(
                    text = device.status,
                    style = MaterialTheme.typography.headlineSmall
                )
            }
        }

        when (device.type) {

            "light" -> {

                Text(
                    text = "Light controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {

                    Button(
                        onClick = {
                            onStateChange("on")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("ON")
                    }

                    Button(
                        onClick = {
                            onStateChange("off")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("OFF")
                    }
                }

                Text("Brightness")

                Slider(
                    value = sliderValue,
                    onValueChange = {
                        sliderValue = it
                    },
                    valueRange = 0f..100f
                )

                Text(
                    text = "${sliderValue.toInt()}%"
                )
            }

            "fan" -> {

                Text(
                    text = "Fan controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {

                    Button(
                        onClick = {
                            onStateChange("low")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("LOW")
                    }

                    Button(
                        onClick = {
                            onStateChange("medium")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("MED")
                    }

                    Button(
                        onClick = {
                            onStateChange("high")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("HIGH")
                    }
                }
            }

            "door" -> {

                Text(
                    text = "Door controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Button(
                    onClick = {
                        onStateChange("open")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("OPEN")
                }

                Button(
                    onClick = {
                        onStateChange("closed")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("CLOSE")
                }
            }

            "window" -> {

                Text(
                    text = "Window controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Button(
                    onClick = {
                        onStateChange("open")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("OPEN")
                }

                Button(
                    onClick = {
                        onStateChange("closed")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("CLOSE")
                }
            }

            "coffee_machine" -> {

                Text(
                    text = "Coffee machine controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(12.dp)
                ) {

                    Button(
                        onClick = {
                            onStateChange("on")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("ON")
                    }

                    Button(
                        onClick = {
                            onStateChange("off")
                        },
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("OFF")
                    }
                }
            }

            "alarm" -> {

                Text(
                    text = "Alarm controls",
                    style = MaterialTheme.typography.titleLarge
                )

                Button(
                    onClick = {
                        onStateChange("active")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("ACTIVATE")
                }

                Button(
                    onClick = {
                        onStateChange("inactive")
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("DEACTIVATE")
                }
            }

            "temperature_sensor" -> {

                Text(
                    text = "Temperature sensor",
                    style = MaterialTheme.typography.titleLarge
                )

                Text(
                    text = "Temperature: ${device.value}"
                )

                Text(
                    text = "This device is read-only."
                )
            }

            "motion_sensor" -> {

                Text(
                    text = "Motion sensor",
                    style = MaterialTheme.typography.titleLarge
                )

                Text(
                    text = "Current reading: ${device.value}"
                )

                Text(
                    text = "This device is read-only."
                )
            }

            else -> {

                Text(
                    text = "No controls available for this device type."
                )
            }
        }
    }
}