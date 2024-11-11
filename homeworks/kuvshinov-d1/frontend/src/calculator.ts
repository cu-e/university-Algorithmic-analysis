import { CategoryScale, Chart, Legend, LinearScale, LineController, LineElement, PointElement, Title, Tooltip } from 'chart.js';

Chart.register(CategoryScale, LinearScale, Title, Tooltip, Legend, LineElement, LineController, PointElement);

export async function getGraph(element: HTMLFormElement) {
  element.addEventListener('submit', async (event) => {
    event.preventDefault();
    const formData = new FormData(element);
    const expression = formData.get('expression') as string;
    const xMin = parseFloat(formData.get('min-x') as string);
    const xMax = parseFloat(formData.get('max-x') as string);
    const step = parseFloat(formData.get('step') as string);

    try {
      //TODO: CloudFlare требует указывать явный url (http://address:port/api/ )
      const response = await fetch("http://localhost/api/", {
        method: "POST",
        headers: {
          "Expression": expression,
          "X-Min": xMin.toString(),
          "X-Max": xMax.toString(),
          "X-Step": step.toString(),
        },
      });

      if (!response.ok) {
        throw new Error('Не удалось получить данные с сервера');
      }

      const data = await response.json();
      const xValues = data.map((item: { x: number }) => item.x);
      const yValues = data.map((item: { value: number }) => item.value);

      const ctx = document.getElementById('chart') as HTMLCanvasElement;

      if (window.chartInstance) {
        window.chartInstance.destroy();
      }

      window.chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
          labels: xValues,
          datasets: [{
            label: 'X',
            data: yValues,
            fill: false,
            borderColor: '#646cff',
            tension: 0.1,
          }]
        },
        options: {
          scales: {
            x: {
              type: 'category',
              title: {
                display: true,
                text: 'X координата'
              }
            },
            y: {
              type: 'linear',
              title: {
                display: true,
                text: 'Результат функции (y)'
              }
            }
          }
        }
      });
    } catch (error) {
      console.error('Ошибка при загрузке данных:', error);
    }
  });
}
