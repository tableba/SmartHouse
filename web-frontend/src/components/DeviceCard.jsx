import { useState } from "react";
import { deleteDevice, updateDeviceState } from "../api/api";
import DeviceControls from "./DeviceControls";

function getDeviceId(device) {
  return device.id ?? device._id;
}

function formatState(state) {
  if (!state || typeof state !== "object") return "—";
  return Object.entries(state)
    .map(([key, value]) => `${key}: ${String(value)}`)
    .join(", ");
}

function DeviceCard({ device, onUpdated, onDeleted }) {
  const [busy, setBusy] = useState("");
  const [error, setError] = useState("");
  const id = getDeviceId(device);

  async function handleStateChange(nextState) {
    setError("");
    setBusy("updating");
    try {
      await updateDeviceState(id, nextState);
      onUpdated({ ...device, state: nextState });
    } catch (err) {
      setError(err.message || "Failed to update device");
    } finally {
      setBusy("");
    }
  }

  async function handleDelete() {
    if (!window.confirm("Delete this device?")) return;
    setError("");
    setBusy("deleting");
    try {
      await deleteDevice(id);
      onDeleted(id);
    } catch (err) {
      setError(err.message || "Failed to delete device");
      setBusy("");
    }
  }

  return (
    <article className="device-card">
      <h2>{device.name || "Unnamed device"}</h2>
      {device.type ? <p className="device-type">{device.type}</p> : null}
      {id ? <p className="muted">ID: {id}</p> : null}
      {device.status ? <p className="muted">Status: {device.status}</p> : null}
      <p className="device-state">{formatState(device.state)}</p>
      <DeviceControls
        device={{ ...device, id }}
        disabled={Boolean(busy)}
        onChange={handleStateChange}
      />
      {error ? <p className="error">{error}</p> : null}
      {busy === "updating" ? <p className="muted">Updating...</p> : null}
      {busy === "deleting" ? <p className="muted">Deleting...</p> : null}
      <button
        type="button"
        className="danger"
        onClick={handleDelete}
        disabled={Boolean(busy)}
      >
        Delete
      </button>
    </article>
  );
}

export default DeviceCard;
