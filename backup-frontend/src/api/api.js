const API_BASE_URL = import.meta.env.DEV
  ? "/api"
  : "https://rimmed-crave-lip.ngrok-free.dev/api";

const TOKEN_KEY = "token";

let unauthorizedHandler = null;

export function setUnauthorizedHandler(handler) {
  unauthorizedHandler = handler;
}

export function getToken() {
  return localStorage.getItem(TOKEN_KEY);
}

export function setToken(token) {
  localStorage.setItem(TOKEN_KEY, token);
}

export function clearToken() {
  localStorage.removeItem(TOKEN_KEY);
}

function handleUnauthorized() {
  clearToken();
  if (unauthorizedHandler) {
    unauthorizedHandler();
  }
}

export async function apiRequest(path, options = {}) {
  const { method = "GET", body, auth = false } = options;
  const headers = {
    "Content-Type": "application/json",
    "ngrok-skip-browser-warning": "true",
  };

  if (auth) {
    const token = getToken();
    if (token) {
      headers.Authorization = `Bearer ${token}`;
    }
  }

  let response;
  try {
    response = await fetch(`${API_BASE_URL}${path}`, {
      method,
      headers,
      body: body !== undefined ? JSON.stringify(body) : undefined,
    });
  } catch {
    throw new Error("Network error. Please check your connection and try again.");
  }

  if (auth && response.status === 401) {
    handleUnauthorized();
    throw new Error("Session expired. Please log in again.");
  }

  let data = null;
  const text = await response.text();
  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      data = { message: text };
    }
  }

  if (!response.ok) {
    const message =
      data?.message ||
      data?.error ||
      `Request failed (${response.status})`;
    throw new Error(message);
  }

  return data;
}

export function registerUser(email, password) {
  return apiRequest("/users/register", {
    method: "POST",
    body: { email, password },
  });
}

export function loginUser(email, password) {
  return apiRequest("/users/login", {
    method: "POST",
    body: { email, password },
  });
}

export function getDevices() {
  return apiRequest("/devices", { auth: true });
}

export function updateDeviceState(id, state) {
  return apiRequest(`/devices/${id}`, {
    method: "PUT",
    auth: true,
    body: { state },
  });
}

export function deleteDevice(id) {
  return apiRequest(`/devices/${id}`, {
    method: "DELETE",
    auth: true,
  });
}
