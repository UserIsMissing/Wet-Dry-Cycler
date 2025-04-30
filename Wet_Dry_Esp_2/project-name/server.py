from flask import Flask, request, jsonify, render_template_string

app = Flask(__name__)
led_state = "off"

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 LED Control</title>
</head>
<body>
    <h1>LED State: {{ state }}</h1>
    <form method="POST" action="/set-led">
        <button name="state" value="on">ON</button>
        <button name="state" value="off">OFF</button>
    </form>
</body>
</html>
"""

@app.route("/")
def index():
    return render_template_string(HTML_TEMPLATE, state=led_state)

@app.route("/set-led", methods=["POST"])
def set_led():
    global led_state
    led_state = request.form["state"]
    return render_template_string(HTML_TEMPLATE, state=led_state)

@app.route("/led-state", methods=["GET"])
def get_led_state():
    global led_state
    return jsonify({"led": led_state})  # ✅ ensures proper JSON output

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)