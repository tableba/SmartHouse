import { useEffect, useState } from "react";
import {
  clearToken,
  getDevices,
  getToken,
  setUnauthorizedHandler,
} from "./api/api";
import DeviceCard from "./components/DeviceCard";
import Login from "./components/Login";
import Register from "./components/Register";
import "./App.css";

function parseDevices(data) {
  if (Array.isArray(data)) return data;
  if (Array.isArray(data?.devices)) return data.devices;
  throw new Error("Unexpected devices response");
}

function getDeviceId(device) {
  return device.id ?? device._id;
}

function App() {
  const [token, setAuthToken] = useState(() => getToken());
  const [authView, setAuthView] = useState("login");
  const [devices, setDevices] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  function logout() {
    clearToken();
    setAuthToken(null);
    setDevices([]);
    setAuthView("login");
  }

  useEffect(() => {
    setUnauthorizedHandler(logout);
    return () => setUnauthorizedHandler(null);
  }, []);

  useEffect(() => {
    if (!token) return;

    let cancelled = false;
    async function loadDevices() {
      setLoading(true);
      setError("");
      try {
        const data = await getDevices();
        if (!cancelled) {
          setDevices(parseDevices(data));
        }
      } catch (err) {
        if (!cancelled) {
          setError(err.message || "Failed to load devices");
          setDevices([]);
        }
      } finally {
        if (!cancelled) setLoading(false);
      }
    }

    loadDevices();
    return () => {
      cancelled = true;
    };
  }, [token]);

  function handleLoggedIn(nextToken) {
    setAuthToken(nextToken);
  }

  function handleUpdated(updatedDevice) {
    const updatedId = getDeviceId(updatedDevice);
    setDevices((current) =>
      current.map((device) =>
        getDeviceId(device) === updatedId ? updatedDevice : device
      )
    );
  }

  function handleDeleted(id) {
    setDevices((current) =>
      current.filter((device) => getDeviceId(device) !== id)
    );
  }

  if (!token) {
    return (
      <div className="app">
        {authView === "register" ? (
          <Register onGoToLogin={() => setAuthView("login")} />
        ) : (
          <Login
            onLoggedIn={handleLoggedIn}
            onGoToRegister={() => setAuthView("register")}
          />
        )}
      </div>
    );
  }

  return (
    <div className="app">
      <header className="header">
        <h1>Devices</h1>
        <button type="button" onClick={logout}>
          Log out
        </button>
      </header>

      <main>
        {loading ? <p>Loading devices...</p> : null}
        {error ? <p className="error">{error}</p> : null}
        {!loading && !error && devices.length === 0 ? (
          <p>No devices found.</p>
        ) : null}
        <section className="device-grid">
          {devices.map((device) => (
            <DeviceCard
              key={getDeviceId(device)}
              device={device}
              onUpdated={handleUpdated}
              onDeleted={handleDeleted}
            />
          ))}
        </section>
      </main>
    </div>
  );
}

export default App;
