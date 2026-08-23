package com.example.smarthouse.data

import org.json.JSONArray
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

object ApiClient {

    private const val BASE_URL =
        "https://rimmed-crave-lip.ngrok-free.dev/api"

    fun login(email: String, password: String): String {
        val url = URL("$BASE_URL/users/login")

        val connection = url.openConnection() as HttpURLConnection

        connection.requestMethod = "POST"
        connection.setRequestProperty("Content-Type", "application/json")
        connection.setRequestProperty("Accept", "application/json")
        connection.doOutput = true

        val body = JSONObject().apply {
            put("email", email)
            put("password", password)
        }

        connection.outputStream.use { output ->
            output.write(body.toString().toByteArray())
        }

        val responseCode = connection.responseCode

        val response = if (responseCode in 200..299) {
            connection.inputStream.bufferedReader().use { it.readText() }
        } else {
            connection.errorStream?.bufferedReader()?.use { it.readText() }
                ?: "Unknown error"
        }

        connection.disconnect()

        if (responseCode !in 200..299) {
            throw Exception("Login failed: HTTP $responseCode\n$response")
        }

        return JSONObject(response).getString("token")
    }

    fun getDevices(token: String): List<Device> {
        val url = URL("$BASE_URL/devices")

        val connection = url.openConnection() as HttpURLConnection

        connection.requestMethod = "GET"
        connection.setRequestProperty(
            "Authorization",
            "Bearer $token"
        )
        connection.setRequestProperty(
            "Accept",
            "application/json"
        )

        val responseCode = connection.responseCode

        val response = if (responseCode in 200..299) {
            connection.inputStream.bufferedReader().use { it.readText() }
        } else {
            connection.errorStream?.bufferedReader()?.use { it.readText() }
                ?: "Unknown error"
        }

        connection.disconnect()

        if (responseCode !in 200..299) {
            throw Exception("Could not get devices: HTTP $responseCode\n$response")
        }

        val jsonArray = JSONArray(response)

        val devices = mutableListOf<Device>()

        for (i in 0 until jsonArray.length()) {
            val json = jsonArray.getJSONObject(i)

            devices.add(
                Device(
                    id = json.optString("id"),
                    name = json.optString("name", "Unnamed device"),
                    type = json.optString("type", "unknown"),
                    status = json.optString("status", "unknown"),
                    value = json.optString("value", "")
                )
            )
        }

        return devices
    }
}