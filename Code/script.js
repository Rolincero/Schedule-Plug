async function loadSchedule() {
	try {
		const response = await fetch('/schedule');
		if (!response.ok) throw new Error('Ошибка загрузки');
		const schedule = await response.json();

		// Заполнение полей ввода
		Object.entries(schedule).forEach(([key, value]) => {
			const input = document.querySelector(`[name="${key}"]`);
			if (input) {
				const [hours, minutes] = value.split(':');
				input.value = `${hours.padStart(2, '0')}:${minutes.padStart(2, '0')}`;
			}
		});

		// Обновление отображения длительности
		updateAllDurations();
	} catch (error) {
		showMessage(error.message, 'error');
	}
}

async function saveSchedule() {
	try {
		// Валидация всех полей
		if (!validateAll()) {
			throw new Error('Исправьте ошибки в расписании');
		}

		// Подготовка данных
		const data = {};
		document.querySelectorAll('input[type="time"]').forEach(input => {
			data[input.name] = parseTime(input.value);
		});

		// Отправка на сервер
		const response = await fetch('/schedule', {
			method: 'POST',
			headers: {'Content-Type': 'application/json'},
			body: JSON.stringify(data)
		});

		if (!response.ok) {
			const error = await response.json();
			throw new Error(error.message || 'Ошибка сервера');
		}

		// Обновление интерфейса
		document.querySelectorAll('.unsaved').forEach(row => {
			row.classList.remove('unsaved');
		});
		showMessage('Расписание успешно сохранено!', 'success');

	} catch (error) {
		showMessage(error.message, 'error');
		console.error('Ошибка сохранения:', error);
	}
}

function showMessage(text, type = 'info') {
	const messageDiv = document.getElementById('message');
	messageDiv.textContent = text;
	messageDiv.className = `message ${type}`;
	setTimeout(() => messageDiv.textContent = '', 3000);
}

const days = [
	"Понедельник", "Вторник", "Среда",
"Четверг", "Пятница", "Суббота", "Воскресенье"
];

function createScheduleForm() {
	const form = document.getElementById('schedule-form');

	days.forEach((dayName, index) => {
		const row = document.createElement('div');
		row.className = 'day-row';

		row.innerHTML = `
		<div class="day-name">${dayName}</div>
		<input type="time" name="d${index}s" step="300" class="time-input">
		<input type="time" name="d${index}e" step="300" class="time-input">
		<div class="duration-display"></div>
		`;

		form.appendChild(row);
	});

	// Добавляем обработчики изменений
	document.querySelectorAll('.time-input').forEach(input => {
		input.addEventListener('change', updateDurationDisplay);
		input.addEventListener('change', markUnsavedChanges);
	});
}

function validateTime(input) {
	const timeRegex = /^([01]\d|2[0-3]):([0-5]\d)$/;
	if (!timeRegex.test(input.value)) {
		markInvalid(input, 'Неверный формат времени');
		return false;
	}

	const [startInput, endInput] = getPairInputs(input);
	const start = parseTime(startInput.value);
	const end = parseTime(endInput.value);

	if (start >= end) {
		markInvalid(input, 'Время окончания должно быть больше');
		return false;
	}

	clearValidation(input);
	return true;
}

function parseTime(timeStr) {
	const [hours, minutes] = timeStr.split(':').map(Number);
	return hours * 3600 + minutes * 60;
}

async function handleResponse(response) {
	if (!response.ok) {
		const error = await response.json().catch(() => ({ error: 'Unknown error' }));
		throw new Error(error.message || 'Ошибка сервера');
	}
	return response.json();
}

function showMessage(text, type = 'info') {
	const messageDiv = document.getElementById('message');
	messageDiv.textContent = text;
	messageDiv.className = `message ${type}`;

	// Анимация появления
	messageDiv.style.opacity = '1';
	setTimeout(() => {
		messageDiv.style.opacity = '0';
	}, 3000);
}

function markUnsavedChanges() {
	const row = this.closest('.day-row');
	row.classList.add('unsaved');
	document.getElementById('save-button').disabled = false;
}

function updateDurationDisplay() {
	const row = this.closest('.day-row');
	const start = parseTime(row.querySelector('[name$="s"]').value);
	const end = parseTime(row.querySelector('[name$="e"]').value);

	if (start && end && start < end) {
		const duration = end - start;
		const hours = Math.floor(duration / 3600);
		const minutes = (duration % 3600) / 60;
		row.querySelector('.duration-display').textContent =
		`${hours}ч ${minutes}м`;
	}
}

function markInvalid(input, message) {
	const row = input.closest('.day-row');
	row.classList.add('invalid');
	row.querySelector('.error-message').textContent = message;
}

function clearValidation(input) {
	const row = input.closest('.day-row');
	row.classList.remove('invalid', 'unsaved');
	row.querySelector('.error-message').textContent = '';
}

function updateAllDurations() {
	document.querySelectorAll('.day-row').forEach(row => {
		const start = parseTime(row.querySelector('[name$="s"]').value);
		const end = parseTime(row.querySelector('[name$="e"]').value);

		if (start && end && start < end) {
			const duration = end - start;
			const hours = Math.floor(duration / 3600);
			const minutes = Math.round((duration % 3600) / 60);
			row.querySelector('.duration').textContent =
			`${hours}ч ${minutes.toString().padStart(2, '0')}м`;
		}
	});
}

function updateAllDurations() {
	document.querySelectorAll('.day-row').forEach(row => {
		const start = parseTime(row.querySelector('[name$="s"]').value);
		const end = parseTime(row.querySelector('[name$="e"]').value);

		if (start && end && start < end) {
			const duration = end - start;
			const hours = Math.floor(duration / 3600);
			const minutes = Math.round((duration % 3600) / 60);
			row.querySelector('.duration').textContent =
			`${hours}ч ${minutes.toString().padStart(2, '0')}м`;
		}
	});
}

document.addEventListener('DOMContentLoaded', () => {
	createScheduleForm();
	loadSchedule();

	// Автосохранение при бездействии
	let timeout;
	document.addEventListener('input', () => {
		clearTimeout(timeout);
		timeout = setTimeout(saveSchedule, 5000);
	});
});

async function saveSchedule() {
	const saveButton = document.getElementById('save-button');
	saveButton.disabled = true;
	saveButton.textContent = 'Сохранение...';

	try {
		const data = {};
		document.querySelectorAll('.time-input').forEach(input => {
			if (!validateTime(input)) throw new Error('Неверные данные');
			data[input.name] = parseTime(input.value);
		});

		const response = await fetch('/schedule', {
			method: 'POST',
			headers: { 'Content-Type': 'application/json' },
			body: JSON.stringify(data)
		});

		await handleResponse(response);
		document.querySelectorAll('.unsaved').forEach(el => el.classList.remove('unsaved'));
	} catch (error) {
		showMessage(error.message, 'error');
	} finally {
		saveButton.textContent = 'Сохранить расписание';
	}
}

// Загружаем расписание при старте
document.addEventListener('DOMContentLoaded', () => {
	// Инициализация обработчиков событий
	setupAutosave();
	setupValidation();

	// Первоначальная загрузка данных
	loadSchedule().then(() => {
		// Обновление длительности после загрузки
		updateAllDurations();
	});

	// Обработчик для кнопки сохранения
	document.getElementById('save-button').addEventListener('click', saveSchedule);
});
