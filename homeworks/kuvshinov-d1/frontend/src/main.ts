import { getGraph } from './calculator'
import './style.css'

document.querySelector<HTMLDivElement>('#app')!.innerHTML = `
<form id = "form-submit" class = "main-block">
<h2>Ввод формулы</h2>
  <textarea
        name = "body"
         placeholder = "Введите формулу в формате строки C++">
  </textarea>
  <button type = "submit"  >Рассчитать график</button>
  <h2>Его график:</h2>
</form>
`

getGraph(document.querySelector<HTMLFormElement>('#form-submit')!)
