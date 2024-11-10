// globals.d.ts
import { Chart } from 'chart.js';

declare global {
  interface Window {
    chartInstance?: Chart<'line', any, any>; 
  }
}
export { };
