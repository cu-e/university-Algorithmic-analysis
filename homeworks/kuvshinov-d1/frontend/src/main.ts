
import { getGraph } from './calculator';
import './style.css';



document.querySelector<HTMLDivElement>('#app')!.innerHTML = `
<div class="main-block">
  <a href="https://github.com/ArashPartow/exprtk">Что такое ExprTk?</a>
  <form id="form-submit" >
    <h2>Ввод формулы</h2>
    <input
      name="expression"
      placeholder="Введите формулу в формате ExprTk"
      >
    </input>
    <p class = "pl-text">Формула в формате ExprTk</p>

    <input name="min-x" placeholder = "X-Min" type = "number" max = "1000" min = "-1000" value = "-10" >
    <p class = "pl-text">Минимальный x</p>

    <input name="max-x" placeholder = "X-Max" type = "number" max = "1000" min = "-1000" value = "10" >
    <p class = "pl-text">Максимальный x</p>

    <input name="step" placeholder = "X-Step"  value = "0.25">
    <p class = "pl-text">Шаг "точности"</p>

    <button type="submit">Рассчитать график</button>
    <h2>Его график:</h2>
    <canvas id="chart" width="800" height="400"></canvas>
  </form>
  </div>
`;

getGraph(document.querySelector<HTMLFormElement>('#form-submit')!);