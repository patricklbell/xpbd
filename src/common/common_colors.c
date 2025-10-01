internal vec3_f32 hsl_to_rgb(vec3_f32 hsl) {
    f32 h = hsl.h;
    f32 s = hsl.s;
    f32 l = hsl.l;

    // Clamp HSL values to [0.0, 1.0]
    h = (h < 0.0f) ? 0.0f : (h > 1.0f) ? 1.0f : h;
    s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s;
    l = (l < 0.0f) ? 0.0f : (l > 1.0f) ? 1.0f : l;

    // If saturation is near zero, return grayscale
    if (s < 1e-6f) {
        return make_3f32(l, l, l);
    }

    // Convert hue to [0.0, 6.0)
    f32 h6 = h * 6.0f;
    if (h6 >= 6.0f) h6 = 0.0f;  // Wrap around (hue is circular)

    // Calculate intermediate values
    int sector = (int)h6;
    f32 frac = h6 - sector;
    f32 p = l * (1.0f - s);
    f32 q = l * (1.0f - s * frac);
    f32 t = l * (1.0f - s * (1.0f - frac));

    // Compute RGB based on the sector
    switch (sector) {
        case 0:  return make_3f32(l, t, p);
        case 1:  return make_3f32(q, l, p);
        case 2:  return make_3f32(p, l, t);
        case 3:  return make_3f32(p, q, l);
        case 4:  return make_3f32(t, p, l);
        case 5:  return make_3f32(l, p, q);
        default: return make_3f32(1,1,1);
    }
}