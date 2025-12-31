const timeData = [];
const bpmData = [];
const avgData = [];
const MAX_POINTS = 600;
const SAMPLE_INTERVAL_MS = 200;

const plot = new uPlot({
  width: window.innerWidth,
  height: 300,
  scales: { x: { time: false } },
  series: [
    {},
    { label: "BPM", stroke: "red", width: 2 },
    { label: "Avg BPM", stroke: "orange", width: 1 },
  ],
}, [timeData, bpmData, avgData], document.getElementById("chart"));

function updateMetricBox(entry) {
  const metrics = document.getElementById("metrics");
  metrics.textContent = `IR: ${entry.irValue} | BPM: ${entry.beatsPerMinute.toFixed(1)} | Avg: ${entry.averageBPM.toFixed(1)}`;
}

function appendMeasurements(measurements) {
  if (!measurements.length) {
    return;
  }

  let lastEntry = null;

  measurements.forEach(entry => {
    const timeSeconds = entry.timestamp / 1000;
    timeData.push(timeSeconds);
    bpmData.push(entry.beatsPerMinute);
    avgData.push(entry.averageBPM);
    lastEntry = entry;
  });

  const overflow = timeData.length - MAX_POINTS;
  if (overflow > 0) {
    timeData.splice(0, overflow);
    bpmData.splice(0, overflow);
    avgData.splice(0, overflow);
  }

  if (lastEntry) {
    updateMetricBox(lastEntry);
  }

  plot.setData([timeData, bpmData, avgData]);
}

function fetchMeasurements() {
  fetch("/api/measurements")
    .then(response => response.json())
    .then(body => {
      if (Array.isArray(body.measurements)) {
        appendMeasurements(body.measurements);
      }
    })
    .catch(err => console.error("Failed to fetch measurement batch", err));
}

fetchMeasurements();
let fetchTimer = setInterval(fetchMeasurements, SAMPLE_INTERVAL_MS);
