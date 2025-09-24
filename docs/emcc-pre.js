const eventListeners = []
const controlsElement = document.getElementById('controls')
        
function updateControls() {
    const ids = [...Array(Module._emcontrols_get_control_count()).keys()]
    eventListeners.forEach(
        ({element, listeners}) => listeners.forEach(({type, listener}) => 
            element.removeEventListener(type, listener)
        )
    )

    controlsElement.innerHTML = ''
    ids.forEach((id) => {
        const label = Module.UTF8ToString(Module._emcontrols_get_label(id))

        let listeners, element
        if (Module._emcontrols_is_button(id)) {
            element = document.createElement('button')
            element.id = id
            element.innerHTML = label

            const onPress = () => Module._emcontrols_trigger_button_press(id)
            listeners = [
                {
                    type: 'click',
                    listener: element.addEventListener('click', onPress)
                }
            ]
        } else if (Module._emcontrols_is_slider(id)) {
            const min = Module._emcontrols_get_slider_min(id)
            const max = Module._emcontrols_get_slider_max(id)
            const value = Module._emcontrols_get_slider_value(id)

            element = document.createElement('div')
            element.id = `slider-container-${id}`
            element.className = 'slider-container'
            
            const labelElement = document.createElement('label')
            labelElement.for = `slider-${id}`
            labelElement.textContent = label + ':'
            labelElement.className = 'slider-label'
            element.appendChild(labelElement)

            const valueElement = document.createElement('span')
            valueElement.textContent = value.toFixed(2)
            element.appendChild(valueElement)

            inputElement = document.createElement('input')
            inputElement.type = 'range'
            inputElement.id = `slider-${id}`
            inputElement.className = 'slider-input'
            element.appendChild(inputElement)

            inputElement.min = min
            inputElement.max = max
            inputElement.value = value
            inputElement.step = 0.01

            const onInput = (e) => {
                const newValue = parseFloat(e.target.value)
                valueElement.textContent = newValue.toFixed(2)
                Module._emcontrols_set_slider_value(id, newValue)
            }

            listeners = [
                {
                    type: 'input',
                    listener: inputElement.addEventListener('input', onInput)
                }
            ]
        }

        eventListeners.push({element, listeners})
        controlsElement.appendChild(element)
    })  
}

Module = {
    onRuntimeInitialized: function() {
        new ResizeObserver(entries => {
            const {x, y, width, height} = entries[0].contentRect

            const pxWidth = Math.floor(width * window.devicePixelRatio)
            const pxHeight = Math.floor(height * window.devicePixelRatio)

            canvas.width = pxWidth
            canvas.height = pxHeight
            Module._os_gfx_wasm_resize_callback(Math.floor(x), Math.floor(y), pxWidth, pxHeight, window.devicePixelRatio)
        }).observe(document.getElementById('canvas'))

        const updateControlsCallbackPtr = Module.addFunction(updateControls, 'v')
        Module._emcontrols_set_update_callback(updateControlsCallbackPtr)
        updateControls()
    }
};