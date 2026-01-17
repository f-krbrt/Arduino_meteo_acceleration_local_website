from collections import deque
import threading
import time

import serial
from fastapi import FastAPI
from fastapi.responses import HTMLResponse, JSONResponse
import uvicorn

# -------- Configuration --------
SERIAL_PORT = "COM6"      # CHANGE THIS to your actual port
BAUDRATE = 115200

MAX_ROWS = 100

INDEX_HTML_PATH = "index.html"   # same folder as this script

# -------- Global state --------
ser = None
serial_lock = threading.Lock()

data_buffer = deque(maxlen=MAX_ROWS)
buffer_lock = threading.Lock()

stop_reader = False

app = FastAPI()


def open_serial():
    global ser
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    # Allow board to reset
    time.sleep(2)


def serial_reader_loop():
    global stop_reader
    while not stop_reader:
        try:
            line = ser.readline().decode(errors="ignore").strip()
        except Exception:
            time.sleep(0.1)
            continue

        if not line:
            continue

        # Only keep IMU data lines: "t=...,ax=...,ay=...,az=..."
        if not line.startswith("t="):
            continue

        try:
            parts = line.split(",")
            vals = {}
            for p in parts:
                k, v = p.split("=", 1)
                k = k.strip()
                v = v.strip()
                if k == "t":
                    vals["t_ms"] = int(v)
                else:
                    vals[k] = float(v)

            row = {
                "t_ms": vals.get("t_ms"),
                "ax": vals.get("ax"),
                "ay": vals.get("ay"),
                "az": vals.get("az"),
                "received_at": time.time(),
            }
        except Exception:
            # Ignore malformed lines
            continue

        with buffer_lock:
            data_buffer.append(row)


def start_background_reader():
    open_serial()
    thread = threading.Thread(target=serial_reader_loop, daemon=True)
    thread.start()
    return thread


# -------- FastAPI lifecycle --------

@app.on_event("startup")
def on_startup():
    start_background_reader()


@app.on_event("shutdown")
def on_shutdown():
    global stop_reader
    stop_reader = True
    time.sleep(0.2)
    if ser is not None:
        ser.close()


# -------- Routes --------

@app.get("/", response_class=HTMLResponse)
def index():
    # Read index.html from disk and return it
    with open(INDEX_HTML_PATH, "r", encoding="utf-8") as f:
        html = f.read()
    return HTMLResponse(content=html)


@app.get("/data", response_class=JSONResponse)
def get_data():
    with buffer_lock:
        rows = list(data_buffer)
    return {"rows": rows}


@app.post("/start", response_class=JSONResponse)
def start_recording():
    if ser is None:
        return {"status": "no-serial"}
    with serial_lock:
        ser.write(b"START\n")
    return {"status": "START sent"}


@app.post("/stop", response_class=JSONResponse)
def stop_recording():
    if ser is None:
        return {"status": "no-serial"}
    with serial_lock:
        ser.write(b"STOP\n")
    return {"status": "STOP sent"}


if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8000)
