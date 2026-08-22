package com.example.smarthouse.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.example.smarthouse.data.Device

@Composable
fun HomeScreen(
    devices: List<Device>,
    onDeviceClick: (Device) -> Unit
) {
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "SmartHouse",
            style = MaterialTheme.typography.headlineMedium
        )

        Text(
            text = "${devices.size} devices",
            style = MaterialTheme.typography.bodyLarge,
            modifier = Modifier.padding(
                top = 4.dp,
                bottom = 16.dp
            )
        )

        LazyColumn(
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            items(devices) { device ->

                DeviceCard(
                    device = device,
                    onClick = {
                        onDeviceClick(device)
                    }
                )
            }
        }
    }
}

@Composable
fun DeviceCard(
    device: Device,
    onClick: () -> Unit
) {
    Card(
        onClick = onClick
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Text(
                text = device.name,
                style = MaterialTheme.typography.titleLarge
            )

            Text(
                text = "Type: ${device.type}"
            )

            Text(
                text = "Status: ${device.status}"
            )

            if (device.value.isNotBlank()) {
                Text(
                    text = "Value: ${device.value}"
                )
            }
        }
    }
}