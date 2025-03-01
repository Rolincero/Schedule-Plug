#pragma once
#include <Arduino.h>

const char WEB_UI[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Умная розетка</title>
  <style>
    /* Основные стили */
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background-color: #1a1a1a;
      color: #ffffff;
      margin: 0;
      padding: 0;
    }

    .tabs {
      background: #2c3e50;
      padding: 10px;
      border-radius: 5px 5px 0 0;
    }

    .tab-button {
      background: none;
      border: none;
      color: #ffffff;
      padding: 10px 20px;
      cursor: pointer;
      opacity: 0.7;
    }

    .tab-button.active {
      opacity: 1;
      border-bottom: 2px solid #3498db;
    }

    .tab-content {
      display: none;
      background: #2c3e50;
      padding: 20px;
      border-radius: 0 0 5px 5px;
    }

    .tab-content.active {
      display: block;
    }

    .schedule-table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 20px;
    }

    .schedule-table th, .schedule-table td {
      padding: 10px;
      text-align: left;
      border-bottom: 1px solid #3a3a3a;
    }

    .status-card {
      background: #2c3e50;
      padding: 20px;
      border-radius: 5px;
      margin: 10px 0;
    }

    .chart-container {
      width: 100%;
      height: 300px;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <div class="tabs">
    <button class="tab-button active" onclick="showTab('schedule')">Расписание</button>
    <button class="tab-button" onclick="showTab('status')">Статус</button>
  </div>

  <div id="schedule" class="tab-content active">
    <h2>Расписание</h2>
    <table class="schedule-table">
      <thead>
        <tr>
          <th>День</th>
          <th>Старт</th>
          <th>Стоп</th>
          <th></th>
        </tr>
      </thead>
      <tbody id="schedule-body">
        <!-- Данные будут заполняться через JS -->
      </tbody>
    </table>
    <button onclick="openEditModal()">Редактировать</button>
  </div>

  <div id="status" class="tab-content">
    <h2>Статус устройства</h2>
    <div class="status-card">
      <h3>Реле: <span id="relay-status">Выкл</span></h3>
      <h3>Температура: <span id="temp-value">0°C</span></h3>
    </div>
    <div class="chart-container">
      <canvas id="temp-chart"></canvas>
    </div>
  </div>

  <script>
    const daysOfWeek = ["Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота", "Воскресенье"];

    // Функция для переключения вкладок
    function showTab(tabName) {
      const tabs = document.querySelectorAll('.tab-content');
      tabs.forEach(tab => tab.classList.remove('active'));

      const buttons = document.querySelectorAll('.tab-button');
      buttons.forEach(button => button.classList.remove('active'));

      document.getElementById(tabName).classList.add('active');
      document.querySelector(`[onclick="showTab('${tabName}')"]`).classList.add('active');
    }

    // Функция для обновления статуса
    async function updateStatus() {
      const response = await fetch("/api/status");
      const data = await response.json();
      document.getElementById("temp-value").textContent = data.temp + "°C";
      document.getElementById("relay-status").textContent = data.relay ? "Вкл" : "Выкл";
    }

    // Обновление статуса каждые 5 секунд
    setInterval(updateStatus, 5000);

    // Инициализация графика (Chart.js)
    const ctx = document.getElementById('temp-chart').getContext('2d');
    const tempChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'Температура',
          borderColor: '#3498db',
          data: []
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        scales: {
          x: { display: false },
          y: { beginAtZero: true }
        }
      }
    });

    // Функция для обновления графика
    async function updateChart() {
      const response = await fetch("/api/temp-history");
      const data = await response.json();
      tempChart.data.labels = data.labels;
      tempChart.data.datasets[0].data = data.values;
      tempChart.update();
    }

    // Обновление графика каждые 10 секунд
    setInterval(updateChart, 10000);

    // Функция для открытия модального окна редактирования расписания
    function openEditModal() {
      fetch("/api/schedule")
        .then(response => response.json())
        .then(data => {
          const scheduleBody = document.getElementById("schedule-body");
          scheduleBody.innerHTML = "";

          for (let i = 0; i < 7; i++) {
            const row = document.createElement("tr");
            row.innerHTML = `
              <td>${daysOfWeek[i]}</td>
              <td><input type="time" value="${data["d" + i + "s"]}"></td>
              <td><input type="time" value="${data["d" + i + "e"]}"></td>
              <td><button onclick="saveSchedule(${i})">Сохранить</button></td>
            `;
            scheduleBody.appendChild(row);
          }
        })
        .catch(error => console.error("Ошибка при загрузке расписания:", error));
    }

    // Функция для сохранения расписания
    function saveSchedule(day) {
      const startTime = document.querySelector(`#schedule-body tr:nth-child(${day + 1}) input[type="time"]:nth-child(1)`).value;
      const endTime = document.querySelector(`#schedule-body tr:nth-child(${day + 1}) input[type="time"]:nth-child(2)`).value;

      fetch("/api/schedule", {
        method: "POST",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded",
        },
        body: `d${day}s=${startTime}&d${day}e=${endTime}`,
      })
        .then(response => {
          if (response.ok) {
            alert("Расписание обновлено!");
          } else {
            alert("Ошибка при обновлении расписания");
          }
        })
        .catch(error => console.error("Ошибка при сохранении расписания:", error));
    }
  </script>
</body>
</html>
)=====";