#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 resolution;
    vec4 ink;
    float phase;
};
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}
void main() {
    vec2 uv = qt_TexCoord0;
    float aspect = resolution.x / max(resolution.y, 1.0);
    vec2 p = (uv - vec2(0.73, 0.49)) * vec2(aspect, 1.0);
    float t = phase * 0.16;
    // An asymmetric confluence of folded light. No textures, ray marching or blur passes.
    float flow = noise(p * 2.3 + vec2(t * 0.28, -t * 0.22));
    vec3 light = vec3(0.0);
    for (int i = 0; i < 6; ++i) {
        float k = float(i);
        float bend = sin(p.x * (2.1 + k * 0.16) + t + k * 0.53) * 0.19;
        bend += sin(p.x * 4.2 - t * 0.7 + k * 0.41) * 0.055;
        float curve = p.y - p.x * 0.34 - bend - (k - 2.5) * 0.078 - (flow - 0.5) * 0.10;
        float width = 0.035 + 0.012 * sin(p.x * 2.0 + k + t);
        float body = exp(-abs(curve) / width);
        float edge = exp(-abs(curve + width * 0.65) * 330.0);
        float aperture = exp(-pow(abs(p.x + 0.04 * k) * 0.90, 3.0));
        vec3 hue = mix(vec3(0.025, 0.17, 0.88), ink.rgb, 0.5 + 0.5 * sin(k * 0.8 + p.x * 1.8 + t));
        light += (hue * body * 0.28 + vec3(0.55, 0.96, 1.0) * edge * 0.40) * aperture;
    }
    float veil = exp(-dot(p * vec2(0.85, 1.6), p * vec2(0.85, 1.6)) * 2.5);
    light += vec3(0.02, 0.12, 0.24) * veil;
    light = vec3(1.0) - exp(-light * 1.55);
    float mask = smoothstep(0.25, 0.58, uv.x);
    float vignette = smoothstep(0.0, 0.16, uv.y) * (1.0 - smoothstep(0.84, 1.0, uv.y));
    vec2 q = abs(uv * resolution - resolution * 0.5) - resolution * 0.5 + 28.0;
    float sdf = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - 28.0;
    float alpha = mask * vignette * (1.0 - smoothstep(-1.0, 0.0, sdf));
    fragColor = vec4(light * alpha, alpha) * qt_Opacity;
}
