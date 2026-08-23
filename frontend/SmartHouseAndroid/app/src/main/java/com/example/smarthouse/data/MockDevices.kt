package com.example.smarthouse.data

object MockDevices {

    val devices = listOf(

        Device(
            id = "1",
            name = "Living Room Light",
            type = "light",
            status = "off",
            value = ""
        ),

        Device(
            id = "2",
            name = "Kitchen Light",
            type = "light",
            status = "on",
            value = ""
        ),

        Device(
            id = "3",
            name = "Bedroom Fan",
            type = "fan",
            status = "on",
            value = "medium"
        ),

        Device(
            id = "4",
            name = "Front Door",
            type = "door",
            status = "closed",
            value = ""
        ),

        Device(
            id = "5",
            name = "Back Door",
            type = "door",
            status = "closed",
            value = ""
        ),

        Device(
            id = "6",
            name = "Coffee Machine",
            type = "coffee_machine",
            status = "off",
            value = ""
        ),

        Device(
            id = "7",
            name = "Living Room Temperature",
            type = "temperature_sensor",
            status = "online",
            value = "22.4°C"
        ),

        Device(
            id = "8",
            name = "Hallway Motion Sensor",
            type = "motion_sensor",
            status = "online",
            value = "no motion"
        ),

        Device(
            id = "9",
            name = "House Alarm",
            type = "alarm",
            status = "off",
            value = ""
        ),

        Device(
            id = "10",
            name = "Bedroom Window",
            type = "window",
            status = "closed",
            value = ""
        )
    )
}