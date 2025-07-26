Module = {
    onRuntimeInitialized: function() {
        new ResizeObserver(entries => {
            const {x, y, width, height} = entries[0].contentRect;

            // Set drawing buffer size (internal resolution)
            canvas.width = width * window.devicePixelRatio;
            canvas.height = height * window.devicePixelRatio;
            
            Module._os_gfx_wasm_resize_callback(Math.floor(x), Math.floor(y), Math.floor(width), Math.floor(height));
        }).observe(document.getElementById('canvas'));
    }
};