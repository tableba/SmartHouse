import { useEffect, useState } from "react";

function normalizeType(type) {
  return String(type || "")
    .replace(/([a-z])([A-Z])/g, "$1 $2")
    .toLowerCase()
    .replace(/[_-]+/g, " ")
    .trim();
}

function BooleanControl({ id, label, checked, disabled, onChange }) {
  return (
    <label className="control-row">
      <input
        id={id}
        type="checkbox"
        checked={Boolean(checked)}
        disabled={disabled}
        onChange={(e) => onChange(e.target.checked)}
      />
      {label}
    </label>
  );
}

function NumberControl({ id, label, value, disabled, onCommit }) {
  return (
    <label className="control-row">
      {label}
      <input
        id={id}
        type="number"
        defaultValue={value ?? ""}
        key={`${id}-${value}`}
        disabled={disabled}
        onBlur={(e) => {
          const next = Number(e.target.value);
          if (Number.isNaN(next)) return;
          if (next === value) return;
          onCommit(next);
        }}
        onKeyDown={(e) => {
          if (e.key === "Enter") {
            e.target.blur();
          }
        }}
      />
    </label>
  );
}

function RangeControl({ id, label, value, disabled, onCommit }) {
  const [local, setLocal] = useState(value ?? 0);

  useEffect(() => {
    setLocal(value ?? 0);
  }, [value]);

  function commit() {
    if (local !== value) onCommit(local);
  }

  return (
    <label className="control-row">
      {label} ({local})
      <input
        id={id}
        type="range"
        min="0"
        max="100"
        value={local}
        disabled={disabled}
        onChange={(e) => setLocal(Number(e.target.value))}
        onMouseUp={commit}
        onTouchEnd={commit}
        onKeyUp={(e) => {
          if (e.key === "Enter" || e.key === "ArrowLeft" || e.key === "ArrowRight") {
            commit();
          }
        }}
      />
    </label>
  );
}

function DeviceControls({ device, disabled, onChange }) {
  const type = normalizeType(device.type);
  const state = device.state && typeof device.state === "object" ? device.state : {};

  function change(key, value) {
    onChange({ ...state, [key]: value });
  }

  if (type === "light") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-power`}
          label="Power"
          checked={state.power}
          disabled={disabled}
          onChange={(value) => change("power", value)}
        />
        <RangeControl
          id={`${device.id}-brightness`}
          label="Brightness"
          value={state.brightness}
          disabled={disabled}
          onCommit={(value) => change("brightness", value)}
        />
      </div>
    );
  }

  if (type === "door" || type === "window") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-open`}
          label="Open"
          checked={state.open}
          disabled={disabled}
          onChange={(value) => change("open", value)}
        />
      </div>
    );
  }

  if (type === "fan") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-power`}
          label="Power"
          checked={state.power}
          disabled={disabled}
          onChange={(value) => change("power", value)}
        />
        <NumberControl
          id={`${device.id}-speed`}
          label="Speed"
          value={state.speed}
          disabled={disabled}
          onCommit={(value) => change("speed", value)}
        />
      </div>
    );
  }

  if (type === "coffee machine") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-power`}
          label="Power"
          checked={state.power}
          disabled={disabled}
          onChange={(value) => change("power", value)}
        />
        <BooleanControl
          id={`${device.id}-brewing`}
          label="Brewing"
          checked={state.brewing}
          disabled={disabled}
          onChange={(value) => change("brewing", value)}
        />
      </div>
    );
  }

  if (type === "temperature sensor") {
    return (
      <div className="controls">
        <NumberControl
          id={`${device.id}-temperature`}
          label="Temperature"
          value={state.temperature}
          disabled={disabled}
          onCommit={(value) => change("temperature", value)}
        />
      </div>
    );
  }

  if (type === "motion sensor") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-motion`}
          label="Motion"
          checked={state.motion}
          disabled={disabled}
          onChange={(value) => change("motion", value)}
        />
      </div>
    );
  }

  if (type === "alarm") {
    return (
      <div className="controls">
        <BooleanControl
          id={`${device.id}-active`}
          label="Active"
          checked={state.active}
          disabled={disabled}
          onChange={(value) => change("active", value)}
        />
      </div>
    );
  }

  return <p className="muted">No controls for this device type.</p>;
}

export default DeviceControls;
