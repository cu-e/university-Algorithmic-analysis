export async function getGraph(element: HTMLFormElement) {
  element.addEventListener('click', async () => { 
    event?.preventDefault(); 
    const formData = new FormData(element)
    try {
      const response = await fetch("", {
        method: "POST",
        headers: {

        },
        body: JSON.stringify({
          body: formData.get("body"),
        })
      }); 
    } catch (error) {
      console.error(error); 
    }
  });
}
