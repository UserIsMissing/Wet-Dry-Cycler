from flask import Flask, request, jsonify, render_template_string
from collections import deque

app = Flask(__name__)

# State
led_state = "off"
adc_history = deque(maxlen=50)  # Stores the last 50 ADC readings

# HTML and JS frontend
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>ESP32 LED and ADC Monitor</h1>

    <h2>LED State: <span id="ledState">{{ led }}</span></h2>
    <button onclick="setLED('on')">Turn ON</button>
    <button onclick="setLED('off')">Turn OFF</button>

    <h2>Live ADC Readings</h2>
    <canvas id="adcChart" width="600" height="200"></canvas>

    <script>
        async function setLED(state) {
            const response = await fetch('/set-led', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ state: state })
            });
            const result = await response.json();
            document.getElementById('ledState').innerText = result.led;
        }

        const ctx = document.getElementById('adcChart').getContext('2d');
        const chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: Array.from({length: {{ history|length }}}, (_, i) => i + 1),
                datasets: [{
                    label: 'ADC Value',
                    data: {{ history }},
                    borderColor: 'blue',
                    backgroundColor: 'lightblue',
                    fill: false,
                    tension: 0.3
                }]
            },
            options: {
                animation: false,
                scales: {
                    y: {
                        beginAtZero: true,
                        suggestedMax: 4095
                    }
                }
            }
        });

        setInterval(async () => {
            const response = await fetch('/adc-data');
            const data = await response.json();
            chart.data.labels = data.history.map((_, i) => i + 1);
            chart.data.datasets[0].data = data.history;
            chart.update();
        }, 1000);
    </script>
</body>
</html>
"""

# Serve the dashboard page
@app.route("/")
def index():
    return render_template_string(HTML_TEMPLATE, led=led_state, history=list(adc_history))

# Set LED state (via browser button)
@app.route("/set-led", methods=["POST"])
def set_led():
    global led_state
    if not request.is_json:
        return jsonify({"error": "Expected JSON"}), 415
    data = request.get_json()
    led_state = data.get("state", "off")
    return jsonify({"led": led_state})

# ESP32 polls this to get LED command
@app.route("/led-state", methods=["GET"])
def get_led_state():
    return jsonify({"led": led_state})

# ESP32 posts ADC data here / browser fetches it
@app.route("/adc-data", methods=["POST", "GET"])
def adc_data():
    if request.method == "POST":
        if not request.is_json:
            return jsonify({"error": "Expected JSON"}), 415
        data = request.get_json()
        adc = data.get("adc")
        if adc is not None:
            adc_history.append(adc)
            return jsonify({"status": "ok", "received": adc})
        return jsonify({"error": "Missing 'adc' key"}), 400
    else:
        return jsonify({"history": list(adc_history)})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
