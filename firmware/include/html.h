static const char HTML_CONTENT[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Image Setup pov fan</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap" rel="stylesheet">
    <style>
        :root { --bg: #0f172a; --card: #1e293b; --accent: #3b82f6; --text: #f8fafc; }
        body { font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); margin: 0; display: flex; justify-content: center; align-items: center; min-height: 100vh; overflow: hidden; }
        .app-container { background: var(--card); padding: 30px; border-radius: 24px; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); width: 100%; max-width: 350px; text-align: center; }
        h2 { margin-top: 0; font-weight: 600; }
                
        .crop-wrapper { width: 288px; height: 288px; border-radius: 50%; border: 4px solid var(--accent); margin: 0 auto 20px auto; overflow: hidden; position: relative; background: #000; box-shadow: inset 0 0 20px rgba(0,0,0,0.8); display: none; touch-action: none; cursor: grab; }
        .crop-wrapper:active { cursor: grabbing; }
        #source-img { position: absolute; transform-origin: top left; pointer-events: none; }

        .preset-group { margin-top: 20px; text-align: left; }
        .preset-label { display: block; font-size: 13px; color: #94a3b8; margin-bottom: 8px; font-weight: 600; }
        .preset-options { display: flex; gap: 6px; background: #0f172a; padding: 4px; border-radius: 12px; }
        .preset-btn { flex: 1; padding: 8px 6px; border: none; border-radius: 9px; background: transparent; color: #94a3b8; font-family: inherit; font-size: 13px; font-weight: 600; cursor: pointer; transition: 0.15s; }
        .preset-btn.active { background: var(--accent); color: #fff; box-shadow: 0 2px 6px rgba(59, 130, 246, 0.4); }
        .preset-btn:hover:not(.active) { color: #f8fafc; }

        .action-buttons { display: flex; gap: 10px; margin-top: 20px; }
        .btn { flex: 1; padding: 12px; border: none; border-radius: 12px; font-size: 16px; font-weight: 600; cursor: pointer; color: white; transition: 0.2s; }
        .start-btn { background: #10b981; box-shadow: 0 4px 6px rgba(16, 185, 129, 0.3); }
        .start-btn:hover { background: #059669; }
        .stop-btn { background: #ef4444; box-shadow: 0 4px 6px rgba(239, 68, 68, 0.3); }
        .stop-btn:hover { background: #dc2626; }

        #status { margin-top: 15px; font-size: 14px; color: #94a3b8; }
    </style>
</head>
<body>

<div class="app-container">
    <h2>POV ventilator</h2>

    <p id="connection" style="margin-top: 0; color: #94a3b8; font-size: 14px;">Connecting to fan...</p>

    <div class="preset-group">
        <label class="preset-label">Image preset</label>
        <div class="preset-options" id="preset-options">
            <button class="preset-btn active" data-preset="test">Test image</button>
            <button class="preset-btn" data-preset="soup">Soup</button>
            <button class="preset-btn" data-preset="hackclub">Hack Club</button>
        </div>
    </div>

    <div class="action-buttons">
        <button class="btn start-btn" id="start-btn">Start</button>
        <button class="btn stop-btn" id="stop-btn">Stop</button>
    </div>
    
    <div id="status"></div>
</div>

<script>
    const status = document.getElementById('status');
    const connection = document.getElementById('connection');
    const startButton = document.getElementById('start-btn');
    const stopButton = document.getElementById('stop-btn');
    const presetOptions = document.getElementById('preset-options');

    let selectedPreset = 'test';

    presetOptions.addEventListener('click', (e) => {
        const btn = e.target.closest('.preset-btn');
        if (!btn) return;
        selectedPreset = btn.dataset.preset;
        presetOptions.querySelectorAll('.preset-btn').forEach(b => b.classList.toggle('active', b === btn));
    });

    async function refreshStatus() {
        try {
            const res = await fetch('/status');
            if (!res.ok) {
                throw new Error('Status request failed');
            }

            const data = await res.json();
            connection.innerText = 'Device online';
            connection.style.color = '#4ade80';
            const remainingSeconds = Number(data.remaining_ms || 0) / 1000;
            const parts = [];
            parts.push(`Displaying: ${data.displaying ? 'yes' : 'no'}`);
            if (data.displaying) {
                parts.push(`Remaining: ${remainingSeconds.toFixed(1)}s`);
            }
            parts.push(`RPM: ${Number(data.rpm).toFixed(1)}`);
            parts.push(`Speed: ${data.motor_speed}%`);
            status.innerText = parts.join(' | ');
            status.style.color = data.displaying ? '#4ade80' : '#94a3b8';
            startButton.disabled = Boolean(data.displaying);
            stopButton.disabled = !Boolean(data.displaying);
        } catch (e) {
            connection.innerText = 'Connection error';
            connection.style.color = '#f87171';
            status.innerText = 'Unable to read fan status.';
            status.style.color = '#f87171';
            startButton.disabled = false;
            stopButton.disabled = false;
        }
    }

    async function sendCommand(path) {
        status.innerText = 'Sending command...';
        try {
            const res = await fetch(path, { method: 'POST' });
            if (!res.ok) {
                throw new Error('Command failed');
            }
            await refreshStatus();
        } catch (e) {
            status.innerText = 'Command failed.';
            status.style.color = '#f87171';
        }
    }

    startButton.addEventListener('click', () => sendCommand(`/start?preset=${encodeURIComponent(selectedPreset)}`));
    stopButton.addEventListener('click', () => sendCommand('/stop'));

    refreshStatus();
    setInterval(refreshStatus, 1000);

</script>
</body>
</html>
)rawliteral";